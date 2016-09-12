/***************************************************************************
 *   Copyright (C) 2009 by Andras Mantia <amantia@kde.org>                 *
 *   Copyright (C) 2010 Volker Krause <vkrause@kde.org>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "parthelper.h"
#include "entities.h"
#include "selectquerybuilder.h"
#include "dbconfig.h"
#include "parttypehelper.h"

#include <private/externalpartstorage_p.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>

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
    if (part->external() && storeExternal) {
        if (!ExternalPartStorage::self()->updatePartFile(data, part->data(), newFile)) {
            throw PartHelperException(QStringLiteral("Failed to update external payload part"));
        }
        part->setData(newFile);
    } else if (!part->external() && storeExternal) {
        if (!ExternalPartStorage::self()->createPartFile(data, part->id(), newFile)) {
            throw PartHelperException(QStringLiteral("Failed to create external payload part"));
        }
        part->setData(newFile);
        part->setExternal(true);
    } else {
        if (part->external() && !storeExternal) {
            const QString file = ExternalPartStorage::resolveAbsolutePath(part->data());
            ExternalPartStorage::self()->removePartFile(file);
        }
        part->setData(data);
        part->setExternal(false);
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
        part->setExternal(true);
    } else {
        part->setExternal(false);
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

    if (part->external()) {
        ExternalPartStorage::self()->removePartFile(ExternalPartStorage::resolveAbsolutePath(part->data()));
    }
    return part->remove();
}

bool PartHelper::remove(const QString &column, const QVariant &value)
{
    SelectQueryBuilder<Part> builder;
    builder.addValueCondition(column, Query::Equals, value);
    builder.addValueCondition(Part::externalColumn(), Query::Equals, true);
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

QByteArray PartHelper::translateData(const QByteArray &data, bool isExternal)
{
    if (isExternal) {
        const QString fileName = ExternalPartStorage::resolveAbsolutePath(data);

        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray payload = file.readAll();
            file.close();
            return payload;
        } else {
            qCCritical(AKONADISERVER_LOG) << "Payload file " << fileName << " could not be open for reading!";
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
    return translateData(part.data(), part.external());
}

bool PartHelper::truncate(Part &part)
{
    if (part.external()) {
        ExternalPartStorage::self()->removePartFile(ExternalPartStorage::resolveAbsolutePath(part.data()));
    }

    part.setData(QByteArray());
    part.setDatasize(0);
    part.setExternal(false);
    return part.update();
}

bool PartHelper::verify(Part &part)
{
    if (!part.external()) {
        return true;
    }

    const QString fileName = ExternalPartStorage::resolveAbsolutePath(part.data());
    if (!QFile::exists(fileName)) {
        qCCritical(AKONADISERVER_LOG) << "Payload file" << fileName << "is missing, trying to recover.";
        part.setData(QByteArray());
        part.setDatasize(0);
        part.setExternal(false);
        return part.update();
    }

    return true;
}
