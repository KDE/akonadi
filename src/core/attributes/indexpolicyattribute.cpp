/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "indexpolicyattribute.h"

#include "private/imapparser_p.h"


using namespace Akonadi;

class Q_DECL_HIDDEN IndexPolicyAttribute::Private
{
public:
    bool enable = true;
};

IndexPolicyAttribute::IndexPolicyAttribute()
    : d(std::make_unique<Private>())
{
}

IndexPolicyAttribute::~IndexPolicyAttribute() = default;

bool IndexPolicyAttribute::indexingEnabled() const
{
    return d->enable;
}

void IndexPolicyAttribute::setIndexingEnabled(bool enable)
{
    d->enable = enable;
}

QByteArray IndexPolicyAttribute::type() const
{
    static const QByteArray sType("INDEXPOLICY");
    return sType;
}

Attribute *IndexPolicyAttribute::clone() const
{
    auto *attr = new IndexPolicyAttribute;
    attr->setIndexingEnabled(indexingEnabled());
    return attr;
}

QByteArray IndexPolicyAttribute::serialized() const
{
    QList<QByteArray> l;
    l.reserve(2);
    l.append("ENABLE");
    l.append(d->enable ? "true" : "false");
    return "(" + ImapParser::join(l, " ") + ')';   //krazy:exclude=doublequote_chars
}

void IndexPolicyAttribute::deserialize(const QByteArray &data)
{
    QList<QByteArray> l;
    ImapParser::parseParenthesizedList(data, l);
    for (int i = 0; i < l.size() - 1; i += 2) {
        const QByteArray &key = l.at(i);
        if (key == "ENABLE") {
            d->enable = l.at(i + 1) == "true";
        }
    }
}
