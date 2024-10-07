/*
    SPDX-FileCopyrightText: 2014-2024 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "manageaccountwidget.h"

#include "agentconfigurationdialog.h"
#include "agentfilterproxymodel.h"
#include "agentinstance.h"
#include "agentinstancecreatejob.h"
#include "agentinstancefilterproxymodel.h"
#include "agentmanager.h"
#include "agenttypedialog.h"

#include "ui_manageaccountwidget.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <QAbstractItemDelegate>
#include <QAbstractItemView>
#include <QKeyEvent>
#include <QPointer>

using namespace Akonadi;

class Akonadi::ManageAccountWidgetPrivate
{
public:
    QString mSpecialCollectionIdentifier;

    QStringList mMimeTypeFilter;
    QStringList mCapabilityFilter;
    QStringList mExcludeCapabilities;

    Ui::ManageAccountWidget ui;
};

ManageAccountWidget::ManageAccountWidget(QWidget *parent)
    : QWidget(parent)
    , d(new Akonadi::ManageAccountWidgetPrivate)
{
    d->ui.setupUi(this);
    connect(d->ui.mAddAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotAddAccount);

    connect(d->ui.mModifyAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotModifySelectedAccount);

    connect(d->ui.mRemoveAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotRemoveSelectedAccount);
    connect(d->ui.mRestartAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotRestartSelectedAccount);

    connect(d->ui.mAccountList, &Akonadi::AgentInstanceWidget::clicked, this, &ManageAccountWidget::slotAccountSelected);
    connect(d->ui.mAccountList, &Akonadi::AgentInstanceWidget::doubleClicked, this, &ManageAccountWidget::slotModifySelectedAccount);

    d->ui.mAccountList->view()->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(d->ui.mFilterAccount, &QLineEdit::textChanged, this, &ManageAccountWidget::slotSearchAgentType);

    d->ui.mFilterAccount->installEventFilter(this);
    d->ui.accountOnCurrentActivity->setVisible(false);
    slotAccountSelected(d->ui.mAccountList->currentAgentInstance());
}

ManageAccountWidget::~ManageAccountWidget() = default;

void ManageAccountWidget::slotSearchAgentType(const QString &str)
{
    d->ui.mAccountList->agentFilterProxyModel()->setFilterRegularExpression(str);
}

void ManageAccountWidget::disconnectAddAccountButton()
{
    disconnect(d->ui.mAddAccountButton, &QPushButton::clicked, this, &ManageAccountWidget::slotAddAccount);
}

bool ManageAccountWidget::enablePlasmaActivities() const
{
    return d->ui.mAccountList->enablePlasmaActivities();
}

void ManageAccountWidget::setEnablePlasmaActivities(bool newEnablePlasmaActivities)
{
    d->ui.accountOnCurrentActivity->setVisible(newEnablePlasmaActivities);
    d->ui.mAccountList->setEnablePlasmaActivities(newEnablePlasmaActivities);
}

AccountActivitiesAbstract *ManageAccountWidget::accountActivitiesAbstract() const
{
    return d->ui.mAccountList->accountActivitiesAbstract();
}

void ManageAccountWidget::setAccountActivitiesAbstract(AccountActivitiesAbstract *abstract)
{
    d->ui.mAccountList->setAccountActivitiesAbstract(abstract);
}

QPushButton *ManageAccountWidget::addAccountButton() const
{
    return d->ui.mAddAccountButton;
}

void ManageAccountWidget::setDescriptionLabelText(const QString &text)
{
    d->ui.label->setText(text);
}

bool ManageAccountWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && obj == d->ui.mFilterAccount) {
        auto key = static_cast<QKeyEvent *>(event);
        if ((key->key() == Qt::Key_Enter) || (key->key() == Qt::Key_Return)) {
            event->accept();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

QAbstractItemView *ManageAccountWidget::view() const
{
    return d->ui.mAccountList->view();
}

void ManageAccountWidget::setSpecialCollectionIdentifier(const QString &identifier)
{
    d->mSpecialCollectionIdentifier = identifier;
}

void ManageAccountWidget::slotAddAccount()
{
    Akonadi::AgentTypeDialog dlg(this);

    Akonadi::AgentFilterProxyModel *filter = dlg.agentFilterProxyModel();
    for (const QString &filterStr : std::as_const(d->mMimeTypeFilter)) {
        filter->addMimeTypeFilter(filterStr);
    }
    for (const QString &capa : std::as_const(d->mCapabilityFilter)) {
        filter->addCapabilityFilter(capa);
    }
    for (const QString &capa : std::as_const(d->mExcludeCapabilities)) {
        filter->excludeCapabilities(capa);
    }
    if (dlg.exec()) {
        const Akonadi::AgentType agentType = dlg.agentType();

        if (agentType.isValid()) {
            auto job = new Akonadi::AgentInstanceCreateJob(agentType, this);
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
    for (const QString &capability : std::as_const(d->mExcludeCapabilities)) {
        d->ui.mAccountList->agentFilterProxyModel()->excludeCapabilities(capability);
    }
}

void ManageAccountWidget::setItemDelegate(QAbstractItemDelegate *delegate)
{
    d->ui.mAccountList->view()->setItemDelegate(delegate);
}

QStringList ManageAccountWidget::capabilityFilter() const
{
    return d->mCapabilityFilter;
}

void ManageAccountWidget::setCapabilityFilter(const QStringList &capabilityFilter)
{
    d->mCapabilityFilter = capabilityFilter;
    for (const QString &capability : std::as_const(d->mCapabilityFilter)) {
        d->ui.mAccountList->agentFilterProxyModel()->addCapabilityFilter(capability);
    }
}

QStringList ManageAccountWidget::mimeTypeFilter() const
{
    return d->mMimeTypeFilter;
}

void ManageAccountWidget::setMimeTypeFilter(const QStringList &mimeTypeFilter)
{
    d->mMimeTypeFilter = mimeTypeFilter;
    for (const QString &mimeType : std::as_const(d->mMimeTypeFilter)) {
        d->ui.mAccountList->agentFilterProxyModel()->addMimeTypeFilter(mimeType);
    }
}

void ManageAccountWidget::slotModifySelectedAccount()
{
    Akonadi::AgentInstance instance = d->ui.mAccountList->currentAgentInstance();
    if (instance.isValid()) {
        QPointer<AgentConfigurationDialog> dlg(new AgentConfigurationDialog(instance, this));
        dlg->exec();
        delete dlg;
    }
}

void ManageAccountWidget::slotRestartSelectedAccount()
{
    const Akonadi::AgentInstance instance = d->ui.mAccountList->currentAgentInstance();
    if (instance.isValid()) {
        instance.restart();
    }
}

void ManageAccountWidget::slotRemoveSelectedAccount()
{
    const Akonadi::AgentInstance instance = d->ui.mAccountList->currentAgentInstance();

    const int rc = KMessageBox::questionTwoActions(this,
                                                   i18n("Do you want to remove account '%1'?", instance.name()),
                                                   i18nc("@title:window", "Remove account?"),
                                                   KStandardGuiItem::remove(),
                                                   KStandardGuiItem::cancel());
    if (rc == KMessageBox::ButtonCode::SecondaryAction) {
        return;
    }

    if (instance.isValid()) {
        Akonadi::AgentManager::self()->removeInstance(instance);
    }

    slotAccountSelected(d->ui.mAccountList->currentAgentInstance());
}

void ManageAccountWidget::slotAccountSelected(const Akonadi::AgentInstance &current)
{
    if (current.isValid()) {
        d->ui.mModifyAccountButton->setEnabled(!current.type().capabilities().contains(QLatin1StringView("NoConfig")));
        d->ui.mRemoveAccountButton->setEnabled(d->mSpecialCollectionIdentifier != current.identifier());
        // Restarting an agent is not possible if it's in Running status... (see AgentProcessInstance::restartWhenIdle)
        d->ui.mRestartAccountButton->setEnabled((current.status() != 1));
    } else {
        d->ui.mModifyAccountButton->setEnabled(false);
        d->ui.mRemoveAccountButton->setEnabled(false);
        d->ui.mRestartAccountButton->setEnabled(false);
    }
}

#include "moc_manageaccountwidget.cpp"
