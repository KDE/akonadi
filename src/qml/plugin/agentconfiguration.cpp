#include "agentconfiguration.h"

using namespace Akonadi::Qml;

AgentConfiguration::AgentConfiguration(QObject *parent)
    : QQuickItem(parent)
{
}

QString AgentConfiguration::instanceId() const
{
    return mInstanceId;
}

void AgentConfiguration::setInstanceId(const QString &instanceId)
{
    if (mInstanceId == instanceId) {
        return;
    }

    mInstanceId = instanceId;
    Q_EMIT instanceIdChanged(mInstanceId);
}