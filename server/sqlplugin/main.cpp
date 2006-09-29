/****************************************************************************
**
** Copyright (C) 1992-2006 Trolltech ASA. All rights reserved.
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <qsqldriverplugin.h>
#include <qstringlist.h>
#include "qsql_mysql_embedded.h"

class QMYSQLEmbeddedDriverPlugin : public QSqlDriverPlugin
{
public:
    QMYSQLEmbeddedDriverPlugin();

    QSqlDriver* create(const QString &);
    QStringList keys() const;
};

QMYSQLEmbeddedDriverPlugin::QMYSQLEmbeddedDriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver* QMYSQLEmbeddedDriverPlugin::create(const QString &name)
{
    if (name == QLatin1String("QMYSQL_EMBEDDED")) {
        QMYSQLEmbeddedDriver* driver = new QMYSQLEmbeddedDriver();
        return driver;
    }
    return 0;
}

QStringList QMYSQLEmbeddedDriverPlugin::keys() const
{
    QStringList l;
    l << QLatin1String("QMYSQL_EMBEDDED");
    return l;
}

Q_EXPORT_STATIC_PLUGIN(QMYSQLEmbeddedDriverPlugin)
Q_EXPORT_PLUGIN2(qsqlmysqlembedded, QMYSQLEmbeddedDriverPlugin)
