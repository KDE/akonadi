/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.og>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CPPGENERATOR_H
#define CPPGENERATOR_H

#include <QTextStream>
#include <QFile>

class Node;
class DocumentNode;
class ClassNode;
class EnumNode;
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
