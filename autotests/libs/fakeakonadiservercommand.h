/*
  Copyright (C) 2009 Stephen Kelly <steveire@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef FAKE_AKONADI_SERVER_COMMAND_H
#define FAKE_AKONADI_SERVER_COMMAND_H

#include <QString>

#include "collection.h"
#include "entitytreemodel.h"
#include "item.h"
#include "tagmodel.h"
#include "tag.h"
#include "akonaditestfake_export.h"

class FakeServerData;

class AKONADITESTFAKE_EXPORT FakeAkonadiServerCommand : public QObject
{
    Q_OBJECT
public:
    enum Type {
        Notification,
        RespondToCollectionFetch,
        RespondToItemFetch,
        RespondToTagFetch
    };

    FakeAkonadiServerCommand(Type type, FakeServerData *serverData);

    virtual ~FakeAkonadiServerCommand()
    {
    }

    Type respondTo() const
    {
        return m_type;
    }
    Akonadi::Collection fetchCollection() const
    {
        return m_parentCollection;
    }

    Type m_type;

    virtual void doCommand() = 0;

Q_SIGNALS:
    void emit_itemsFetched(const Akonadi::Item::List &list);
    void emit_collectionsFetched(const Akonadi::Collection::List &list);
    void emit_tagsFetched(const Akonadi::Tag::List &tags);

    void emit_monitoredCollectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &target);
    void emit_monitoredCollectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent);
    void emit_monitoredCollectionRemoved(const Akonadi::Collection &collection);
    void emit_monitoredCollectionChanged(const Akonadi::Collection &collection);

    void emit_monitoredItemMoved(const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &target);
    void emit_monitoredItemAdded(const Akonadi::Item &item, const Akonadi::Collection &parent);
    void emit_monitoredItemRemoved(const Akonadi::Item &item);
    void emit_monitoredItemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts);

    void emit_monitoredItemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection);
    void emit_monitoredItemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection);

    void emit_monitoredTagAdded(const Akonadi::Tag &tag);
    void emit_monitoredTagChanged(const Akonadi::Tag &tag);
    void emit_monitoredTagRemoved(const Akonadi::Tag &tag);
protected:
    Akonadi::Collection getCollectionByDisplayName(const QString &displayName) const;
    Akonadi::Item getItemByDisplayName(const QString &displayName) const;
    Akonadi::Tag getTagByDisplayName(const QString &displayName) const;

    bool isItemSignal(const QByteArray &signature) const;
    bool isCollectionSignal(const QByteArray &signature) const;
    bool isTagSignal(const QByteArray &signature) const;

protected:
    FakeServerData *m_serverData = nullptr;
    QAbstractItemModel *m_model = nullptr;
    Akonadi::Collection m_parentCollection;
    Akonadi::Tag m_parentTag;
    QHash<Akonadi::Collection::Id, Akonadi::Collection> m_collections;
    QHash<Akonadi::Item::Id, Akonadi::Item> m_items;
    QHash<Akonadi::Collection::Id, QList<Akonadi::Collection::Id> > m_childElements;
    QHash<Akonadi::Tag::Id, Akonadi::Tag> m_tags;

private:
    void connectForwardingSignals();
};

class AKONADITESTFAKE_EXPORT FakeMonitorCommand : public FakeAkonadiServerCommand
{
public:
    explicit FakeMonitorCommand(FakeServerData *serverData)
        : FakeAkonadiServerCommand(Notification, serverData)
    {
    }
    virtual ~FakeMonitorCommand()
    {
    }
};

class AKONADITESTFAKE_EXPORT FakeCollectionMovedCommand : public FakeMonitorCommand
{
public:
    FakeCollectionMovedCommand(const QString &collection, const QString &source, const QString &target, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_collectionName(collection)
        , m_sourceName(source)
        , m_targetName(target)
    {
    }

    virtual ~FakeCollectionMovedCommand()
    {
    }

    void doCommand() override;

private:
    QString m_collectionName;
    QString m_sourceName;
    QString m_targetName;
};

class AKONADITESTFAKE_EXPORT FakeCollectionAddedCommand : public FakeMonitorCommand
{
public:
    FakeCollectionAddedCommand(const QString &collection, const QString &parent, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_collectionName(collection)
        , m_parentName(parent)
    {
    }

    virtual ~FakeCollectionAddedCommand()
    {
    }

    void doCommand() override;

private:
    QString m_collectionName;
    QString m_parentName;
};

class AKONADITESTFAKE_EXPORT FakeCollectionRemovedCommand : public FakeMonitorCommand
{
public:
    FakeCollectionRemovedCommand(const QString &collection, const QString &source, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_collectionName(collection)
        , m_parentName(source)
    {
    }

    virtual ~FakeCollectionRemovedCommand()
    {
    }

    void doCommand() override;

private:
    QString m_collectionName;
    QString m_parentName;
};

class AKONADITESTFAKE_EXPORT FakeCollectionChangedCommand : public FakeMonitorCommand
{
public:
    FakeCollectionChangedCommand(const QString &collection, const QString &parent, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_collectionName(collection)
        , m_parentName(parent)
    {
    }

    FakeCollectionChangedCommand(const Akonadi::Collection &collection, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_collection(collection)
    {
    }

    virtual ~FakeCollectionChangedCommand()
    {
    }

    void doCommand() override;

private:
    Akonadi::Collection m_collection;
    QString m_collectionName;
    QString m_parentName;
};

class AKONADITESTFAKE_EXPORT FakeItemMovedCommand : public FakeMonitorCommand
{
public:
    FakeItemMovedCommand(const QString &item, const QString &source, const QString &target, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_itemName(item)
        , m_sourceName(source)
        , m_targetName(target)
    {
    }

    virtual ~FakeItemMovedCommand()
    {
    }

    void doCommand() override;

private:
    QString m_itemName;
    QString m_sourceName;
    QString m_targetName;
};

class AKONADITESTFAKE_EXPORT FakeItemAddedCommand : public FakeMonitorCommand
{
public:
    FakeItemAddedCommand(const QString &item, const QString &parent, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_itemName(item)
        , m_parentName(parent)
    {
    }

    virtual ~FakeItemAddedCommand()
    {
    }

    void doCommand() override;

private:
    QString m_itemName;
    QString m_parentName;
};

class AKONADITESTFAKE_EXPORT FakeItemRemovedCommand : public FakeMonitorCommand
{
public:
    FakeItemRemovedCommand(const QString &item, const QString &parent, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_itemName(item)
        , m_parentName(parent)
    {
    }

    virtual ~FakeItemRemovedCommand()
    {
    }

    void doCommand() override;

private:
    QString m_itemName;
    QString m_parentName;
    FakeServerData *m_serverData = nullptr;
};

class AKONADITESTFAKE_EXPORT FakeItemChangedCommand : public FakeMonitorCommand
{
public:
    FakeItemChangedCommand(const QString &item, const QString &parent, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_itemName(item)
        , m_parentName(parent)
    {
    }

    virtual ~FakeItemChangedCommand()
    {
    }

    void doCommand() override;

private:
    QString m_itemName;
    QString m_parentName;
};

class AKONADITESTFAKE_EXPORT FakeTagAddedCommand : public FakeMonitorCommand
{
public:
    FakeTagAddedCommand(const QString &tag, const QString &parent, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_tagName(tag)
        , m_parentName(parent)
    {
    }

    virtual ~FakeTagAddedCommand()
    {
    }

    void doCommand() override;

private:
    QString m_tagName;
    QString m_parentName;
};

class AKONADITESTFAKE_EXPORT FakeTagChangedCommand : public FakeMonitorCommand
{
public:
    FakeTagChangedCommand(const QString &tag, const QString &parent, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_tagName(tag)
        , m_parentName(parent)
    {
    }

    virtual ~FakeTagChangedCommand()
    {
    }

    void doCommand() override;

private:
    QString m_tagName;
    QString m_parentName;
};

class AKONADITESTFAKE_EXPORT FakeTagMovedCommand : public FakeMonitorCommand
{
public:
    FakeTagMovedCommand(const QString &tag, const QString &oldParent, const QString &newParent, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_tagName(tag)
        , m_oldParent(oldParent)
        , m_newParent(newParent)
    {
    }

    virtual ~FakeTagMovedCommand()
    {
    }

    void doCommand() override;

private:
    QString m_tagName;
    QString m_oldParent;
    QString m_newParent;
};

class AKONADITESTFAKE_EXPORT FakeTagRemovedCommand : public FakeMonitorCommand
{
public:
    FakeTagRemovedCommand(const QString &tag, const QString &parent, FakeServerData *serverData)
        : FakeMonitorCommand(serverData)
        , m_tagName(tag)
        , m_parentName(parent)
    {
    }

    virtual ~FakeTagRemovedCommand()
    {
    }

    void doCommand() override;

private:
    QString m_tagName;
    QString m_parentName;
};

class AKONADITESTFAKE_EXPORT FakeJobResponse : public FakeAkonadiServerCommand
{
    struct Token {
        enum Type {
            Branch,
            Leaf
        };
        Type type;
        QString content;
    };
public:
    FakeJobResponse(const Akonadi::Collection &parentCollection, Type respondTo, FakeServerData *serverData)
        : FakeAkonadiServerCommand(respondTo, serverData)
    {
        m_parentCollection = parentCollection;
    }

    FakeJobResponse(const Akonadi::Tag &parentTag, Type respondTo, FakeServerData *serverData)
        : FakeAkonadiServerCommand(respondTo, serverData)
    {
        m_parentTag = parentTag;
    }

    virtual ~FakeJobResponse()
    {
    }

    void appendCollection(const Akonadi::Collection &collection)
    {
        m_collections.insert(collection.id(), collection);
        m_childElements[collection.parentCollection().id()].append(collection.id());
    }
    void appendItem(const Akonadi::Item &item)
    {
        m_items.insert(item.id(), item);
    }

    void appendTag(const Akonadi::Tag &tag)
    {
        m_tags.insert(tag.id(), tag);
    }

    void doCommand() override;

    static QList<FakeAkonadiServerCommand *> interpret(FakeServerData *fakeServerData, const QString &input);

private:
    static QList<FakeJobResponse *> parseTreeString(FakeServerData *fakeServerData, const QString &treeString);
    static QList<FakeJobResponse::Token> tokenize(const QString &treeString);
    static void parseEntityString(QList<FakeJobResponse *> &collectionResponseList,
                                  QHash<Akonadi::Collection::Id, FakeJobResponse *> &itemResponseMap,
                                  QList<FakeJobResponse *> &tagResponseList,
                                  Akonadi::Collection::List &recentCollections,
                                  Akonadi::Tag::List &recentTags,
                                  FakeServerData *fakeServerData,
                                  const QString &entityString,
                                  int depth);
};

#endif
