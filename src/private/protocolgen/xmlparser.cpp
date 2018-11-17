/*
    Copyright (c) 2017 Daniel Vrátil <dvratil@kde.og>

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

#include "xmlparser.h"
#include "nodetree.h"

#include <QFile>

#include <iostream>


#define qPrintableRef(x) reinterpret_cast<const char *>(x.unicode())

XmlParser::XmlParser()
    : mTree(nullptr)
{
}

XmlParser::~XmlParser()
{
    delete mTree;
}

Node const *XmlParser::tree() const
{
    return mTree;
}


bool XmlParser::parse(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        std::cerr << qPrintable(file.errorString());
        return false;
    }

    mReader.setDevice(&file);
    while (!mReader.atEnd()) {
        mReader.readNext();
        if (mReader.isStartElement() && mReader.name() == QLatin1Literal("protocol")) {
            if (!parseProtocol()) {
                return false;
            }
        }
    }

    return true;
}

bool XmlParser::parseProtocol()
{
    Q_ASSERT(mReader.name() == QLatin1String("protocol"));

    const auto attrs = mReader.attributes();
    if (!attrs.hasAttribute(QLatin1String("version"))) {
        printError(QStringLiteral("Missing \"version\" attribute in <protocol> tag!"));
        return false;
    }

    auto documentNode = new DocumentNode(attrs.value(QLatin1String("version")).toInt());

    while (!mReader.atEnd() &&
        !(mReader.isEndElement() && mReader.name() == QLatin1String("protocol")))
    {
        mReader.readNext();
        if (mReader.isStartElement()) {
            const auto elemName = mReader.name();
            if (elemName == QLatin1String("class") ||
                elemName == QLatin1String("command") ||
                elemName == QLatin1String("response") ||
                elemName == QLatin1String("notification"))
            {
                if (!parseCommand(documentNode)) {
                    return false;
                }
            } else {
                printError(QStringLiteral("Unsupported tag: ").append(mReader.name()));
                return false;
            }
        }
    }

    mTree = documentNode;

    return true;
}

bool XmlParser::parseCommand(DocumentNode *documentNode)
{
    const auto attrs = mReader.attributes();
    if (!attrs.hasAttribute(QLatin1String("name"))) {
        printError(QStringLiteral("Missing \"name\" attribute  in command tag!"));
        return false;
    }

    auto classNode = new ClassNode(attrs.value(QLatin1String("name")).toString(),
                                   ClassNode::elementNameToType(mReader.name()),
                                   documentNode);
    new CtorNode({}, classNode);

    while (!mReader.atEnd() &&
        !(mReader.isEndElement() && classNode->classType() == ClassNode::elementNameToType(mReader.name())))
    {
        mReader.readNext();
        if (mReader.isStartElement()) {
            if (mReader.name() == QLatin1String("ctor")) {
                if (!parseCtor(classNode)) {
                    return false;
                }
            } else if (mReader.name() == QLatin1String("enum")
                || mReader.name() == QLatin1String("flag")) {
                if (!parseEnum(classNode)) {
                    return false;
                }
            } else if (mReader.name() == QLatin1String("param")) {
                if (!parseParam(classNode)) {
                    return false;
                }
            } else {
                printError(QStringLiteral("Unsupported tag: ").append(mReader.name()));
                return false;
            }
        }
    }

    return true;
}

bool XmlParser::parseCtor(ClassNode *classNode)
{
    QVector<CtorNode::Argument> args;
    while (!mReader.atEnd() &&
            !(mReader.isEndElement() && (mReader.name() == QLatin1String("ctor"))))
    {
        mReader.readNext();
        if (mReader.isStartElement()) {
            if (mReader.name() == QLatin1String("arg")) {
                const auto attrs = mReader.attributes();
                const QString name = attrs.value(QLatin1String("name")).toString();
                const QString def = attrs.value(QLatin1String("default")).toString();
                args << CtorNode::Argument{ name, QString(), def };
            } else {
                printError(QStringLiteral("Unsupported tag: ").append(mReader.name()));
                return false;
            }
        }

    }
    new CtorNode(args, classNode);

    return true;
}

bool XmlParser::parseEnum(ClassNode *classNode)
{
    const auto attrs = mReader.attributes();
    if (!attrs.hasAttribute(QLatin1String("name"))) {
        printError(QStringLiteral("Missing \"name\" attribute in enum/flag tag!"));
        return false;
    }

    auto enumNode = new EnumNode(attrs.value(QLatin1String("name")).toString(),
                                 EnumNode::elementNameToType(mReader.name()),
                                 classNode);

    while (!mReader.atEnd() &&
            !(mReader.isEndElement() && (enumNode->enumType() == EnumNode::elementNameToType(mReader.name())))) {
        mReader.readNext();
        if (mReader.isStartElement()) {
            if (mReader.name() == QLatin1String("value")) {
                if (!parseEnumValue(enumNode)) {
                    return false;
                }
            } else {
                printError(QStringLiteral("Invalid tag inside of enum/flag tag: ").append(mReader.name()));
                return false;
            }
        }
    }

    return true;
}

bool XmlParser::parseEnumValue(EnumNode *enumNode)
{
    Q_ASSERT(mReader.name() == QLatin1String("value"));

    const auto attrs = mReader.attributes();
    if (!attrs.hasAttribute(QLatin1String("name"))) {
        printError(QStringLiteral("Missing \"name\" attribute in <value> tag!"));
        return false;
    }

    auto valueNode = new EnumValueNode(attrs.value(QLatin1String("name")).toString(),
                                       enumNode);
    if (attrs.hasAttribute(QLatin1String("value"))) {
        valueNode->setValue(attrs.value(QLatin1String("value")).toString());
    }

    return true;
}


bool XmlParser::parseParam(ClassNode *classNode)
{
    Q_ASSERT(mReader.name() == QLatin1String("param"));

    const auto attrs = mReader.attributes();
    if (!attrs.hasAttribute(QLatin1String("name"))) {
        printError(QStringLiteral("Missing \"name\" attribute in <param> tag!"));
        return false;
    }
    if (!attrs.hasAttribute(QLatin1String("type"))) {
        printError(QStringLiteral("Missing \"type\" attribute in <param> tag!"));
        return false;
    }

    const auto name = attrs.value(QLatin1String("name")).toString();
    const auto type = attrs.value(QLatin1String("type")).toString();

    for (auto child : classNode->children()) {
        if (child->type() == Node::Ctor) {
            auto ctor = const_cast<CtorNode *>(static_cast<const CtorNode *>(child));
            ctor->setArgumentType(name, type);
        }
    }

    auto paramNode = new PropertyNode(name, type, classNode);

    if (attrs.hasAttribute(QLatin1String("default"))) {
        paramNode->setDefaultValue(attrs.value(QLatin1String("default")).toString());
    }
    if (attrs.hasAttribute(QLatin1String("readOnly"))) {
        paramNode->setReadOnly(attrs.value(QLatin1String("readOnly")) == QLatin1String("true"));
    }
    if (attrs.hasAttribute(QLatin1String("asReference"))) {
        paramNode->setAsReference(attrs.value(QLatin1String("asReference")) == QLatin1String("true"));
    }

    while (!mReader.atEnd() &&
        !(mReader.isEndElement() && mReader.name() == QLatin1String("param"))) {
        mReader.readNext();
        if (mReader.isStartElement()) {
            if (mReader.name() == QLatin1String("setter")) {
                if (!parseSetter(paramNode)) {
                    return false;
                }
            } else if (mReader.name() == QLatin1String("depends")) {
                auto dependsAttrs = mReader.attributes();
                if (!dependsAttrs.hasAttribute(QLatin1String("enum"))) {
                    printError(QStringLiteral("Missing \"enum\" attribute in <depends> tag!"));
                    return false;
                }
                if (!dependsAttrs.hasAttribute(QLatin1String("value"))) {
                    printError(QStringLiteral("Missing \"value\" attribute in <depends> tag!"));
                    return false;
                }
                paramNode->addDependency(dependsAttrs.value(QLatin1String("enum")).toString(),
                                         dependsAttrs.value(QLatin1String("value")).toString());
            } else {
                printError(QStringLiteral("Unknown tag: ").append(mReader.name()));
                return false;
            }
        }
    }

    return true;
}

bool XmlParser::parseSetter(PropertyNode *parent)
{
    const auto attrs = mReader.attributes();
    auto setter = new PropertyNode::Setter;
    setter->name = attrs.value(QLatin1String("name")).toString();
    setter->type = attrs.value(QLatin1String("type")).toString();

    while (!mReader.atEnd() &&
        !(mReader.isEndElement() && mReader.name() == QLatin1String("setter"))) {
        mReader.readNext();
        if (mReader.isStartElement()) {
            if (mReader.name() == QLatin1String("append")) {
                setter->append = mReader.attributes().value(QLatin1String("name")).toString();
            } else if (mReader.name() == QLatin1String("remove")) {
                setter->remove = mReader.attributes().value(QLatin1String("name")).toString();
            }
        }
    }

    parent->setSetter(setter);

    return true;
}


void XmlParser::printError(const QString &error)
{
    std::cerr << "Error:" << mReader.lineNumber() << ":" << mReader.columnNumber()
              << ": " << qPrintable(error) << std::endl;
}
