/*
    Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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
#include <KLineEdit>
#include <QKeyEvent>
#include <QAbstractItemDelegate>

using namespace Akonadi;

class Akonadi::ManageAccountWidgetPrivate
{
public:
    ManageAccountWidgetPrivate()
        : mWidget(Q_NULLPTR)
    {

    }
    ~ManageAccountWidgetPrivate()
    {
        delete mWidget;
    }

    QString mSpecialCollectionIdentifier;

    QStringList mMimeTypeFilter;
    QStringList mCapabilityFilter;
    QStringList mExcludeCapabilities;

    Ui::ManageAccountWidget *mWidget;
};

ManageAccountWidget::ManageAccountWidget(QWidget *parent)
    : QWidget(parent),
      d(new Akonadi::ManageAccountWidgetPrivate)
{
    d->mWidget = new Ui::ManageAccountWidget;
    d->mWidget->setupUi(this);
    connect(d->mWidget->mAddAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotAddAccount);

    connect(d->mWidget->mModifyAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotModifySelectedAccount);

    connect(d->mWidget->mRemoveAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotRemoveSelectedAccount);
    connect(d->mWidget->mRestartAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotRestartSelectedAccount);

    connect(d->mWidget->mAccountList, &Akonadi::AgentInstanceWidget::clicked, this, &ManageAccountWidget::slotAccountSelected);
    connect(d->mWidget->mAccountList, &Akonadi::AgentInstanceWidget::doubleClicked, this, &ManageAccountWidget::slotModifySelectedAccount);

    d->mWidget->mAccountList->view()->setSelectionMode(QAbstractItemView::SingleSelection);

    d->mWidget->mFilterAccount->setProxy(d->mWidget->mAccountList->agentFilterProxyModel());
    d->mWidget->mFilterAccount->installEventFilter(this);
    slotAccountSelected(d->mWidget->mAccountList->currentAgentInstance());
}

ManageAccountWidget::~ManageAccountWidget()
{
    delete d;
}

bool ManageAccountWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && obj == d->mWidget->mFilterAccount) {
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
    return d->mWidget->mAccountList->view();
}

void ManageAccountWidget::setSpecialCollectionIdentifier(const QString &identifier)
{
    d->mSpecialCollectionIdentifier = identifier;
}

void ManageAccountWidget::slotAddAccount()
{
    Akonadi::AgentTypeDialog dlg(this);

    Akonadi::AgentFilterProxyModel *filter = dlg.agentFilterProxyModel();
    Q_FOREACH (const QString &filterStr, d->mMimeTypeFilter) {
        filter->addMimeTypeFilter(filterStr);
    }
    Q_FOREACH (const QString &capa, d->mCapabilityFilter) {
        filter->addCapabilityFilter(capa);
    }
    Q_FOREACH (const QString &capa, d->mExcludeCapabilities) {
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
    d->mWidget->mAccountList->view()->setItemDelegate(delegate);
}

QStringList ManageAccountWidget::capabilityFilter() const
{
    return d->mCapabilityFilter;
}

void ManageAccountWidget::setCapabilityFilter(const QStringList &capabilityFilter)
{
    d->mCapabilityFilter = capabilityFilter;
    Q_FOREACH (const QString &capability, d->mCapabilityFilter) {
        d->mWidget->mAccountList->agentFilterProxyModel()->addCapabilityFilter(capability);
    }
}

QStringList ManageAccountWidget::mimeTypeFilter() const
{
    return d->mMimeTypeFilter;
}

void ManageAccountWidget::setMimeTypeFilter(const QStringList &mimeTypeFilter)
{
    d->mMimeTypeFilter = mimeTypeFilter;
    Q_FOREACH (const QString &mimeType, d->mMimeTypeFilter) {
        d->mWidget->mAccountList->agentFilterProxyModel()->addMimeTypeFilter(mimeType);
    }
}

void ManageAccountWidget::slotModifySelectedAccount()
{
    Akonadi::AgentInstance instance = d->mWidget->mAccountList->currentAgentInstance();
    if (instance.isValid()) {
        KWindowSystem::allowExternalProcessWindowActivation();
        instance.configure(this);
    }
}

void ManageAccountWidget::slotRestartSelectedAccount()
{
    const Akonadi::AgentInstance instance =  d->mWidget->mAccountList->currentAgentInstance();
    if (instance.isValid()) {
        instance.restart();
    }
}

void ManageAccountWidget::slotRemoveSelectedAccount()
{
    const Akonadi::AgentInstance instance =  d->mWidget->mAccountList->currentAgentInstance();

    const int rc = KMessageBox::questionYesNo(this,
                   i18n("Do you want to remove account '%1'?", instance.name()),
                   i18n("Remove account?"));
    if (rc == KMessageBox::No) {
        return;
    }

    if (instance.isValid()) {
        Akonadi::AgentManager::self()->removeInstance(instance);
    }

    slotAccountSelected(d->mWidget->mAccountList->currentAgentInstance());

}

void ManageAccountWidget::slotAccountSelected(const Akonadi::AgentInstance &current)
{
    if (current.isValid()) {
        d->mWidget->mModifyAccountButton->setEnabled(!current.type().capabilities().contains(QStringLiteral("NoConfig")));
        d->mWidget->mRemoveAccountButton->setEnabled(d->mSpecialCollectionIdentifier != current.identifier());
        // Restarting an agent is not possible if it's in Running status... (see AgentProcessInstance::restartWhenIdle)
        d->mWidget->mRestartAccountButton->setEnabled((current.status() != 1));
    } else {
        d->mWidget->mModifyAccountButton->setEnabled(false);
        d->mWidget->mRemoveAccountButton->setEnabled(false);
        d->mWidget->mRestartAccountButton->setEnabled(false);
    }
}
