/*
  SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentactionmanager.h"

#include "agentfilterproxymodel.h"
#include "agentinstancecreatejob.h"
#include "agentinstancemodel.h"
#include "agentmanager.h"
#include "agenttypedialog.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>
#include <QAction>
#include <QIcon>

#include <QItemSelectionModel>
#include <QPointer>
#include <ki18n_version.h>
#if KI18N_VERSION >= QT_VERSION_CHECK(5, 89, 0)
#include <KLazyLocalizedString>
#undef I18N_NOOP
#define I18N_NOOP kli18n
#endif

using namespace Akonadi;

/// @cond PRIVATE

static const struct {
    const char *name;
#if KI18N_VERSION < QT_VERSION_CHECK(5, 89, 0)
    const char *label;
#else
    const KLazyLocalizedString label;
#endif
    const char *icon;
    int shortcut;
    const char *slot;
} agentActionData[] = {{"akonadi_agentinstance_create", I18N_NOOP("&New Agent Instance..."), "folder-new", 0, SLOT(slotCreateAgentInstance())},
                       {"akonadi_agentinstance_delete", I18N_NOOP("&Delete Agent Instance"), "edit-delete", 0, SLOT(slotDeleteAgentInstance())},
                       {"akonadi_agentinstance_configure", I18N_NOOP("&Configure Agent Instance"), "configure", 0, SLOT(slotConfigureAgentInstance())}};
static const int numAgentActionData = sizeof agentActionData / sizeof *agentActionData;

static_assert(numAgentActionData == AgentActionManager::LastType, "agentActionData table does not match AgentActionManager types");

/**
 * @internal
 */
class Akonadi::AgentActionManagerPrivate
{
public:
    explicit AgentActionManagerPrivate(AgentActionManager *parent)
        : q(parent)
    {
        mActions.fill(nullptr, AgentActionManager::LastType);

        setContextText(AgentActionManager::CreateAgentInstance, AgentActionManager::DialogTitle, i18nc("@title:window", "New Agent Instance"));

        setContextText(AgentActionManager::CreateAgentInstance, AgentActionManager::ErrorMessageText, ki18n("Could not create agent instance: %1"));

        setContextText(AgentActionManager::CreateAgentInstance, AgentActionManager::ErrorMessageTitle, i18n("Agent instance creation failed"));

        setContextText(AgentActionManager::DeleteAgentInstance, AgentActionManager::MessageBoxTitle, i18nc("@title:window", "Delete Agent Instance?"));

        setContextText(AgentActionManager::DeleteAgentInstance,
                       AgentActionManager::MessageBoxText,
                       i18n("Do you really want to delete the selected agent instance?"));
    }

    void enableAction(AgentActionManager::Type type, bool enable)
    {
        Q_ASSERT(type >= 0 && type < AgentActionManager::LastType);
        if (QAction *act = mActions[type]) {
            act->setEnabled(enable);
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
            const AgentInstance &instance = instances.first();
            if (instance.type().capabilities().contains(QLatin1String("NoConfig"))) {
                configureActionEnabled = false;
            }
        }

        enableAction(AgentActionManager::CreateAgentInstance, createActionEnabled);
        enableAction(AgentActionManager::DeleteAgentInstance, deleteActionEnabled);
        enableAction(AgentActionManager::ConfigureAgentInstance, configureActionEnabled);

        Q_EMIT q->actionStateUpdated();
    }

    AgentInstance::List selectedAgentInstances() const
    {
        AgentInstance::List instances;

        if (!mSelectionModel) {
            return instances;
        }

        const QModelIndexList lstModelIndex = mSelectionModel->selectedRows();
        for (const QModelIndex &index : lstModelIndex) {
            const auto instance = index.data(AgentInstanceModel::InstanceRole).value<AgentInstance>();
            if (instance.isValid()) {
                instances << instance;
            }
        }

        return instances;
    }

    void slotCreateAgentInstance()
    {
        QPointer<Akonadi::AgentTypeDialog> dlg(new Akonadi::AgentTypeDialog(mParentWidget));
        dlg->setWindowTitle(contextText(AgentActionManager::CreateAgentInstance, AgentActionManager::DialogTitle));

        for (const QString &mimeType : std::as_const(mMimeTypeFilter)) {
            dlg->agentFilterProxyModel()->addMimeTypeFilter(mimeType);
        }

        for (const QString &capability : std::as_const(mCapabilityFilter)) {
            dlg->agentFilterProxyModel()->addCapabilityFilter(capability);
        }

        if (dlg->exec() == QDialog::Accepted) {
            const AgentType agentType = dlg->agentType();

            if (agentType.isValid()) {
                auto job = new AgentInstanceCreateJob(agentType, q);
                q->connect(job, &KJob::result, q, [this](KJob *job) {
                    slotAgentInstanceCreationResult(job);
                });
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
            if (KMessageBox::questionYesNo(mParentWidget,
                                           contextText(AgentActionManager::DeleteAgentInstance, AgentActionManager::MessageBoxText),
                                           contextText(AgentActionManager::DeleteAgentInstance, AgentActionManager::MessageBoxTitle),
                                           KStandardGuiItem::del(),
                                           KStandardGuiItem::cancel(),
                                           QString(),
                                           KMessageBox::Dangerous)
                == KMessageBox::Yes) {
                for (const AgentInstance &instance : instances) {
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
            KMessageBox::error(mParentWidget,
                               contextText(AgentActionManager::CreateAgentInstance, AgentActionManager::ErrorMessageText).arg(job->errorString()),
                               contextText(AgentActionManager::CreateAgentInstance, AgentActionManager::ErrorMessageTitle));
        }
    }

    void setContextText(AgentActionManager::Type type, AgentActionManager::TextContext context, const QString &data)
    {
        mContextTexts[type].insert(context, data);
    }

    void setContextText(AgentActionManager::Type type, AgentActionManager::TextContext context, const KLocalizedString &data)
    {
        mContextTexts[type].insert(context, data.toString());
    }

    QString contextText(AgentActionManager::Type type, AgentActionManager::TextContext context) const
    {
        return mContextTexts[type].value(context);
    }

    AgentActionManager *const q;
    KActionCollection *mActionCollection = nullptr;
    QWidget *mParentWidget = nullptr;
    QItemSelectionModel *mSelectionModel = nullptr;
    QVector<QAction *> mActions;
    QStringList mMimeTypeFilter;
    QStringList mCapabilityFilter;

    using ContextTexts = QHash<AgentActionManager::TextContext, QString>;
    QHash<AgentActionManager::Type, ContextTexts> mContextTexts;
};

/// @endcond

AgentActionManager::AgentActionManager(KActionCollection *actionCollection, QWidget *parent)
    : QObject(parent)
    , d(new AgentActionManagerPrivate(this))
{
    d->mParentWidget = parent;
    d->mActionCollection = actionCollection;
}

AgentActionManager::~AgentActionManager() = default;

void AgentActionManager::setSelectionModel(QItemSelectionModel *selectionModel)
{
    d->mSelectionModel = selectionModel;
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, [this]() {
        d->updateActions();
    });
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
    if (QAction *act = d->mActions[type]) {
        return act;
    }

    auto action = new QAction(d->mParentWidget);
#if KI18N_VERSION < QT_VERSION_CHECK(5, 89, 0)
    action->setText(i18n(agentActionData[type].label));
#else
    action->setText(agentActionData[type].label.toString());
#endif

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
        auto action = createAction(static_cast<Type>(type));
        Q_UNUSED(action)
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

void AgentActionManager::setContextText(Type type, TextContext context, const KLocalizedString &text)
{
    d->setContextText(type, context, text);
}

#include "moc_agentactionmanager.cpp"
