/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadi-xml_export.h"
#include "collection.h"
#include "job.h"
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
    friend class XmlWriteJobPrivate;
    XmlWriteJobPrivate *const d;
    void done();
};

}

