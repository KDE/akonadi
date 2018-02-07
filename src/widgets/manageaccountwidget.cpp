/*
    Copyright (c) 2014-2018 Montel Laurent <montel@kde.org>

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

#include "manageaccountwidget.h"

#include "agentinstance.h"
#include "agentinstancecreatejob.h"
#include "agenttypedialog.h"
#include "agentfilterproxymodel.h"
#include "agentmanager.h"

#include "ui_manageaccountwidget.h"

#include <KMessageBox>
#include <KLocalizedString>
#include <KWindowSystem>
#include <QKeyEvent>
#include <QAbstractItemDelegate>

using namespace Akonadi;

class Akonadi::ManageAccountWidgetPrivate
{
public:
    ManageAccountWidgetPrivate()
    {

    }
    ~ManageAccountWidgetPrivate()
    {
        delete ui;
    }

    QString mSpecialCollectionIdentifier;

    QStringList mMimeTypeFilter;
    QStringList mCapabilityFilter;
    QStringList mExcludeCapabilities;

    Ui::ManageAccountWidget *ui = nullptr;
};

ManageAccountWidget::ManageAccountWidget(QWidget *parent)
    : QWidget(parent),
      d(new Akonadi::ManageAccountWidgetPrivate)
{
    d->ui = new Ui::ManageAccountWidget;
    d->ui->setupUi(this);
    connect(d->ui->mAddAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotAddAccount);

    connect(d->ui->mModifyAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotModifySelectedAccount);

    connect(d->ui->mRemoveAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotRemoveSelectedAccount);
    connect(d->ui->mRestartAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotRestartSelectedAccount);

    connect(d->ui->mAccountList, &Akonadi::AgentInstanceWidget::clicked, this, &ManageAccountWidget::slotAccountSelected);
    connect(d->ui->mAccountList, &Akonadi::AgentInstanceWidget::doubleClicked, this, &ManageAccountWidget::slotModifySelectedAccount);

    d->ui->mAccountList->view()->setSelectionMode(QAbstractItemView::SingleSelection);

    d->ui->mFilterAccount->setProxy(d->ui->mAccountList->agentFilterProxyModel());
    d->ui->mFilterAccount->installEventFilter(this);
    slotAccountSelected(d->ui->mAccountList->currentAgentInstance());
}

ManageAccountWidget::~ManageAccountWidget()
{
    delete d;
}

void ManageAccountWidget::disconnectAddAccountButton()
{
    disconnect(d->ui->mAddAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotAddAccount);
}

QPushButton *ManageAccountWidget::addAccountButton() const
{
    return d->ui->mAddAccountButton;
}

void ManageAccountWidget::setDescriptionLabelText(const QString &text)
{
    d->ui->label->setText(text);
}

bool ManageAccountWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && obj == d->ui->mFilterAccount) {
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
        if ((key->key() == Qt::Key_Enter) || (key->key() == Qt::Key_Return)) {
            event->accept();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

QAbstractItemView *ManageAccountWidget::view() const
{
    return d->ui->mAccountList->view();
}

void ManageAccountWidget::setSpecialCollectionIdentifier(const QString &identifier)
{
    d->mSpecialCollectionIdentifier = identifier;
}

void ManageAccountWidget::slotAddAccount()
{
    Akonadi::AgentTypeDialog dlg(this);

    Akonadi::AgentFilterProxyModel *filter = dlg.agentFilterProxyModel();
    for (const QString &filterStr : qAsConst(d->mMimeTypeFilter)) {
        filter->addMimeTypeFilter(filterStr);
    }
    for (const QString &capa : qAsConst(d->mCapabilityFilter)) {
        filter->addCapabilityFilter(capa);
    }
    for (const QString &capa : qAsConst(d->mExcludeCapabilities)) {
        filter->excludeCapabilities(capa);
    }
    if (dlg.exec()) {
        const Akonadi::AgentType agentType = dlg.agentType();

        if (agentType.isValid()) {

            Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob(agentType, this);
            job->configure(this);
            job->start();
        }
    }
}

QStringList ManageAccountWidget::excludeCapabilities() const
{
    return d->mExcludeCapabilities;
}

void ManageAccountWidget::setExcludeCapabilities(const QStringList &excludeCapabilities)
{
    d->mExcludeCapabilities = excludeCapabilities;
}

void ManageAccountWidget::setItemDelegate(QAbstractItemDelegate *delegate)
{
    d->ui->mAccountList->view()->setItemDelegate(delegate);
}

QStringList ManageAccountWidget::capabilityFilter() const
{
    return d->mCapabilityFilter;
}

void ManageAccountWidget::setCapabilityFilter(const QStringList &capabilityFilter)
{
    d->mCapabilityFilter = capabilityFilter;
    for (const QString &capability : qAsConst(d->mCapabilityFilter)) {
        d->ui->mAccountList->agentFilterProxyModel()->addCapabilityFilter(capability);
    }
}

QStringList ManageAccountWidget::mimeTypeFilter() const
{
    return d->mMimeTypeFilter;
}

void ManageAccountWidget::setMimeTypeFilter(const QStringList &mimeTypeFilter)
{
    d->mMimeTypeFilter = mimeTypeFilter;
    for (const QString &mimeType : qAsConst(d->mMimeTypeFilter)) {
        d->ui->mAccountList->agentFilterProxyModel()->addMimeTypeFilter(mimeType);
    }
}

void ManageAccountWidget::slotModifySelectedAccount()
{
    Akonadi::AgentInstance instance = d->ui->mAccountList->currentAgentInstance();
    if (instance.isValid()) {
        KWindowSystem::allowExternalProcessWindowActivation();
        instance.configure(this);
    }
}

void ManageAccountWidget::slotRestartSelectedAccount()
{
    const Akonadi::AgentInstance instance =  d->ui->mAccountList->currentAgentInstance();
    if (instance.isValid()) {
        instance.restart();
    }
}

void ManageAccountWidget::slotRemoveSelectedAccount()
{
    const Akonadi::AgentInstance instance =  d->ui->mAccountList->currentAgentInstance();

    const int rc = KMessageBox::questionYesNo(this,
                   i18n("Do you want to remove account '%1'?", instance.name()),
                   i18n("Remove account?"));
    if (rc == KMessageBox::No) {
        return;
    }

    if (instance.isValid()) {
        Akonadi::AgentManager::self()->removeInstance(instance);
    }

    slotAccountSelected(d->ui->mAccountList->currentAgentInstance());

}

void ManageAccountWidget::slotAccountSelected(const Akonadi::AgentInstance &current)
{
    if (current.isValid()) {
        d->ui->mModifyAccountButton->setEnabled(!current.type().capabilities().contains(QLatin1String("NoConfig")));
        d->ui->mRemoveAccountButton->setEnabled(d->mSpecialCollectionIdentifier != current.identifier());
        // Restarting an agent is not possible if it's in Running status... (see AgentProcessInstance::restartWhenIdle)
        d->ui->mRestartAccountButton->setEnabled((current.status() != 1));
    } else {
        d->ui->mModifyAccountButton->setEnabled(false);
        d->ui->mRemoveAccountButton->setEnabled(false);
        d->ui->mRestartAccountButton->setEnabled(false);
    }
}
