/***************************************************************************
 *   SPDX-FileCopyrightText: 2009 Andras Mantia <amantia@kde.org>          *
 *   SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "parthelper.h"
#include "selectquerybuilder.h"
#include "dbconfig.h"
#include "parttypehelper.h"

#include <private/externalpartstorage_p.h>

#include <QFile>

#include "akonadiserver_debug.h"

using namespace Akonadi;
using namespace Akonadi::Server;

void PartHelper::update(Part *part, const QByteArray &data, qint64 dataSize)
{
    if (!part) {
        throw PartHelperException("Invalid part");
    }

    const bool storeExternal = dataSize > DbConfig::configuredDatabase()->sizeThreshold();

    QByteArray newFile;
    if (part->storage() == Part::External && storeExternal) {
        if (!ExternalPartStorage::self()->updatePartFile(data, part->data(), newFile)) {
            throw PartHelperException(QStringLiteral("Failed to update external payload part"));
        }
        part->setData(newFile);
    } else if (part->storage() != Part::External && storeExternal) {
        if (!ExternalPartStorage::self()->createPartFile(data, part->id(), newFile)) {
            throw PartHelperException(QStringLiteral("Failed to create external payload part"));
        }
        part->setData(newFile);
        part->setStorage(Part::External);
    } else {
        if (part->storage() == Part::External && !storeExternal) {
            const QString file = ExternalPartStorage::resolveAbsolutePath(part->data());
            ExternalPartStorage::self()->removePartFile(file);
        }
        part->setData(data);
        part->setStorage(Part::Internal);
    }

    part->setDatasize(dataSize);
    const bool result = part->update();
    if (!result) {
        throw PartHelperException("Failed to update database record");
    }
}

bool PartHelper::insert(Part *part, qint64 *insertId)
{
    if (!part) {
        return false;
    }

    const bool storeInFile = part->datasize() > DbConfig::configuredDatabase()->sizeThreshold();
    //it is needed to insert first the metadata so a new id is generated for the part,
    //and we need this id for the payload file name
    QByteArray data;
    if (storeInFile) {
        data = part->data();
        part->setData(QByteArray());
        part->setStorage(Part::External);
    } else {
        part->setStorage(Part::Internal);
    }

    bool result = part->insert(insertId);

    if (storeInFile && result) {
        QByteArray filename;
        if (!ExternalPartStorage::self()->createPartFile(data, part->id(), filename)) {
            throw PartHelperException("Failed to create external payload part");
        }
        part->setData(filename);
        result = part->update();
    }

    return result;
}

bool PartHelper::remove(Part *part)
{
    if (!part) {
        return false;
    }

    if (part->storage() == Part::External) {
        ExternalPartStorage::self()->removePartFile(ExternalPartStorage::resolveAbsolutePath(part->data()));
    }
    return part->remove();
}

bool PartHelper::remove(const QString &column, const QVariant &value)
{
    SelectQueryBuilder<Part> builder;
    builder.addValueCondition(column, Query::Equals, value);
    builder.addValueCondition(Part::storageColumn(), Query::Equals, Part::External);
    builder.addValueCondition(Part::dataColumn(), Query::IsNot, QVariant());
    if (!builder.exec()) {
//      qCDebug(AKONADISERVER_LOG) << "Error selecting records to be deleted from table"
//          << Part::tableName() << builder.query().lastError().text();
        return false;
    }
    const Part::List parts = builder.result();
    Part::List::ConstIterator it = parts.constBegin();
    Part::List::ConstIterator end = parts.constEnd();
    for (; it != end; ++it) {
        ExternalPartStorage::self()->removePartFile(ExternalPartStorage::resolveAbsolutePath((*it).data()));
    }
    return Part::remove(column, value);
}

QByteArray PartHelper::translateData(const QByteArray &data, Part::Storage storage)
{
    if (storage == Part::External || storage == Part::Foreign) {
        QFile file;
        if (storage == Part::External) {
            file.setFileName(ExternalPartStorage::resolveAbsolutePath(data));
        } else {
            file.setFileName(QString::fromUtf8(data));
        }

        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray payload = file.readAll();
            file.close();
            return payload;
        } else {
            qCCritical(AKONADISERVER_LOG) << "Payload file " << file.fileName() << " could not be open for reading!";
            qCCritical(AKONADISERVER_LOG) << "Error: " << file.errorString();
            return QByteArray();
        }
    } else {
        // not external
        return data;
    }
}

QByteArray PartHelper::translateData(const Part &part)
{
    return translateData(part.data(), part.storage());
}

bool PartHelper::truncate(Part &part)
{
    if (part.storage() == Part::External) {
        ExternalPartStorage::self()->removePartFile(ExternalPartStorage::resolveAbsolutePath(part.data()));
    }

    part.setData(QByteArray());
    part.setDatasize(0);
    part.setStorage(Part::Internal);
    return part.update();
}

bool PartHelper::verify(Part &part)
{
    if (part.storage() == Part::Internal) {
        return true;
    }

    QString fileName;
    if (part.storage() == Part::External) {
        fileName = ExternalPartStorage::resolveAbsolutePath(part.data());
    } else if (part.storage() == Part::Foreign) {
        fileName = QString::fromUtf8(part.data());
    } else {
        Q_ASSERT(false);
    }

    if (!QFile::exists(fileName)) {
        qCCritical(AKONADISERVER_LOG) << "Payload file" << fileName << "is missing, trying to recover.";
        part.setData(QByteArray());
        part.setDatasize(0);
        part.setStorage(Part::Internal);
        return part.update();
    }

    return true;
}
