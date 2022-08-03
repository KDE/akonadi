/*
  SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>
  SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QObject>

#include <Akonadi/Tag>

#include <memory>

class QColor;

namespace Akonadi
{

class TagCachePrivate;

/**
 * Client-side cache of all exist tags.
 *
 * This can be instantiated explicitly or used as a singleton for
 * process-wide sharing.
 *
 * @since 5.20.43
 */
class AKONADICORE_EXPORT TagCache : public QObject
{
    Q_OBJECT
public:
    explicit TagCache(QObject *parent = nullptr);
    ~TagCache();

    /** Returns the tag with the GID @p gid, if available. */
    Q_REQUIRED_RESULT Akonadi::Tag tagByGid(const QByteArray &gid) const;
    /** Returns the tag with the name @p name, if available. */
    Q_REQUIRED_RESULT Akonadi::Tag tagByName(const QString &name) const;

    /** Returns the (background) color of the tag named @p tagName.
     *  If there is no such tag, or the tag has no color associated,
     *  an invalid QColor value is returned.
     */
    Q_REQUIRED_RESULT QColor tagColor(const QString &tagName) const;

    /** Sets the (background) color of the tag named @p tagName to @p color. */
    void setTagColor(const QString &tagName, const QColor &color);

    /** Returns the singleton instance. */
    static TagCache *instance();

private:
    std::unique_ptr<TagCachePrivate> d;
};
}
