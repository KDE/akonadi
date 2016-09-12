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

#ifndef CPPGENERATOR_H
#define CPPGENERATOR_H

#include <QTextStream>
#include <QFile>
#include <QVector>

class Node;
class DocumentNode;
class ClassNode;
class EnumNode;
class EnumValueNode;
class PropertyNode;

class CppGenerator
{
public:
    explicit CppGenerator();
    ~CppGenerator();

    bool generate(Node const *node);

private:
    bool generateDocument(DocumentNode const *node);
    bool generateClass(ClassNode const *node);

private:
    void writeHeaderHeader(DocumentNode const *node);
    void writeHeaderFooter(DocumentNode const *node);
    void writeHeaderClass(ClassNode const *node);
    void writeHeaderEnum(EnumNode const *node);

    void writeImplHeader(DocumentNode const *node);
    void writeImplFooter(DocumentNode const *node);
    void writeImplSerializer(DocumentNode const *node);
    void writeImplClass(ClassNode const *node);
    void writeImplSerializer(PropertyNode const *node,
                             const char *streamingOperator);

    void writeImplPropertyDependencies(PropertyNode const *node);

private:
    QFile mHeaderFile;
    QTextStream mHeader;
    QFile mImplFile;
    QTextStream mImpl;
};

#endif // CPPGENERATOR_H
