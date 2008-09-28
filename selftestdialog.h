/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SELFTESTDIALOG_H
#define AKONADI_SELFTESTDIALOG_H

#include "akonadiprivate_export.h"
#include "ui_selftestdialog.h"

#include <KDialog>

class QStandardItem;
class QStandardItemModel;

namespace Akonadi {

/**
 * @internal
 */
class AKONADI_TESTS_EXPORT SelfTestDialog : public KDialog
{
  Q_OBJECT
  public:
    SelfTestDialog( QWidget *parent = 0 );

  private:
    enum ResultType {
      Skip,
      Success,
      Warning,
      Error
    };
    QStandardItem* report( ResultType type, const QString &summary, const QString &details );
    void runTests();
    QVariant serverSetting( const QString &group, const QString &key, const QVariant &def ) const;
    bool useStandaloneMysqlServer() const;
    bool runProcess( const QString &app, const QStringList &args, QString &result ) const;

    void testSQLDriver();
    void testMySQLServer();
    void testMySQLServerLog();
    void testMySQLServerConfig();
    void testAkonadiCtl();
    void testServerStatus();
    void testProtocolVersion();
    void testResources();
    void testServerLog();
    void testControlLog();

    QString createReport();

  private slots:
    void selectionChanged( const QModelIndex &index );
    void saveReport();
    void copyReport();
    void linkActivated( const QString &link );

  private:
    Ui::SelfTestDialog ui;
    QStandardItemModel* mTestModel;
};

}

#endif
