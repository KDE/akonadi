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
