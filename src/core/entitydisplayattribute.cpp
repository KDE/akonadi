/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "entitydisplayattribute.h"

#include <akonadi/private/imapparser_p.h>

#include <QIcon>

using namespace Akonadi;

class EntityDisplayAttribute::Private
{
public:
    Private()
        : hidden(false)
    {
    }
    QString name;
    QString icon;
    QString activeIcon;
    QColor backgroundColor;
    bool hidden;
};

EntityDisplayAttribute::EntityDisplayAttribute()
    : d(new Private)
{
}

EntityDisplayAttribute::~ EntityDisplayAttribute()
{
    delete d;
}

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
    static const QByteArray sType( "ENTITYDISPLAY" );
    return sType;
}

EntityDisplayAttribute *EntityDisplayAttribute::clone() const
{
    EntityDisplayAttribute *attr = new EntityDisplayAttribute();
    attr->d->name = d->name;
    attr->d->icon = d->icon;
    attr->d->activeIcon = d->activeIcon;
    attr->d->backgroundColor = d->backgroundColor;
    return attr;
}

QByteArray EntityDisplayAttribute::serialized() const
{
    QList<QByteArray> l;
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
            QList<int> components;

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
