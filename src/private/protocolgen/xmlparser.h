/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.og>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef XMLPARSER_H
#define XMLPARSER_H

#include <QXmlStreamReader>

#include <memory>

class Node;
class DocumentNode;
class EnumNode;
class ClassNode;
class PropertyNode;

class XmlParser
{
public:
    explicit XmlParser();
    ~XmlParser();

    bool parse(const QString &filename);

    Node const *tree() const;

private:
    bool parseProtocol();
    bool parseCommand(DocumentNode *parent);
    bool parseEnum(ClassNode *parent);
    bool parseEnumValue(EnumNode *parent);
    bool parseParam(ClassNode *parent);
    bool parseCtor(ClassNode *parent);
    bool parseSetter(PropertyNode *parent);

    void printError(const QString &error);
private:
    QXmlStreamReader mReader;
    std::unique_ptr<Node> mTree;

};

#endif // XMLPARSER_H
