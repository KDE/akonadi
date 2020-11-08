/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "entitydisplayattribute.h"

#include "private/imapparser_p.h"


using namespace Akonadi;

class Q_DECL_HIDDEN EntityDisplayAttribute::Private
{
public:
    QString name;
    QString icon;
    QString activeIcon;
    QColor backgroundColor;
};

EntityDisplayAttribute::EntityDisplayAttribute()
    : d(std::make_unique<Private>())
{
}

EntityDisplayAttribute::~EntityDisplayAttribute() = default;

QString EntityDisplayAttribute::displayName() const
{
    return d->name;
}

void EntityDisplayAttribute::setDisplayName(const QString &name)
{
    d->name = name;
}

QIcon EntityDisplayAttribute::icon() const
{
    return QIcon::fromTheme(d->icon);
}

QString EntityDisplayAttribute::iconName() const
{
    return d->icon;
}

void EntityDisplayAttribute::setIconName(const QString &icon)
{
    d->icon = icon;
}

QByteArray Akonadi::EntityDisplayAttribute::type() const
{
    static const QByteArray sType("ENTITYDISPLAY");
    return sType;
}

EntityDisplayAttribute *EntityDisplayAttribute::clone() const
{
    auto *attr = new EntityDisplayAttribute();
    attr->d->name = d->name;
    attr->d->icon = d->icon;
    attr->d->activeIcon = d->activeIcon;
    attr->d->backgroundColor = d->backgroundColor;
    return attr;
}

QByteArray EntityDisplayAttribute::serialized() const
{
    QList<QByteArray> l;
    l.reserve(4);
    l << ImapParser::quote(d->name.toUtf8());
    l << ImapParser::quote(d->icon.toUtf8());
    l << ImapParser::quote(d->activeIcon.toUtf8());
    QList<QByteArray> components;
    if (d->backgroundColor.isValid()) {
        components = QList<QByteArray>() << QByteArray::number(d->backgroundColor.red())
                     << QByteArray::number(d->backgroundColor.green())
                     << QByteArray::number(d->backgroundColor.blue())
                     << QByteArray::number(d->backgroundColor.alpha());
    }
    l << '(' + ImapParser::join(components, " ") + ')';
    return '(' + ImapParser::join(l, " ") + ')';
}

void EntityDisplayAttribute::deserialize(const QByteArray &data)
{
    QList<QByteArray> l;
    ImapParser::parseParenthesizedList(data, l);
    int size = l.size();
    Q_ASSERT(size >= 2);
    d->name = QString::fromUtf8(l[0]);
    d->icon = QString::fromUtf8(l[1]);
    if (size >= 3) {
        d->activeIcon = QString::fromUtf8(l[2]);
    }
    if (size >= 4) {
        if (!l[3].isEmpty()) {
            QList<QByteArray> componentData;
            ImapParser::parseParenthesizedList(l[3], componentData);
            if (componentData.size() != 4) {
                return;
            }
            QVector<int> components;
            components.reserve(4);

            bool ok;
            for (int i = 0; i <= 3; ++i) {
                components << componentData.at(i).toInt(&ok);
                if (!ok) {
                    return;
                }
            }
            d->backgroundColor = QColor(components.at(0), components.at(1), components.at(2), components.at(3));
        }
    }
}

void EntityDisplayAttribute::setActiveIconName(const QString &name)
{
    d->activeIcon = name;
}

QIcon EntityDisplayAttribute::activeIcon() const
{
    return QIcon::fromTheme(d->activeIcon);
}

QString EntityDisplayAttribute::activeIconName() const
{
    return d->activeIcon;
}

QColor EntityDisplayAttribute::backgroundColor() const
{
    return d->backgroundColor;
}

void EntityDisplayAttribute::setBackgroundColor(const QColor &color)
{
    d->backgroundColor = color;
}
