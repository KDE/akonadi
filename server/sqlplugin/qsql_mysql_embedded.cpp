/****************************************************************************
**
** Copyright (C) 1992-2006 Trolltech ASA. All rights reserved.
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qsql_mysql_embedded.h"
#include "qsql_mysql_embedded.moc"

#include <qcoreapplication.h>
#include <qvariant.h>
#include <qdatetime.h>
#include <qsqlerror.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qsqlquery.h>
#include <qsqlrecord.h>
#include <qstringlist.h>
#include <qtextcodec.h>
#include <qthread.h>
#include <qvector.h>

#include <qdebug.h>

#ifdef Q_OS_WIN32
// comment the next line out if you want to use MySQL/embedded on Win32 systems.
// note that it will crash if you don't statically link to the mysql/e library!
# define Q_NO_MYSQL_EMBEDDED
#endif

Q_DECLARE_METATYPE(MYSQL_RES*)
Q_DECLARE_METATYPE(MYSQL*)

#if MYSQL_VERSION_ID >= 40108
Q_DECLARE_METATYPE(MYSQL_STMT*)
#endif

#if MYSQL_VERSION_ID >= 40100
#  define Q_CLIENT_MULTI_STATEMENTS CLIENT_MULTI_STATEMENTS
#else
#  define Q_CLIENT_MULTI_STATEMENTS 0
#endif

class QMYSQLEmbeddedDriverPrivate
{
public:
    QMYSQLEmbeddedDriverPrivate() : mysql(0), tc(0), preparedQuerys(false), preparedQuerysEnabled(false) {}
    MYSQL *mysql;
    QTextCodec *tc;

    bool preparedQuerys;
    bool preparedQuerysEnabled;
    QStringList commandLineArgs;
};

class QMYSQLEmbeddedResultPrivate : public QMYSQLEmbeddedDriverPrivate
{
public:
    QMYSQLEmbeddedResultPrivate() : QMYSQLEmbeddedDriverPrivate(), result(0), tc(QTextCodec::codecForLocale()),
        rowsAffected(0), hasBlobs(false)
#if MYSQL_VERSION_ID >= 40108
        , stmt(0), meta(0), inBinds(0), outBinds(0)
#endif
        {}

    MYSQL_RES *result;
    MYSQL_ROW row;
    QTextCodec *tc;

    int rowsAffected;

    bool bindInValues();
    void bindBlobs();

    bool hasBlobs;
    struct QMyField
    {
        QMyField()
            : outField(0), nullIndicator(false), bufLength(0ul),
              myField(0), type(QVariant::Invalid)
        {}
        char *outField;
        my_bool nullIndicator;
        ulong bufLength;
        MYSQL_FIELD *myField;
        QVariant::Type type;
    };

    QVector<QMyField> fields;

#if MYSQL_VERSION_ID >= 40108
    MYSQL_STMT* stmt;
    MYSQL_RES* meta;

    MYSQL_BIND *inBinds;
    MYSQL_BIND *outBinds;
#endif
};

static QTextCodec* codec(MYSQL* mysql)
{
#if MYSQL_VERSION_ID >= 32321
    QTextCodec* heuristicCodec = QTextCodec::codecForName(mysql_character_set_name(mysql));
    if (heuristicCodec)
        return heuristicCodec;
#endif
    return QTextCodec::codecForLocale();
}

static QSqlError qMakeError(const QString& err, QSqlError::ErrorType type,
                            const QMYSQLEmbeddedDriverPrivate* p)
{
    const char *cerr = mysql_error(p->mysql);
    return QSqlError(QLatin1String("QMYSQLEmbedded: ") + err,
                     p->tc ? p->tc->toUnicode(cerr) : QString::fromLatin1(cerr),
                     type, mysql_errno(p->mysql));
}


static QVariant::Type qDecodeMYSQLEmbeddedType(int mysqltype, uint flags)
{
    QVariant::Type type;
    switch (mysqltype) {
    case FIELD_TYPE_TINY :
    case FIELD_TYPE_SHORT :
    case FIELD_TYPE_LONG :
    case FIELD_TYPE_INT24 :
        type = (flags & UNSIGNED_FLAG) ? QVariant::UInt : QVariant::Int;
        break;
    case FIELD_TYPE_YEAR :
        type = QVariant::Int;
        break;
    case FIELD_TYPE_LONGLONG :
        type = (flags & UNSIGNED_FLAG) ? QVariant::ULongLong : QVariant::LongLong;
        break;
    case FIELD_TYPE_FLOAT :
    case FIELD_TYPE_DOUBLE :
        type = QVariant::Double;
        break;
    case FIELD_TYPE_DATE :
        type = QVariant::Date;
        break;
    case FIELD_TYPE_TIME :
        type = QVariant::Time;
        break;
    case FIELD_TYPE_DATETIME :
    case FIELD_TYPE_TIMESTAMP :
        type = QVariant::DateTime;
        break;
    case FIELD_TYPE_BLOB :
    case FIELD_TYPE_TINY_BLOB :
    case FIELD_TYPE_MEDIUM_BLOB :
    case FIELD_TYPE_LONG_BLOB :
        type = (flags & BINARY_FLAG) ? QVariant::ByteArray : QVariant::String;
        break;
    default:
    case FIELD_TYPE_ENUM :
    case FIELD_TYPE_SET :
    case FIELD_TYPE_STRING :
    case FIELD_TYPE_VAR_STRING :
    case FIELD_TYPE_DECIMAL :
        type = QVariant::String;
        break;
    }
    return type;
}

static QSqlField qToField(MYSQL_FIELD *field, QTextCodec *tc)
{
    QSqlField f(tc->toUnicode(field->name),
                qDecodeMYSQLEmbeddedType(int(field->type), field->flags));
    f.setRequired(IS_NOT_NULL(field->flags));
    f.setLength(field->length);
    f.setPrecision(field->decimals);
    f.setSqlType(field->type);
    return f;
}

#if MYSQL_VERSION_ID >= 40108

static QSqlError qMakeStmtError(const QString& err, QSqlError::ErrorType type,
                            MYSQL_STMT* stmt)
{
    const char *cerr = mysql_stmt_error(stmt);
    return QSqlError(QLatin1String("QMYSQLEmbedded3: ") + err,
                     QString::fromLatin1(cerr),
                     type, mysql_stmt_errno(stmt));
}

static bool qIsBlob(int t)
{
    return t == MYSQL_TYPE_TINY_BLOB
           || t == MYSQL_TYPE_BLOB
           || t == MYSQL_TYPE_MEDIUM_BLOB
           || t == MYSQL_TYPE_LONG_BLOB;
}

void QMYSQLEmbeddedResultPrivate::bindBlobs()
{
    int i;
    MYSQL_FIELD *fieldInfo;
    MYSQL_BIND  *bind;
//    Q_ASSERT(meta);

    for(i = 0; i < fields.count(); ++i) {
        fieldInfo = fields.at(i).myField;
        if (qIsBlob(inBinds[i].buffer_type) && meta && fieldInfo) {
            bind = &inBinds[i];
            bind->buffer_length = fieldInfo->max_length;
            delete[] static_cast<char*>(bind->buffer);
            bind->buffer = new char[fieldInfo->max_length];
            fields[i].outField = static_cast<char*>(bind->buffer);
            bind->buffer_type = MYSQL_TYPE_STRING;
        }
    }
}

bool QMYSQLEmbeddedResultPrivate::bindInValues()
{
    MYSQL_BIND *bind;
    char *field;
    int i = 0;

    if (!meta)
        meta = mysql_stmt_result_metadata(stmt);
    if (!meta)
        return false;

    fields.resize(mysql_num_fields(meta));

    inBinds = new MYSQL_BIND[fields.size()];
    memset(inBinds, 0, fields.size() * sizeof(MYSQL_BIND));

    MYSQL_FIELD *fieldInfo;

    while((fieldInfo = mysql_fetch_field(meta))) {
        QMyField &f = fields[i];
        f.myField = fieldInfo;
        f.type = qDecodeMYSQLEmbeddedType(fieldInfo->type, fieldInfo->flags);
        if (qIsBlob(fieldInfo->type)) {
            // the size of a blob-field is available as soon as we call
            // mysql_stmt_store_result()
            // after mysql_stmt_exec() in QMYSQLEmbeddedResult::exec()
            fieldInfo->length = 0;
            hasBlobs = true;
        } else {
            fieldInfo->type = MYSQL_TYPE_STRING;
        }
        bind = &inBinds[i];
        field = new char[fieldInfo->length + 1];
        memset(field, 0, fieldInfo->length + 1);

        bind->buffer_type = fieldInfo->type;
        bind->buffer = field;
        bind->buffer_length = f.bufLength = fieldInfo->length + 1;
        bind->is_null = &f.nullIndicator;
        bind->length = &f.bufLength;
        f.outField=field;

        ++i;
    }
    return true;
}
#endif

QMYSQLEmbeddedResult::QMYSQLEmbeddedResult(const QMYSQLEmbeddedDriver* db)
: QSqlResult(db)
{
    d = new QMYSQLEmbeddedResultPrivate();
    d->mysql = db->d->mysql;
    d->tc = db->d->tc;
    d->preparedQuerysEnabled = db->d->preparedQuerysEnabled;
}

QMYSQLEmbeddedResult::~QMYSQLEmbeddedResult()
{
    cleanup();
    delete d;
}

QVariant QMYSQLEmbeddedResult::handle() const
{
#if MYSQL_VERSION_ID >= 40108
    return d->meta ? qVariantFromValue(d->meta) : qVariantFromValue(d->stmt);
#else
    return qVariantFromValue(d->result);
#endif
}

void QMYSQLEmbeddedResult::cleanup()
{
    if (d->result)
        mysql_free_result(d->result);

#if MYSQL_VERSION_ID >= 40108
    if (d->stmt) {
        if (mysql_stmt_close(d->stmt))
            qWarning("QMYSQLEmbeddedResult::cleanup: unable to free statement handle");
        d->stmt = 0;
    }

    if (d->meta) {
        mysql_free_result(d->meta);
        d->meta = 0;
    }

    int i;
    for (i = 0; i < d->fields.count(); ++i)
        delete[] d->fields[i].outField;

    if (d->outBinds) {
        delete[] d->outBinds;
        d->outBinds = 0;
    }

    if (d->inBinds) {
        delete[] d->inBinds;
        d->inBinds = 0;
    }

//    for(i = 0; i < d->outFields.size(); ++i)
//        delete[] d->outFields.at(i);
#endif
    d->hasBlobs = false;
    d->fields.clear();
    d->result = NULL;
    d->row = NULL;
    setAt(-1);
    setActive(false);

    d->preparedQuerys = d->preparedQuerysEnabled;
}

bool QMYSQLEmbeddedResult::fetch(int i)
{
    if (isForwardOnly()) { // fake a forward seek
        if (at() < i) {
            int x = i - at();
            while (--x && fetchNext());
            return fetchNext();
        } else {
            return false;
        }
    }
    if (at() == i)
        return true;
    if (d->preparedQuerys) {
#if MYSQL_VERSION_ID >= 40108
        mysql_stmt_data_seek(d->stmt, i);

        if (mysql_stmt_fetch(d->stmt)) {
            setLastError(qMakeStmtError(QCoreApplication::translate("QMYSQLEmbeddedResult",
                         "Unable to fetch data"), QSqlError::StatementError, d->stmt));
            return false;
        }
#else
        return false;
#endif
    } else {
        mysql_data_seek(d->result, i);
        d->row = mysql_fetch_row(d->result);
        if (!d->row)
            return false;
    }

    setAt(i);
    return true;
}

bool QMYSQLEmbeddedResult::fetchNext()
{
    if (d->preparedQuerys) {
#if MYSQL_VERSION_ID >= 40108
        if (mysql_stmt_fetch(d->stmt))
            return false;
#else
        return false;
#endif
    } else {
    d->row = mysql_fetch_row(d->result);
    if (!d->row)
        return false;
    }
    setAt(at() + 1);
    return true;
}

bool QMYSQLEmbeddedResult::fetchLast()
{
    if (isForwardOnly()) { // fake this since MySQL can't seek on forward only queries
        bool success = fetchNext(); // did we move at all?
        while (fetchNext());
        return success;
    }

    my_ulonglong numRows;
    if (d->preparedQuerys) {
#if MYSQL_VERSION_ID >= 40108
        numRows = mysql_stmt_num_rows(d->stmt);
#else
        numRows = 0;
#endif
    } else {
        numRows = mysql_num_rows(d->result);
    }
    if (at() == int(numRows))
        return true;
    if (!numRows)
        return false;
    return fetch(numRows - 1);
}

bool QMYSQLEmbeddedResult::fetchFirst()
{
    if (at() == 0)
        return true;

    if (isForwardOnly())
        return (at() == QSql::BeforeFirstRow) ? fetchNext() : false;
    return fetch(0);
}

QVariant QMYSQLEmbeddedResult::data(int field)
{

    if (!isSelect() || field >= d->fields.count()) {
        qWarning("QMYSQLEmbeddedResult::data: column %d out of range", field);
        return QVariant();
    }

    const QMYSQLEmbeddedResultPrivate::QMyField &f = d->fields.at(field);
    QString val;
    if (d->preparedQuerys) {
        if (f.nullIndicator)
            return QVariant(f.type);

        if (f.type != QVariant::ByteArray)
            val = d->tc->toUnicode(f.outField);
    } else {
        if (d->row[field] == NULL) {
            // NULL value
            return QVariant(f.type);
        }
        if (f.type != QVariant::ByteArray)
            val = d->tc->toUnicode(d->row[field]);
    }

    switch(f.type) {
    case QVariant::LongLong:
        return QVariant(val.toLongLong());
    case QVariant::ULongLong:
        return QVariant(val.toULongLong());
    case QVariant::Int:
        return QVariant(val.toInt());
    case QVariant::UInt:
        return QVariant(val.toUInt());
    case QVariant::Double:
        return QVariant(val.toDouble());
    case QVariant::Date:
        if (val.isEmpty()) {
            return QVariant(QDate());
        } else {
            return QVariant(QDate::fromString(val, Qt::ISODate) );
        }
    case QVariant::Time:
        if (val.isEmpty()) {
            return QVariant(QTime());
        } else {
            return QVariant(QTime::fromString(val, Qt::ISODate));
        }
    case QVariant::DateTime:
        if (val.isEmpty())
            return QVariant(QDateTime());
        if (val.length() == 14)
            // TIMESTAMPS have the format yyyyMMddhhmmss
            val.insert(4, QLatin1Char('-')).insert(7, QLatin1Char('-')).insert(10,
                    QLatin1Char('T')).insert(13, QLatin1Char(':')).insert(16, QLatin1Char(':'));
        return QVariant(QDateTime::fromString(val, Qt::ISODate));
    case QVariant::ByteArray: {

        QByteArray ba;
        if (d->preparedQuerys) {
            ba = QByteArray(f.outField, f.bufLength);
        } else {
            unsigned long* fl = mysql_fetch_lengths(d->result);
            ba = QByteArray(d->row[field], fl[field]);
        }
        return QVariant(ba);
    }
    default:
    case QVariant::String:
        return QVariant(val);
    }
    qWarning("QMYSQLEmbeddedResult::data: unknown data type");
    return QVariant();
}

bool QMYSQLEmbeddedResult::isNull(int field)
{
   if (d->preparedQuerys)
       return d->fields.at(field).nullIndicator;
   else
       return d->row[field] == NULL;
}

bool QMYSQLEmbeddedResult::reset (const QString& query)
{
    if (!driver() || !driver()->isOpen() || driver()->isOpenError())
        return false;

    cleanup();
    d->preparedQuerys = false;

    const QByteArray encQuery(d->tc->fromUnicode(query));
    if (mysql_real_query(d->mysql, encQuery.data(), encQuery.length())) {
        setLastError(qMakeError(QCoreApplication::translate("QMYSQLEmbeddedResult", "Unable to execute query"),
                     QSqlError::StatementError, d));
        return false;
    }
    d->result = mysql_store_result(d->mysql);
    if (!d->result && mysql_field_count(d->mysql) > 0) {
        setLastError(qMakeError(QCoreApplication::translate("QMYSQLEmbeddedResult", "Unable to store result"),
                    QSqlError::StatementError, d));
        return false;
    }
    int numFields = mysql_field_count(d->mysql);
    setSelect(numFields != 0);
    d->fields.resize(numFields);
    d->rowsAffected = mysql_affected_rows(d->mysql);
    if (isSelect()) {
        for(int i = 0; i < numFields; i++) {
            MYSQL_FIELD* field = mysql_fetch_field_direct(d->result, i);
            d->fields[i].type = qDecodeMYSQLEmbeddedType(field->type, field->flags);
        }
    }
    setActive(true);
    return true;
}

int QMYSQLEmbeddedResult::size()
{
    if (isSelect())
        if (d->preparedQuerys)
#if MYSQL_VERSION_ID >= 40108
            return int(mysql_stmt_num_rows(d->stmt));
#else
            return -1;
#endif
        else
            return int(mysql_num_rows(d->result));
    else
        return -1;
}

int QMYSQLEmbeddedResult::numRowsAffected()
{
    return d->rowsAffected;
}

QVariant QMYSQLEmbeddedResult::lastInsertId() const
{
    if (!isActive())
        return QVariant();

    if (d->preparedQuerys) {
#if MYSQL_VERSION_ID >= 40108
        quint64 id = mysql_stmt_insert_id(d->stmt);
        if (id)
            return QVariant(id);
#endif
    } else {
        quint64 id = mysql_insert_id(d->mysql);
        if (id)
            return QVariant(id);
    }
    return QVariant();
}

QSqlRecord QMYSQLEmbeddedResult::record() const
{
    QSqlRecord info;
    MYSQL_RES *res;
    if (!isActive() || !isSelect())
        return info;

#if MYSQL_VERSION_ID >= 40108
    res = d->preparedQuerys ? d->meta : d->result;
#else
    res = d->result;
#endif

    if (!mysql_errno(d->mysql)) {
        mysql_field_seek(res, 0);
        MYSQL_FIELD* field = mysql_fetch_field(res);
        while(field) {
            info.append(qToField(field, d->tc));
            field = mysql_fetch_field(res);
        }
    }
    mysql_field_seek(res, 0);
    return info;
}


#if MYSQL_VERSION_ID >= 40108

static MYSQL_TIME *toMySqlDate(QDate date, QTime time, QVariant::Type type)
{
    Q_ASSERT(type == QVariant::Time || type == QVariant::Date
             || type == QVariant::DateTime);

    MYSQL_TIME *myTime = new MYSQL_TIME;
    memset(myTime, 0, sizeof(MYSQL_TIME));

    if (type == QVariant::Time || type == QVariant::DateTime) {
        myTime->hour = time.hour();
        myTime->minute = time.minute();
        myTime->second = time.second();
        myTime->second_part = time.msec();
    }
    if (type == QVariant::Date || type == QVariant::DateTime) {
        myTime->year = date.year();
        myTime->month = date.month();
        myTime->day = date.day();
    }

    return myTime;
}

bool QMYSQLEmbeddedResult::prepare(const QString& query)
{
    cleanup();
    if (!d->preparedQuerys)
        return QSqlResult::prepare(query);

    int r;

    if (query.isEmpty())
        return false;

    if (!d->stmt)
        d->stmt = mysql_stmt_init(d->mysql);
    if (!d->stmt) {
        setLastError(qMakeError(QCoreApplication::translate("QMYSQLEmbeddedResult", "Unable to prepare statement"),
                     QSqlError::StatementError, d));
        return false;
    }

    const QByteArray encQuery(d->tc->fromUnicode(query));
    r = mysql_stmt_prepare(d->stmt, encQuery.constData(), encQuery.length());
    if (r != 0) {
        setLastError(qMakeStmtError(QCoreApplication::translate("QMYSQLEmbeddedResult",
                     "Unable to prepare statement"), QSqlError::StatementError, d->stmt));
        cleanup();
        return false;
    }

    if (mysql_stmt_param_count(d->stmt) > 0) {// allocate memory for outvalues
        d->outBinds = new MYSQL_BIND[mysql_stmt_param_count(d->stmt)];
    }

    setSelect(d->bindInValues());
    return true;
}

bool QMYSQLEmbeddedResult::exec()
{
    if (!d->preparedQuerys)
        return QSqlResult::exec();
    if (!d->stmt)
        return false;

    int r = 0;
    MYSQL_BIND* currBind;
    QVector<MYSQL_TIME *> timeVector;
    QVector<QByteArray> stringVector;
    QVector<my_bool> nullVector;

    const QVector<QVariant> values = boundValues();

    r = mysql_stmt_reset(d->stmt);
    if (r != 0) {
        setLastError(qMakeStmtError(QCoreApplication::translate("QMYSQLEmbeddedResult",
                     "Unable to reset statement"), QSqlError::StatementError, d->stmt));
        return false;
    }

    if (mysql_stmt_param_count(d->stmt) > 0 &&
        mysql_stmt_param_count(d->stmt) == (uint)values.count()) {

        nullVector.resize(values.count());
        for (int i = 0; i < values.count(); ++i) {
            const QVariant &val = boundValues().at(i);
            void *data = const_cast<void *>(val.constData());

            currBind = &d->outBinds[i];

            nullVector[i] = static_cast<my_bool>(val.isNull());
            currBind->is_null = &nullVector[i];
            currBind->length = 0;

            switch (val.type()) {
                case QVariant::ByteArray:
                    currBind->buffer_type = MYSQL_TYPE_BLOB;
                    currBind->buffer = const_cast<char *>(val.toByteArray().constData());
                    currBind->buffer_length = val.toByteArray().size();
                    break;

                case QVariant::Time:
                case QVariant::Date:
                case QVariant::DateTime: {
                    MYSQL_TIME *myTime = toMySqlDate(val.toDate(), val.toTime(), val.type());
                    timeVector.append(myTime);

                    currBind->buffer = myTime;
                    switch(val.type()) {
                    case QVariant::Time:
                        currBind->buffer_type = MYSQL_TYPE_TIME;
                        myTime->time_type = MYSQL_TIMESTAMP_TIME;
                        break;
                    case QVariant::Date:
                        currBind->buffer_type = MYSQL_TYPE_DATE;
                        myTime->time_type = MYSQL_TIMESTAMP_DATE;
                        break;
                    case QVariant::DateTime:
                        currBind->buffer_type = MYSQL_TYPE_DATETIME;
                        myTime->time_type = MYSQL_TIMESTAMP_DATETIME;
                        break;
                    default:
                        break;
                    }
                    currBind->buffer_length = sizeof(MYSQL_TIME);
                    currBind->length = 0;
                    break; }
                case QVariant::UInt:
                case QVariant::Int:
                case QVariant::Bool:
                    currBind->buffer_type = MYSQL_TYPE_LONG;
                    currBind->buffer = data;
                    currBind->buffer_length = sizeof(int);
                    currBind->is_unsigned = (val.type() != QVariant::Int);
                    break;
                case QVariant::Double:
                    currBind->buffer_type = MYSQL_TYPE_DOUBLE;
                    currBind->buffer = data;
                    currBind->buffer_length = sizeof(double);
                    currBind->is_unsigned = 0;
                    break;
                case QVariant::LongLong:
                case QVariant::ULongLong:
                    currBind->buffer_type = MYSQL_TYPE_LONGLONG;
                    currBind->buffer = data;
                    currBind->buffer_length = sizeof(qint64);
                    currBind->is_unsigned = (val.type() == QVariant::ULongLong);
                    break;
                case QVariant::String:
                default: {
                    QByteArray ba = d->tc->fromUnicode(val.toString());
                    stringVector.append(ba);
                    currBind->buffer_type = MYSQL_TYPE_STRING;
                    currBind->buffer = const_cast<char *>(ba.constData());
                    currBind->buffer_length = ba.length();
                    currBind->is_unsigned = 0;
                    break; }
            }
        }

        r = mysql_stmt_bind_param(d->stmt, d->outBinds);
        if (r != 0) {
            setLastError(qMakeStmtError(QCoreApplication::translate("QMYSQLEmbeddedResult",
                         "Unable to bind value"), QSqlError::StatementError, d->stmt));
            qDeleteAll(timeVector);
            return false;
        }
    }
    r = mysql_stmt_execute(d->stmt);

    qDeleteAll(timeVector);

    if (r != 0) {
        setLastError(qMakeStmtError(QCoreApplication::translate("QMYSQLEmbeddedResult",
                     "Unable to execute statement"), QSqlError::StatementError, d->stmt));
        return false;
    }
    //if there is meta-data there is also data
    setSelect(d->meta);

    d->rowsAffected = mysql_stmt_affected_rows(d->stmt);

    if (isSelect()) {
        my_bool update_max_length = true;

        r = mysql_stmt_bind_result(d->stmt, d->inBinds);
        if (r != 0) {
            setLastError(qMakeStmtError(QCoreApplication::translate("QMYSQLEmbeddedResult",
                         "Unable to bind outvalues"), QSqlError::StatementError, d->stmt));
            return false;
        }
        if (d->hasBlobs)
            mysql_stmt_attr_set(d->stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &update_max_length);

        r = mysql_stmt_store_result(d->stmt);
        if (r != 0) {
            setLastError(qMakeStmtError(QCoreApplication::translate("QMYSQLEmbeddedResult",
                         "Unable to store statement results"), QSqlError::StatementError, d->stmt));
            return false;
        }

        if (d->hasBlobs) {
            // mysql_stmt_store_result() with STMT_ATTR_UPDATE_MAX_LENGTH set to true crashes
            // when called without a preceding call to mysql_stmt_bind_result()
            // in versions < 4.1.8
            d->bindBlobs();
            r = mysql_stmt_bind_result(d->stmt, d->inBinds);
            if (r != 0) {
                setLastError(qMakeStmtError(QCoreApplication::translate("QMYSQLEmbeddedResult",
                             "Unable to bind outvalues"), QSqlError::StatementError, d->stmt));
                return false;
            }
        }
        setAt(QSql::BeforeFirstRow);
    }
    setActive(true);
    return true;
}
#endif
/////////////////////////////////////////////////////////

void QMYSQLEmbeddedDriver::qServerInit()
{
#ifndef Q_NO_MYSQL_EMBEDDED
# if MYSQL_VERSION_ID >= 40000
    if ( QCoreApplication::instance()->thread() != QThread::currentThread() ) {
        mysql_thread_init();
        return;
    }

    static bool init = false;
    if (init)
        return;

    d->commandLineArgs.prepend( "mysql_test" );
    d->commandLineArgs.append( "--bootstrap" );
    static char **server_options = (char**)malloc( d->commandLineArgs.count() * sizeof( char* ) );
    for ( int i = 0; i < d->commandLineArgs.count(); ++i ) {
      server_options[ i ] = (char*)malloc( d->commandLineArgs[ i ].size() + 1 );
      qstrcpy( server_options[ i ], d->commandLineArgs[ i ].toLatin1().data() );
    }

    // this should only be called once
    // has no effect on client/server library
    // but is vital for the embedded lib
    qDebug( "tokoe: call mysql_server_init" );
    if (mysql_server_init(d->commandLineArgs.count(), server_options, 0)) {
        qWarning("QMYSQLEmbeddedDriver::qServerInit: unable to start server.");
    }

    init = true;
# endif // MYSQL_VERSION_ID
#endif // Q_NO_MYSQL_EMBEDDED
}

QMYSQLEmbeddedDriver::QMYSQLEmbeddedDriver(QObject * parent)
    : QSqlDriver(parent)
{
    init();
}

/*!
    Create a driver instance with the open connection handle, \a con.
    The instance's parent (owner) is \a parent.
*/

QMYSQLEmbeddedDriver::QMYSQLEmbeddedDriver(MYSQL * con, QObject * parent)
    : QSqlDriver(parent)
{
    init();
    if (con) {
        d->mysql = (MYSQL *) con;
        d->tc = codec(con);
        setOpen(true);
        setOpenError(false);
    }
}

void QMYSQLEmbeddedDriver::init()
{
    d = new QMYSQLEmbeddedDriverPrivate();
    d->mysql = 0;
}

QMYSQLEmbeddedDriver::~QMYSQLEmbeddedDriver()
{
    delete d;
#ifndef Q_NO_MYSQL_EMBEDDED
# if MYSQL_VERSION_ID > 40000
    if ( QCoreApplication::instance()->thread() == QThread::currentThread() )
        mysql_server_end();
    else
        mysql_thread_end();
# endif
#endif
}

bool QMYSQLEmbeddedDriver::hasFeature(DriverFeature f) const
{
    switch (f) {
    case Transactions:
// CLIENT_TRANSACTION should be defined in all recent mysql client libs > 3.23.34
#ifdef CLIENT_TRANSACTIONS
        if (d->mysql) {
            if ((d->mysql->server_capabilities & CLIENT_TRANSACTIONS) == CLIENT_TRANSACTIONS)
                return true;
        }
#endif
        return false;
    case NamedPlaceholders:
    case BatchOperations:
        return false;
    case QuerySize:
    case BLOB:
    case LastInsertId:
        return true;
    case Unicode:
        return true;
    case PreparedQueries:
    case PositionalPlaceholders:
#if MYSQL_VERSION_ID >= 40108
        return d->preparedQuerysEnabled;
#else
        return false;
#endif
    }
    return false;
}

static void setOptionFlag(uint &optionFlags, const QString &opt)
{
    if (opt == QLatin1String("CLIENT_COMPRESS"))
        optionFlags |= CLIENT_COMPRESS;
    else if (opt == QLatin1String("CLIENT_FOUND_ROWS"))
        optionFlags |= CLIENT_FOUND_ROWS;
    else if (opt == QLatin1String("CLIENT_IGNORE_SPACE"))
        optionFlags |= CLIENT_IGNORE_SPACE;
    else if (opt == QLatin1String("CLIENT_INTERACTIVE"))
        optionFlags |= CLIENT_INTERACTIVE;
    else if (opt == QLatin1String("CLIENT_NO_SCHEMA"))
        optionFlags |= CLIENT_NO_SCHEMA;
    else if (opt == QLatin1String("CLIENT_ODBC"))
        optionFlags |= CLIENT_ODBC;
    else if (opt == QLatin1String("CLIENT_SSL"))
        optionFlags |= CLIENT_SSL;
    else
        qWarning("QMYSQLEmbeddedDriver::open: Unknown connect option '%s'", opt.toLocal8Bit().constData());
}

bool QMYSQLEmbeddedDriver::open(const QString& db,
                         const QString&,
                         const QString&,
                         const QString&,
                         int,
                         const QString& connOpts)
{
    if (isOpen())
        close();

    /* This is a hack to get MySQL's stored procedure support working.
       Since a stored procedure _may_ return multiple result sets,
       we have to enable CLIEN_MULTI_STATEMENTS here, otherwise _any_
       stored procedure call will fail.
    */
    unsigned int optionFlags = Q_CLIENT_MULTI_STATEMENTS;
    const QStringList opts(connOpts.split(QLatin1Char(';'), QString::SkipEmptyParts));

    QString unixSocket;
    // extract the real options from the string
    for (int i = 0; i < opts.count(); ++i) {
        QString tmp(opts.at(i).simplified());
        int idx;
        if ((idx = tmp.indexOf(QLatin1Char('='))) != -1) {
            QString val = tmp.mid(idx + 1).simplified();
            QString opt = tmp.left(idx).simplified();
            if (opt == QLatin1String("SERVER_DATADIR")) {
                d->commandLineArgs.append( QString( "--datadir=%1" ).arg( val ) );
            } else if (opt == QLatin1String("UNIX_SOCKET"))
                unixSocket = val;
            else if (val == QLatin1String("TRUE") || val == QLatin1String("1"))
                setOptionFlag(optionFlags, tmp.left(idx).simplified());
            else
                qWarning("QMYSQLEmbeddedDriver::open: Illegal connect option value '%s'",
                         tmp.toLocal8Bit().constData());
        } else {
            setOptionFlag(optionFlags, tmp);
        }
    }

    /* We initialize the server here to allow to pass the command line
       arguments for the embedded server via the connection options.
     */
    qServerInit();

    if ( !d->mysql )
        d->mysql = mysql_init((MYSQL*) 0);
    if ( d->mysql ) {
        mysql_options(d->mysql, MYSQL_OPT_USE_EMBEDDED_CONNECTION, 0);
        mysql_options(d->mysql, MYSQL_READ_DEFAULT_GROUP, "libmysql_client");
        mysql_real_connect(d->mysql, 0, 0, 0,
                           db.isNull() ? static_cast<const char *>(0)
                                       : db.toLocal8Bit().constData(),
                           0, 0, 0);
    } else {
        setLastError(qMakeError(tr("Unable to connect"),
                     QSqlError::ConnectionError, d));
        mysql_close(d->mysql);
        setOpenError(true);
        return false;
    }
#if MYSQL_VERSION_ID >= 50007
    // force the communication to be utf8
    mysql_set_character_set(d->mysql, "utf8");
#endif
    d->tc = codec(d->mysql);

#if MYSQL_VERSION_ID >= 40108
    d->preparedQuerysEnabled = mysql_get_client_version() >= 40108
                        && mysql_get_server_version(d->mysql) >= 40100;
#else
    d->preparedQuerysEnabled = false;
#endif

    setOpen(true);
    setOpenError(false);
    return true;
}

void QMYSQLEmbeddedDriver::close()
{
    if (isOpen()) {
        if ( QCoreApplication::instance()->thread() == QThread::currentThread() )
            mysql_close(d->mysql);
        setOpen(false);
        setOpenError(false);
    }
}

QSqlResult *QMYSQLEmbeddedDriver::createResult() const
{
    return new QMYSQLEmbeddedResult(this);
}

QStringList QMYSQLEmbeddedDriver::tables(QSql::TableType type) const
{
    QStringList tl;
    if (!isOpen())
        return tl;
    if (!(type & QSql::Tables))
        return tl;

    MYSQL_RES* tableRes = mysql_list_tables(d->mysql, NULL);
    MYSQL_ROW row;
    int i = 0;
    while (tableRes) {
        mysql_data_seek(tableRes, i);
        row = mysql_fetch_row(tableRes);
        if (!row)
            break;
        tl.append(d->tc->toUnicode(row[0]));
        i++;
    }
    mysql_free_result(tableRes);
    return tl;
}

QSqlIndex QMYSQLEmbeddedDriver::primaryIndex(const QString& tablename) const
{
    QSqlIndex idx;
    bool prepQ;
    if (!isOpen())
        return idx;

    prepQ = d->preparedQuerys;
    d->preparedQuerys = false;

    QSqlQuery i(createResult());
    QString stmt(QLatin1String("show index from %1;"));
    QSqlRecord fil = record(tablename);
    i.exec(stmt.arg(tablename));
    while (i.isActive() && i.next()) {
        if (i.value(2).toString() == QLatin1String("PRIMARY")) {
            idx.append(fil.field(i.value(4).toString()));
            idx.setCursorName(i.value(0).toString());
            idx.setName(i.value(2).toString());
        }
    }

    d->preparedQuerys = prepQ;
    return idx;
}

QSqlRecord QMYSQLEmbeddedDriver::record(const QString& tablename) const
{
    QSqlRecord info;
    if (!isOpen())
        return info;
    MYSQL_RES* r = mysql_list_fields(d->mysql, tablename.toLocal8Bit().constData(), 0);
    if (!r) {
        return info;
    }
    MYSQL_FIELD* field;
    while ((field = mysql_fetch_field(r)))
        info.append(qToField(field, d->tc));
    mysql_free_result(r);
    return info;
}

QVariant QMYSQLEmbeddedDriver::handle() const
{
    return qVariantFromValue(d->mysql);
}

bool QMYSQLEmbeddedDriver::beginTransaction()
{
#ifndef CLIENT_TRANSACTIONS
    return false;
#endif
    if (!isOpen()) {
        qWarning("QMYSQLEmbeddedDriver::beginTransaction: Database not open");
        return false;
    }
    if (mysql_query(d->mysql, "BEGIN WORK")) {
        setLastError(qMakeError(tr("Unable to begin transaction"),
                                QSqlError::StatementError, d));
        return false;
    }
    return true;
}

bool QMYSQLEmbeddedDriver::commitTransaction()
{
#ifndef CLIENT_TRANSACTIONS
    return false;
#endif
    if (!isOpen()) {
        qWarning("QMYSQLEmbeddedDriver::commitTransaction: Database not open");
        return false;
    }
    if (mysql_query(d->mysql, "COMMIT")) {
        setLastError(qMakeError(tr("Unable to commit transaction"),
                                QSqlError::StatementError, d));
        return false;
    }
    return true;
}

bool QMYSQLEmbeddedDriver::rollbackTransaction()
{
#ifndef CLIENT_TRANSACTIONS
    return false;
#endif
    if (!isOpen()) {
        qWarning("QMYSQLEmbeddedDriver::rollbackTransaction: Database not open");
        return false;
    }
    if (mysql_query(d->mysql, "ROLLBACK")) {
        setLastError(qMakeError(tr("Unable to rollback transaction"),
                                QSqlError::StatementError, d));
        return false;
    }
    return true;
}

QString QMYSQLEmbeddedDriver::formatValue(const QSqlField &field, bool trimStrings) const
{
    QString r;
    if (field.isNull()) {
        r = QLatin1String("NULL");
    } else {
        switch(field.type()) {
        case QVariant::String:
            // Escape '\' characters
            r = QSqlDriver::formatValue(field, trimStrings);
            r.replace(QLatin1String("\\"), QLatin1String("\\\\"));
            break;
        case QVariant::ByteArray:
            if (isOpen()) {
                const QByteArray ba = field.value().toByteArray();
                // buffer has to be at least length*2+1 bytes
                char* buffer = new char[ba.size() * 2 + 1];
                int escapedSize = int(mysql_real_escape_string(d->mysql, buffer,
                                      ba.data(), ba.size()));
                r.reserve(escapedSize + 3);
                r.append(QLatin1Char('\'')).append(d->tc->toUnicode(buffer)).append(QLatin1Char('\''));
                delete[] buffer;
                break;
            } else {
                qWarning("QMYSQLEmbeddedDriver::formatValue: Database not open");
            }
            // fall through
        default:
            r = QSqlDriver::formatValue(field, trimStrings);
        }
    }
    return r;
}
