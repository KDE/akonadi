/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2007 Robert Zwerus <arzie@dds.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ITEMDUMPER_H
#define ITEMDUMPER_H

#include "collection.h"

#include <QElapsedTimer>

class KJob;

class ItemDumper : public QObject
{
    Q_OBJECT
public:
    ItemDumper(const QString &path, const QString &filename, const QString &mimetype, int count);

private Q_SLOTS:
    void done(KJob *job);

private:
    QElapsedTimer mTime;
    int mJobCount;
};

#endif
