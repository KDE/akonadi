/*
  Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "agentactionmanager.h"

#include "agentfilterproxymodel.h"
#include "agentinstancecreatejob.h"
#include "agentinstancemodel.h"
#include "agentmanager.h"
#include "agenttypedialog.h"
#include "metatypes.h"

#include <QAction>
#include <QIcon>
#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>

#include <QItemSelectionModel>
#include <QPointer>

#include <boost/static_assert.hpp>

using namespace Akonadi;

//@cond PRIVATE

static const struct {
    const char *name;
    const char *label;
    const char *icon;
    int shortcut;
    const char *slot;
} agentActionData[] = {
    {   "akonadi_agentinstance_create", I18N_NOOP("&New Agent Instance..."),
        "folder-new", 0, SLOT(slotCreateAgentInstance())
    },
    {   "akonadi_agentinstance_delete", I18N_NOOP("&Delete Agent Instance"),
        "edit-delete", 0, SLOT(slotDeleteAgentInstance())
    },
    {   "akonadi_agentinstance_configure", I18N_NOOP("&Configure Agent Instance"),
        "configure", 0, SLOT(slotConfigureAgentInstance())
    }
};
static const int numAgentActionData = sizeof agentActionData / sizeof * agentActionData;

BOOST_STATIC_ASSERT(numAgentActionData == AgentActionManager::LastType);

/**
 * @internal
 */
class AgentActionManager::Private
{
public:
    Private(AgentActionManager *parent)
        : q(parent)
        , mSelectionModel(0)
    {
        mActions.fill(0, AgentActionManager::LastType);

        setContextText(AgentActionManager::CreateAgentInstance,
                       AgentActionManager::DialogTitle,
                       i18nc("@title:window", "New Agent Instance"));

        setContextText(AgentActionManager::CreateAgentInstance,
                       AgentActionManager::ErrorMessageText,
                       ki18n("Could not create agent instance: %1"));

        setContextText(AgentActionManager::CreateAgentInstance,
                       AgentActionManager::ErrorMessageTitle,
                       i18n("Agent instance creation failed"));

        setContextText(AgentActionManager::DeleteAgentInstance,
                       AgentActionManager::MessageBoxTitle,
                       i18nc("@title:window", "Delete Agent Instance?"));

        setContextText(AgentActionManager::DeleteAgentInstance,
                       AgentActionManager::MessageBoxText,
                       i18n("Do you really want to delete the selected agent instance?"));
    }

    void enableAction(AgentActionManager::Type type, bool enable)
    {
        Q_ASSERT(type >= 0 && type < AgentActionManager::LastType);
        if (mActions[type]) {
            mActions[type]->setEnabled(enable);
        }
    }

    void updateActions()
    {
        const AgentInstance::List instances = selectedAgentInstances();

        const bool createActionEnabled = true;
        bool deleteActionEnabled = true;
        bool configureActionEnabled = true;

        if (instances.isEmpty()) {
            deleteActionEnabled = false;
            configureActionEnabled = false;
        }

        if (instances.count() == 1) {
            const AgentInstance instance = instances.first();
            if (instance.type().capabilities().contains(QStringLiteral("NoConfig"))) {
                configureActionEnabled = false;
            }
        }

        enableAction(CreateAgentInstance, createActionEnabled);
        enableAction(DeleteAgentInstance, deleteActionEnabled);
        enableAction(ConfigureAgentInstance, configureActionEnabled);

        emit q->actionStateUpdated();
    }

    AgentInstance::List selectedAgentInstances() const
    {
        AgentInstance::List instances;

        if (!mSelectionModel) {
            return instances;
        }

        foreach (const QModelIndex &index, mSelectionModel->selectedRows()) {
            const AgentInstance instance =
                index.data(AgentInstanceModel::InstanceRole).value<AgentInstance>();
            if (instance.isValid()) {
                instances << instance;
            }
        }

        return instances;
    }

    void slotCreateAgentInstance()
    {
        QPointer<Akonadi::AgentTypeDialog> dlg(new Akonadi::AgentTypeDialog(mParentWidget));
        dlg->setCaption(contextText(AgentActionManager::CreateAgentInstance,
                                    AgentActionManager::DialogTitle));

        foreach (const QString &mimeType, mMimeTypeFilter) {
            dlg->agentFilterProxyModel()->addMimeTypeFilter(mimeType);
        }

        foreach (const QString &capability, mCapabilityFilter) {
            dlg->agentFilterProxyModel()->addCapabilityFilter(capability);
        }

        if (dlg->exec() == QDialog::Accepted && dlg != 0) {
            const AgentType agentType = dlg->agentType();

            if (agentType.isValid()) {
                AgentInstanceCreateJob *job = new AgentInstanceCreateJob(agentType, q);
                q->connect(job, SIGNAL(result(KJob*)), SLOT(slotAgentInstanceCreationResult(KJob*)));
                job->configure(mParentWidget);
                job->start();
            }
        }
        delete dlg;
    }

    void slotDeleteAgentInstance()
    {
        const AgentInstance::List instances = selectedAgentInstances();
        if (!instances.isEmpty()) {
            if (KMessageBox::questionYesNo(
                    mParentWidget,
                    contextText(AgentActionManager::DeleteAgentInstance,
                                AgentActionManager::MessageBoxText),
                    contextText(AgentActionManager::DeleteAgentInstance,
                                AgentActionManager::MessageBoxTitle),
                    KStandardGuiItem::del(),
                    KStandardGuiItem::cancel(),
                    QString(),
                    KMessageBox::Dangerous) == KMessageBox::Yes) {

                foreach (const AgentInstance &instance, instances) {
                    AgentManager::self()->removeInstance(instance);
                }
            }
        }
    }

    void slotConfigureAgentInstance()
    {
        AgentInstance::List instances = selectedAgentInstances();
        if (instances.isEmpty()) {
            return;
        }

        instances.first().configure(mParentWidget);
    }

    void slotAgentInstanceCreationResult(KJob *job)
    {
        if (job->error()) {
            KMessageBox::error(
                mParentWidget,
                contextText(AgentActionManager::CreateAgentInstance,
                            AgentActionManager::ErrorMessageText).arg(job->errorString()),
                contextText(AgentActionManager::CreateAgentInstance,
                            AgentActionManager::ErrorMessageTitle));
        }
    }

    void setContextText(AgentActionManager::Type type,
                        AgentActionManager::TextContext context, const QString &data)
    {
        mContextTexts[type].insert(context, data);
    }

    void setContextText(AgentActionManager::Type type,
                        AgentActionManager::TextContext context, const KLocalizedString &data)
    {

        mContextTexts[type].insert(context, data.toString());
    }

    QString contextText(AgentActionManager::Type type,
                        AgentActionManager::TextContext context) const
    {
        return mContextTexts[type].value(context);
    }

    AgentActionManager *q;
    KActionCollection *mActionCollection;
    QWidget *mParentWidget;
    QItemSelectionModel *mSelectionModel;
    QVector<QAction *> mActions;
    QStringList mMimeTypeFilter;
    QStringList mCapabilityFilter;

    typedef QHash<AgentActionManager::TextContext, QString> ContextTexts;
    QHash<AgentActionManager::Type, ContextTexts> mContextTexts;
};

//@endcond

AgentActionManager::AgentActionManager(KActionCollection *actionCollection, QWidget *parent)
    : QObject(parent)
    , d(new Private(this))
{
    d->mParentWidget = parent;
    d->mActionCollection = actionCollection;
}

AgentActionManager::~AgentActionManager()
{
    delete d;
}

void AgentActionManager::setSelectionModel(QItemSelectionModel *selectionModel)
{
    d->mSelectionModel = selectionModel;
    connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(updateActions()));
}

void AgentActionManager::setMimeTypeFilter(const QStringList &mimeTypes)
{
    d->mMimeTypeFilter = mimeTypes;
}

void AgentActionManager::setCapabilityFilter(const QStringList &capabilities)
{
    d->mCapabilityFilter = capabilities;
}

QAction *AgentActionManager::createAction(Type type)
{
    Q_ASSERT(type >= 0 && type < LastType);
    Q_ASSERT(agentActionData[type].name);
    if (d->mActions[type]) {
        return d->mActions[type];
    }

    QAction *action = new QAction(d->mParentWidget);
    action->setText(i18n(agentActionData[type].label));

    if (agentActionData[type].icon) {
        action->setIcon(QIcon::fromTheme(QString::fromLatin1(agentActionData[type].icon)));
    }

    action->setShortcut(agentActionData[type].shortcut);

    if (agentActionData[type].slot) {
        connect(action, SIGNAL(triggered()), agentActionData[type].slot);
    }

    d->mActionCollection->addAction(QString::fromLatin1(agentActionData[type].name), action);
    d->mActions[type] = action;
    d->updateActions();

    return action;
}

void AgentActionManager::createAllActions()
{
    for (int type = 0; type < LastType; ++type) {
        createAction((Type)type);
    }
}

QAction *AgentActionManager::action(Type type) const
{
    Q_ASSERT(type >= 0 && type < LastType);
    return d->mActions[type];
}

void AgentActionManager::interceptAction(Type type, bool intercept)
{
    Q_ASSERT(type >= 0 && type < LastType);

    const QAction *action = d->mActions[type];

    if (!action) {
        return;
    }

    if (intercept) {
        disconnect(action, SIGNAL(triggered()), this, agentActionData[type].slot);
    } else {
        connect(action, SIGNAL(triggered()), agentActionData[type].slot);
    }
}

AgentInstance::List AgentActionManager::selectedAgentInstances() const
{
    return d->selectedAgentInstances();
}

void AgentActionManager::setContextText(Type type, TextContext context, const QString &text)
{
    d->setContextText(type, context, text);
}

void AgentActionManager::setContextText(Type type, TextContext context,
                                        const KLocalizedString &text)
{
    d->setContextText(type, context, text);
}

#include "moc_agentactionmanager.cpp"
