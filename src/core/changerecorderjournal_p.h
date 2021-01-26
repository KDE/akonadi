/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CHANGERECORDERJOURNAL_P_H
#define CHANGERECORDERJOURNAL_P_H

#include "akonaditests_export.h"
#include "private/protocol_p.h"

class QSettings;
class QFile;

namespace Akonadi
{
class AKONADI_TESTS_EXPORT ChangeRecorderJournalReader
{
public:
    enum LegacyType { InvalidType, Item, Collection, Tag, Relation };

    // Ancient QSettings legacy store
    static Protocol::ChangeNotificationPtr loadQSettingsNotification(QSettings *settings);

    static QQueue<Protocol::ChangeNotificationPtr> loadFrom(QFile *device, bool &needsFullSave);

private:
    enum LegacyOp { InvalidOp, Add, Modify, Move, Remove, Link, Unlink, Subscribe, Unsubscribe, ModifyFlags, ModifyTags, ModifyRelations };

    static Protocol::ChangeNotificationPtr loadQSettingsItemNotification(QSettings *settings);
    static Protocol::ChangeNotificationPtr loadQSettingsCollectionNotification(QSettings *settings);

    // More modern mechanisms
    static Protocol::ChangeNotificationPtr loadItemNotification(QDataStream &stream, quint64 version);
    static Protocol::ChangeNotificationPtr loadCollectionNotification(QDataStream &stream, quint64 version);
    static Protocol::ChangeNotificationPtr loadTagNotification(QDataStream &stream, quint64 version);
    static Protocol::ChangeNotificationPtr loadRelationNotification(QDataStream &stream, quint64 version);

    static Protocol::ItemChangeNotification::Operation mapItemOperation(LegacyOp op);
    static Protocol::CollectionChangeNotification::Operation mapCollectionOperation(LegacyOp op);
    static Protocol::TagChangeNotification::Operation mapTagOperation(LegacyOp op);
    static Protocol::RelationChangeNotification::Operation mapRelationOperation(LegacyOp op);

    static QSet<Protocol::ItemChangeNotification::Relation> extractRelations(QSet<QByteArray> &flags);
};

class AKONADI_TESTS_EXPORT ChangeRecorderJournalWriter
{
public:
    static void saveTo(const QQueue<Protocol::ChangeNotificationPtr> &changes, QIODevice *device);

private:
    static ChangeRecorderJournalReader::LegacyType mapToLegacyType(Protocol::Command::Type type);

    static void saveItemNotification(QDataStream &stream, const Protocol::ItemChangeNotification &ntf);
    static void saveCollectionNotification(QDataStream &stream, const Protocol::CollectionChangeNotification &ntf);
    static void saveTagNotification(QDataStream &stream, const Protocol::TagChangeNotification &ntf);
    static void saveRelationNotification(QDataStream &stream, const Protocol::RelationChangeNotification &ntf);

    static QSet<QByteArray> encodeRelations(const QSet<Protocol::ItemChangeNotification::Relation> &relations);
};

} // namespace

#endif
