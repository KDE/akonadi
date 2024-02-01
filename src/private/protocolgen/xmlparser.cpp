/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.og>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "xmlparser.h"
#include "nodetree.h"

#include <QFile>

#include <iostream>

#define qPrintableRef(x) reinterpret_cast<const char *>((x).unicode())

XmlParser::XmlParser()
{
}

XmlParser::~XmlParser()
{
}

Node const *XmlParser::tree() const
{
    return mTree.get();
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
        if (mReader.isStartElement() && mReader.name() == QLatin1StringView("protocol")) {
            if (!parseProtocol()) {
                return false;
            }
        }
    }

    return true;
}

bool XmlParser::parseProtocol()
{
    Q_ASSERT(mReader.name() == QLatin1StringView("protocol"));

    const auto attrs = mReader.attributes();
    if (!attrs.hasAttribute(QLatin1StringView("version"))) {
        printError(QStringLiteral("Missing \"version\" attribute in <protocol> tag!"));
        return false;
    }

    auto documentNode = std::make_unique<DocumentNode>(attrs.value(QLatin1StringView("version")).toInt());

    while (!mReader.atEnd() && !(mReader.isEndElement() && mReader.name() == QLatin1StringView("protocol"))) {
        mReader.readNext();
        if (mReader.isStartElement()) {
            const auto elemName = mReader.name();
            if (elemName == QLatin1StringView("class") || elemName == QLatin1StringView("command") || elemName == QLatin1StringView("response")
                || elemName == QLatin1StringView("notification")) {
                if (!parseCommand(documentNode.get())) {
                    return false;
                }
            } else {
                printError(QStringLiteral("Unsupported tag: ").append(mReader.name()));
                return false;
            }
        }
    }

    mTree = std::move(documentNode);

    return true;
}

bool XmlParser::parseCommand(DocumentNode *documentNode)
{
    const auto attrs = mReader.attributes();
    if (!attrs.hasAttribute(QLatin1StringView("name"))) {
        printError(QStringLiteral("Missing \"name\" attribute  in command tag!"));
        return false;
    }

    auto classNode = new ClassNode(attrs.value(QLatin1StringView("name")).toString(), ClassNode::elementNameToType(mReader.name()), documentNode);
    new CtorNode({}, classNode);

    while (!mReader.atEnd() && !(mReader.isEndElement() && classNode->classType() == ClassNode::elementNameToType(mReader.name()))) {
        mReader.readNext();
        if (mReader.isStartElement()) {
            if (mReader.name() == QLatin1StringView("ctor")) {
                if (!parseCtor(classNode)) {
                    return false;
                }
            } else if (mReader.name() == QLatin1StringView("enum") || mReader.name() == QLatin1StringView("flag")) {
                if (!parseEnum(classNode)) {
                    return false;
                }
            } else if (mReader.name() == QLatin1StringView("param")) {
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
    QList<CtorNode::Argument> args;
    while (!mReader.atEnd() && !(mReader.isEndElement() && (mReader.name() == QLatin1StringView("ctor")))) {
        mReader.readNext();
        if (mReader.isStartElement()) {
            if (mReader.name() == QLatin1StringView("arg")) {
                const auto attrs = mReader.attributes();
                const QString name = attrs.value(QLatin1StringView("name")).toString();
                const QString def = attrs.value(QLatin1StringView("default")).toString();
                args << CtorNode::Argument{name, QString(), def};
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
    if (!attrs.hasAttribute(QLatin1StringView("name"))) {
        printError(QStringLiteral("Missing \"name\" attribute in enum/flag tag!"));
        return false;
    }

    auto enumNode = new EnumNode(attrs.value(QLatin1StringView("name")).toString(), EnumNode::elementNameToType(mReader.name()), classNode);

    while (!mReader.atEnd() && !(mReader.isEndElement() && (enumNode->enumType() == EnumNode::elementNameToType(mReader.name())))) {
        mReader.readNext();
        if (mReader.isStartElement()) {
            if (mReader.name() == QLatin1StringView("value")) {
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
    Q_ASSERT(mReader.name() == QLatin1StringView("value"));

    const auto attrs = mReader.attributes();
    if (!attrs.hasAttribute(QLatin1StringView("name"))) {
        printError(QStringLiteral("Missing \"name\" attribute in <value> tag!"));
        return false;
    }

    auto valueNode = new EnumValueNode(attrs.value(QLatin1StringView("name")).toString(), enumNode);
    if (attrs.hasAttribute(QLatin1StringView("value"))) {
        valueNode->setValue(attrs.value(QLatin1StringView("value")).toString());
    }

    return true;
}

bool XmlParser::parseParam(ClassNode *classNode)
{
    Q_ASSERT(mReader.name() == QLatin1StringView("param"));

    const auto attrs = mReader.attributes();
    if (!attrs.hasAttribute(QLatin1StringView("name"))) {
        printError(QStringLiteral("Missing \"name\" attribute in <param> tag!"));
        return false;
    }
    if (!attrs.hasAttribute(QLatin1StringView("type"))) {
        printError(QStringLiteral("Missing \"type\" attribute in <param> tag!"));
        return false;
    }

    const auto name = attrs.value(QLatin1StringView("name")).toString();
    const auto type = attrs.value(QLatin1StringView("type")).toString();

    for (const auto child : classNode->children()) {
        if (child->type() == Node::Ctor) {
            auto ctor = const_cast<CtorNode *>(static_cast<const CtorNode *>(child));
            ctor->setArgumentType(name, type);
        }
    }

    auto paramNode = new PropertyNode(name, type, classNode);

    if (attrs.hasAttribute(QLatin1StringView("default"))) {
        paramNode->setDefaultValue(attrs.value(QLatin1StringView("default")).toString());
    }
    if (attrs.hasAttribute(QLatin1StringView("readOnly"))) {
        paramNode->setReadOnly(attrs.value(QLatin1StringView("readOnly")) == QLatin1StringView("true"));
    }
    if (attrs.hasAttribute(QLatin1StringView("asReference"))) {
        paramNode->setAsReference(attrs.value(QLatin1StringView("asReference")) == QLatin1StringView("true"));
    }

    while (!mReader.atEnd() && !(mReader.isEndElement() && mReader.name() == QLatin1StringView("param"))) {
        mReader.readNext();
        if (mReader.isStartElement()) {
            if (mReader.name() == QLatin1StringView("setter")) {
                if (!parseSetter(paramNode)) {
                    return false;
                }
            } else if (mReader.name() == QLatin1StringView("depends")) {
                auto dependsAttrs = mReader.attributes();
                if (!dependsAttrs.hasAttribute(QLatin1StringView("enum"))) {
                    printError(QStringLiteral("Missing \"enum\" attribute in <depends> tag!"));
                    return false;
                }
                if (!dependsAttrs.hasAttribute(QLatin1StringView("value"))) {
                    printError(QStringLiteral("Missing \"value\" attribute in <depends> tag!"));
                    return false;
                }
                paramNode->addDependency(dependsAttrs.value(QLatin1StringView("enum")).toString(), dependsAttrs.value(QLatin1StringView("value")).toString());
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
    setter->name = attrs.value(QLatin1StringView("name")).toString();
    setter->type = attrs.value(QLatin1StringView("type")).toString();

    while (!mReader.atEnd() && !(mReader.isEndElement() && mReader.name() == QLatin1StringView("setter"))) {
        mReader.readNext();
        if (mReader.isStartElement()) {
            if (mReader.name() == QLatin1StringView("append")) {
                setter->append = mReader.attributes().value(QLatin1StringView("name")).toString();
            } else if (mReader.name() == QLatin1StringView("remove")) {
                setter->remove = mReader.attributes().value(QLatin1StringView("name")).toString();
            }
        }
    }

    parent->setSetter(setter);

    return true;
}

void XmlParser::printError(const QString &error)
{
    std::cerr << "Error:" << mReader.lineNumber() << ":" << mReader.columnNumber() << ": " << qPrintable(error) << std::endl;
}
