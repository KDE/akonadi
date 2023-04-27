#pragma once

#include <QQuickItem>

namespace Akonadi::Qml
{

class AgentConfiguration : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QString instanceId READ instanceId WRITE setInstanceId NOTIFY instanceIdChanged)
public:
    explicit AgentConfig(QObject *parent);

    QString instanceId() const;
    void setInstanceId(const QString &instanceId);

Q_SIGNALS:
    void instanceIdChanged(const QString &instanceId);

private:
    QString mInstanceId;
};

} // namespace Akonadi::Qml