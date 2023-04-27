#include "agentqmlconfigurationbase.h"

#include <QObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>

#include <KConfigSkeleton>

using namespace Akonadi;

namespace Akonadi
{
class AgentQmlConfigurationBasePrivate
{
public:
    KConfigSkeleton *skeleton;
    std::unique_ptr<QQmlContext> context;
};

} // namespace Akonadi

AgentQmlConfigurationBase::AgentQmlConfigurationBase(KConfigSkeleton *skeleton, QObject *parent, const QVariantList &args)
    : AgentConfigurationBase(skeleton->sharedConfig(), parent, args)
    , d(std::make_unique<AgentQmlConfigurationBasePrivate>())
{
    d->skeleton = skeleton;
}

AgentQmlConfigurationBase::~AgentQmlConfigurationBase() = default;

QQuickItem *AgentQmlConfigurationBase::mainUi(const QString &packagePath, QQmlEngine *engine, QQmlContext *context)
{
    d->context = std::make_unique<QQmlContext>(context);
    d->context->setContextProperty(QStringLiteral("settings"), d->skeleton);

    QQmlComponent component(engine, QUrl::fromLocalFile(packagePath));
    auto *obj = component.create(d->context.get());

    return qobject_cast<QQuickItem *>(obj);
}
