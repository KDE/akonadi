/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    Inspired by kdelibs/kdecore/io/kdebug.h
    Copyright (C) 1997 Matthias Kalle Dalheimer (kalle@kde.org)
                  2002 Holger Freyther (freyther@kde.org)

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "akdebug.h"

#include <private/standarddirs_p.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QLoggingCategory>

#include <KCrash/KCrash>

#include <cassert>

class FileDebugStream : public QIODevice
{
    Q_OBJECT
public:
    FileDebugStream()
        : mType(QtCriticalMsg)
    {
        open(WriteOnly);
    }

    bool isSequential() const Q_DECL_OVERRIDE
    {
        return true;
    }
    qint64 readData(char *, qint64) Q_DECL_OVERRIDE
    {
        return 0;
    }
    qint64 readLineData(char *, qint64) Q_DECL_OVERRIDE
    {
        return 0;
    }

    qint64 writeData(const char *data, qint64 len) Q_DECL_OVERRIDE
    {
        if (!mFileName.isEmpty()) {
            QFile outputFile(mFileName);
            outputFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Unbuffered);
            outputFile.write(data, len);
            outputFile.putChar('\n');
            outputFile.close();
        }

        return len;
    }

    void setFileName(const QString &fileName)
    {
        mFileName = fileName;
    }

    void setType(QtMsgType type)
    {
        mType = type;
    }
private:
    QString mFileName;
    QtMsgType mType;
};

class DebugPrivate
{
public:
    DebugPrivate()
        : origHandler(nullptr)
    {
    }

    ~DebugPrivate()
    {
        file.close();
    }

    QString errorLogFileName() const
    {
        return Akonadi::StandardDirs::saveDir("data")
               + QDir::separator()
               + name
               + QLatin1String(".error");
    }

    void log(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        QMutexLocker locker(&mutex);
#ifdef QT_NO_DEBUG_OUTPUT
        if (type == QtDebugMsg) {
            return;
        }
#endif
        const QByteArray buf = msg.toUtf8();
        file.write(buf.constData(), buf.size());
        file.flush();

        if (origHandler) {
            origHandler(type, context, msg);
        }
    }

    void setName(const QString &appName)
    {
        // Keep only the executable name, e.g. akonadi_control
        name = appName.mid(appName.lastIndexOf(QLatin1Char('/')) + 1);

        if (file.isOpen()) {
            file.close();
        }
        file.setFileName(errorLogFileName());
        file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Unbuffered);
    }

    void setOrigHandler(QtMessageHandler origHandler_)
    {
        origHandler = origHandler_;
    }

    void setOrigCategoryFilter(QLoggingCategory::CategoryFilter origFilter_)
    {
        origFilter = origFilter_;
    }

    QMutex mutex;
    QFile file;
    QString name;
    QtMessageHandler origHandler;
    QLoggingCategory::CategoryFilter origFilter;
    QByteArray loggingCategory;
};

Q_GLOBAL_STATIC(DebugPrivate, sInstance)

void akMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
        sInstance()->log(type, context, msg);
        break;
    case QtFatalMsg:
        sInstance()->log(QtInfoMsg, context, msg);
        abort();
    }
}

void akCategoryFilter(QLoggingCategory *category)
{
    if ((qstrcmp(category->categoryName(), sInstance()->loggingCategory) == 0) ||
        (qstrcmp(category->categoryName(), "org.kde.pim.akonadiprivate") == 0)) {
        category->setEnabled(QtDebugMsg, true);
        category->setEnabled(QtInfoMsg, true);
        category->setEnabled(QtWarningMsg, true);
        category->setEnabled(QtCriticalMsg, true);
        category->setEnabled(QtFatalMsg, true);
    } else if (sInstance()->origFilter) {
        sInstance()->origFilter(category);
    }
}

void akInit(const QString &appName)
{
    KCrash::initialize();
    sInstance()->setName(appName);

    QFileInfo infoOld(sInstance()->errorLogFileName() + QLatin1String(".old"));
    if (infoOld.exists()) {
        QFile fileOld(infoOld.absoluteFilePath());
        const bool success = fileOld.remove();
        if (!success) {
            qFatal("Cannot remove old log file - running on a readonly filesystem maybe?");
        }
    }
    QFileInfo info(sInstance()->errorLogFileName());
    if (info.exists()) {
        QFile file(info.absoluteFilePath());
        const bool success = file.rename(sInstance()->errorLogFileName() + QLatin1String(".old"));
        if (!success) {
            qFatal("Cannot rename log file - running on a readonly filesystem maybe?");
        }
    }

    QtMessageHandler origHandler = qInstallMessageHandler(akMessageHandler);
    sInstance()->setOrigHandler(origHandler);
}

void akMakeVerbose(const QByteArray &category)
{
    sInstance()->loggingCategory = category;
    QLoggingCategory::CategoryFilter oldFilter = QLoggingCategory::installFilter(akCategoryFilter);
    sInstance()->setOrigCategoryFilter(oldFilter);
}

#include "akdebug.moc"

