/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2009 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "xmlwriter.h"
#include "format_p.h"

#include "attribute.h"
#include "collection.h"
#include "item.h"

using namespace Akonadi;

QDomElement XmlWriter::attributeToElement(Attribute *attr, QDomDocument &document)
{
    if (document.isNull()) {
        return QDomElement();
    }

    QDomElement top = document.createElement(Format::Tag::attribute());
    top.setAttribute(Format::Attr::attributeType(), QString::fromUtf8(attr->type()));
    QDomText attrText = document.createTextNode(QString::fromUtf8(attr->serialized()));
    top.appendChild(attrText);

    return top;
}

template<typename T> static void writeAttributesImpl(const T &entity, QDomElement &parentElem)
{
    if (parentElem.isNull()) {
        return;
    }

    QDomDocument doc = parentElem.ownerDocument();
    Q_FOREACH (Attribute *attr, entity.attributes()) {
        parentElem.appendChild(XmlWriter::attributeToElement(attr, doc));
    }
}

void XmlWriter::writeAttributes(const Item &item, QDomElement &parentElem)
{
    writeAttributesImpl(item, parentElem);
}

void XmlWriter::writeAttributes(const Collection &collection, QDomElement &parentElem)
{
    writeAttributesImpl(collection, parentElem);
}

QDomElement XmlWriter::collectionToElement(const Akonadi::Collection &collection, QDomDocument &document)
{
    if (document.isNull()) {
        return QDomElement();
    }

    QDomElement top = document.createElement(Format::Tag::collection());
    top.setAttribute(Format::Attr::remoteId(), collection.remoteId());
    top.setAttribute(Format::Attr::collectionName(), collection.name());
    top.setAttribute(Format::Attr::collectionContentTypes(), collection.contentMimeTypes().join(QLatin1Char(',')));
    writeAttributes(collection, top);

    return top;
}

QDomElement XmlWriter::writeCollection(const Akonadi::Collection &collection, QDomElement &parentElem)
{
    if (parentElem.isNull()) {
        return QDomElement();
    }

    QDomDocument doc = parentElem.ownerDocument();
    const QDomElement elem = collectionToElement(collection, doc);
    parentElem.insertBefore(elem, QDomNode()); // collection need to be before items to pass schema validation
    return elem;
}

QDomElement XmlWriter::itemToElement(const Akonadi::Item &item, QDomDocument &document)
{
    if (document.isNull()) {
        return QDomElement();
    }

    QDomElement top = document.createElement(Format::Tag::item());
    top.setAttribute(Format::Attr::remoteId(), item.remoteId());
    top.setAttribute(Format::Attr::itemMimeType(), item.mimeType());

    if (item.hasPayload()) {
        QDomElement payloadElem = document.createElement(Format::Tag::payload());
        QDomText payloadText = document.createTextNode(QString::fromUtf8(item.payloadData()));
        payloadElem.appendChild(payloadText);
        top.appendChild(payloadElem);
    }

    writeAttributes(item, top);

    Q_FOREACH (const Item::Flag &flag, item.flags()) {
        QDomElement flagElem = document.createElement(Format::Tag::flag());
        QDomText flagText = document.createTextNode(QString::fromUtf8(flag));
        flagElem.appendChild(flagText);
        top.appendChild(flagElem);
    }

    return top;
}

QDomElement XmlWriter::writeItem(const Item &item, QDomElement &parentElem)
{
    if (parentElem.isNull()) {
        return QDomElement();
    }

    QDomDocument doc = parentElem.ownerDocument();
    const QDomElement elem = itemToElement(item, doc);
    parentElem.appendChild(elem);
    return elem;
}
