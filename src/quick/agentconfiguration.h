// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Akonadi/AgentFilterProxyModel>
#include <Akonadi/AgentInstance>
#include <Akonadi/SpecialCollections>
#include <qqmlregistration.h>

struct AgentDataChange {
    Q_GADGET
    Q_PROPERTY(QString instanceId MEMBER instanceId)
    Q_PROPERTY(int status MEMBER status)

    QString instanceId;
    int status;
};

class AgentConfiguration : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Akonadi::AgentFilterProxyModel *availableAgents READ availableAgents NOTIFY availableAgentsChanged)
    Q_PROPERTY(Akonadi::AgentFilterProxyModel *runningAgents READ runningAgents NOTIFY runningAgentsChanged)
    Q_PROPERTY(QStringList mimetypes READ mimetypes WRITE setMimetypes NOTIFY mimetypesChanged)
    Q_PROPERTY(Akonadi::SpecialCollections *specialCollections READ specialCollections WRITE setSpecialCollections NOTIFY specialCollectionsChanged)

public:
    enum AgentStatuses {
        Idle = Akonadi::AgentInstance::Idle,
        Running = Akonadi::AgentInstance::Running,
        Broken = Akonadi::AgentInstance::Broken,
        NotConfigured = Akonadi::AgentInstance::NotConfigured,
    };
    Q_ENUM(AgentStatuses)

    explicit AgentConfiguration(QObject *parent = nullptr);
    ~AgentConfiguration() override;

    [[nodiscard]] Akonadi::AgentFilterProxyModel *availableAgents();
    [[nodiscard]] Akonadi::AgentFilterProxyModel *runningAgents();
    [[nodiscard]] QStringList mimetypes() const;
    void setMimetypes(const QStringList &mimetypes);

    [[nodiscard]] Akonadi::SpecialCollections *specialCollections() const;
    void setSpecialCollections(Akonadi::SpecialCollections *specialCollections);

    Q_INVOKABLE [[nodiscard]] bool isRemovable(int index);

public Q_SLOTS:
    void createNew(int index);
    void edit(int index);
    void editIdentifier(const QString &resourceIdentifier);
    void remove(int index);
    void removeIdentifier(const QString &resourceIdentifier);
    void restart(int index);
    void restartIdentifier(const QString &resourceIdentifier);

Q_SIGNALS:
    void agentProgressChanged(const Akonadi::AgentInstance &instance);
    void mimetypesChanged();
    void runningAgentsChanged();
    void availableAgentsChanged();
    void errorOccurred(const QString &error);
    void specialCollectionsChanged();

private:
    void setupEdit(Akonadi::AgentInstance instance);
    void setupRemove(const Akonadi::AgentInstance &instance);
    void setupRestart(Akonadi::AgentInstance instance);

    Akonadi::SpecialCollections *m_specialCollections = nullptr;
    Akonadi::AgentFilterProxyModel *m_runningAgents = nullptr;
    Akonadi::AgentFilterProxyModel *m_availableAgents = nullptr;
    QStringList m_mimetypes;
};
