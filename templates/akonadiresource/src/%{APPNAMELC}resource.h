/*
 * SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef %{APPNAMEUC}RESOURCE_H
#define %{APPNAMEUC}RESOURCE_H

#include <AkonadiAgentBase/ResourceBase>

class %{APPNAME}Resource : public Akonadi::ResourceBase,
                           public Akonadi::AgentBase::Observer
{
    Q_OBJECT

public:
    explicit %{APPNAME}Resource(const QString &id);
    ~%{APPNAME}Resource() override;

public Q_SLOTS:
    void configure(WId windowId) override;

protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &col) override;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;

protected:
    void aboutToQuit() override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;
};

#endif
