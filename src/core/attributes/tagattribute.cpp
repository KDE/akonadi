/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagattribute.h"

#include "private/imapparser_p.h"

using namespace Akonadi;

class Q_DECL_HIDDEN TagAttribute::Private
{
public:
    QString name;
    QString icon;
    QColor backgroundColor;
    QColor textColor;
    QString font;
    bool inToolbar = false;
    QString shortcut;
    int priority = -1;
};

TagAttribute::TagAttribute()
    : d(std::make_unique<Private>())
{
}

TagAttribute::~TagAttribute() = default;

QString TagAttribute::displayName() const
{
    return d->name;
}

void TagAttribute::setDisplayName(const QString &name)
{
    d->name = name;
}

QString TagAttribute::iconName() const
{
    return d->icon;
}

void TagAttribute::setIconName(const QString &icon)
{
    d->icon = icon;
}

QByteArray Akonadi::TagAttribute::type() const
{
    static const QByteArray sType("TAG");
    return sType;
}

TagAttribute *TagAttribute::clone() const
{
    auto attr = new TagAttribute();
    attr->d->name = d->name;
    attr->d->icon = d->icon;
    attr->d->backgroundColor = d->backgroundColor;
    attr->d->textColor = d->textColor;
    attr->d->font = d->font;
    attr->d->inToolbar = d->inToolbar;
    attr->d->shortcut = d->shortcut;
    attr->d->priority = d->priority;
    return attr;
}

QByteArray TagAttribute::serialized() const
{
    QList<QByteArray> l;
    l.reserve(8);
    l << ImapParser::quote(d->name.toUtf8());
    l << ImapParser::quote(d->icon.toUtf8());
    l << ImapParser::quote(d->font.toUtf8());
    l << ImapParser::quote(d->shortcut.toUtf8());
    l << ImapParser::quote(QString::number(d->inToolbar).toUtf8());
    {
        QList<QByteArray> components;
        if (d->backgroundColor.isValid()) {
            components = QList<QByteArray>() << QByteArray::number(d->backgroundColor.red()) << QByteArray::number(d->backgroundColor.green())
                                             << QByteArray::number(d->backgroundColor.blue()) << QByteArray::number(d->backgroundColor.alpha());
        }
        l << '(' + ImapParser::join(components, " ") + ')';
    }
    {
        QList<QByteArray> components;
        if (d->textColor.isValid()) {
            components = QList<QByteArray>() << QByteArray::number(d->textColor.red()) << QByteArray::number(d->textColor.green())
                                             << QByteArray::number(d->textColor.blue()) << QByteArray::number(d->textColor.alpha());
        }
        l << '(' + ImapParser::join(components, " ") + ')';
    }
    l << ImapParser::quote(QString::number(d->priority).toUtf8());
    return '(' + ImapParser::join(l, " ") + ')';
}

static QColor parseColor(const QByteArray &data)
{
    QList<QByteArray> componentData;
    ImapParser::parseParenthesizedList(data, componentData);
    if (componentData.size() != 4) {
        return QColor();
    }
    QVector<int> components;
    components.reserve(4);
    bool ok;
    for (int i = 0; i <= 3; ++i) {
        components << componentData.at(i).toInt(&ok);
        if (!ok) {
            return QColor();
        }
    }
    return QColor(components.at(0), components.at(1), components.at(2), components.at(3));
}

void TagAttribute::deserialize(const QByteArray &data)
{
    QList<QByteArray> l;
    ImapParser::parseParenthesizedList(data, l);
    int size = l.size();
    Q_ASSERT(size >= 7);
    d->name = QString::fromUtf8(l[0]);
    d->icon = QString::fromUtf8(l[1]);
    d->font = QString::fromUtf8(l[2]);
    d->shortcut = QString::fromUtf8(l[3]);
    d->inToolbar = QString::fromUtf8(l[4]).toInt();
    if (!l[5].isEmpty()) {
        d->backgroundColor = parseColor(l[5]);
    }
    if (!l[6].isEmpty()) {
        d->textColor = parseColor(l[6]);
    }
    if (l.size() >= 8) {
        d->priority = QString::fromUtf8(l[7]).toInt();
    }
}

QColor TagAttribute::backgroundColor() const
{
    return d->backgroundColor;
}

void TagAttribute::setBackgroundColor(const QColor &color)
{
    d->backgroundColor = color;
}

void TagAttribute::setTextColor(const QColor &color)
{
    d->textColor = color;
}

QColor TagAttribute::textColor() const
{
    return d->textColor;
}

void TagAttribute::setFont(const QString &font)
{
    d->font = font;
}

QString TagAttribute::font() const
{
    return d->font;
}

void TagAttribute::setInToolbar(bool inToolbar)
{
    d->inToolbar = inToolbar;
}

bool TagAttribute::inToolbar() const
{
    return d->inToolbar;
}

void TagAttribute::setShortcut(const QString &shortcut)
{
    d->shortcut = shortcut;
}

QString TagAttribute::shortcut() const
{
    return d->shortcut;
}

void TagAttribute::setPriority(int priority)
{
    d->priority = priority;
}

int TagAttribute::priority() const
{
    return d->priority;
}
