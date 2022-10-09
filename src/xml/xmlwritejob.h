/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadi-xml_export.h"
// AkonadiCore
#include <akonadi/collection.h>
#include <akonadi/job.h>

#include <memory>

namespace Akonadi
{
class Collection;
class XmlWriteJobPrivate;

/**
  Serializes a given Akonadi collection into a XML file.
*/
class AKONADI_XML_EXPORT XmlWriteJob : public Job
{
    Q_OBJECT
public:
    XmlWriteJob(const Collection &root, const QString &fileName, QObject *parent = nullptr);
    XmlWriteJob(const Collection::List &roots, const QString &fileName, QObject *parent = nullptr);
    ~XmlWriteJob() override;

protected:
    /* reimpl. */ void doStart() override;

private:
    void done();

private:
    friend class XmlWriteJobPrivate;
    std::unique_ptr<XmlWriteJobPrivate> const d;
};

}
