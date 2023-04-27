#pragma once

#include "akonadiqml_export.h"
#include <core/agentconfigurationbase.h>

#include <memory>

#include <QVariantList>

class QObject;
class QQmlEngine;
class QQmlContext;
class QQuickItem;

class KConfigSkeleton;

namespace Akonadi
{

class AgentQmlConfigurationBasePrivate;

class AKONADIQML_EXPORT AgentQmlConfigurationBase : public AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit AgentQmlConfigurationBase(KConfigSkeleton *config, QObject *parent, const QVariantList &args);
    ~AgentQmlConfigurationBase();

    virtual QQuickItem *mainUi(const QString &packagePath, QQmlEngine *engine, QQmlContext *context);

private:
    std::unique_ptr<AgentQmlConfigurationBasePrivate> d;
};

} // namespace Akonadi