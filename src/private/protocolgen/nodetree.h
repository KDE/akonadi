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

#ifndef NODETREE_H
#define NODETREE_H

#include <QVector>
#include <QMultiMap>

class Node
{
public:
    enum NodeType {
        Document,
        Class,
        Ctor,
        Enum,
        EnumValue,
        Property
    };

    Node(NodeType type, Node *parent);
    ~Node();

    NodeType type() const;
    Node *parent() const;

    void appendNode(Node *child);

    const QVector<Node const *> &children() const;

protected:
    Node *mParent;
    QVector<Node const *> mChildren;
    NodeType mType;
};



class DocumentNode : public Node
{
public:
    DocumentNode(int version);

    int version() const;

private:
    int mVersion;
};

class PropertyNode;
class ClassNode : public Node
{
public:
    enum ClassType {
        Invalid,
        Class,
        Command,
        Response,
        Notification
    };

    ClassNode(const QString &name, ClassType type, DocumentNode *parent);
    QString name() const;
    ClassType classType() const;
    QString className() const;
    QString parentClassName() const;
    QVector<PropertyNode const *> properties() const;

    static ClassType elementNameToType(const QStringRef &name);

private:
    QString mName;
    ClassType mClassType;
};


class CtorNode : public Node
{
public:
    struct Argument {
        QString name;
        QString type;
        QString defaultValue;

        QString mVariableName() const
        {
            return QStringLiteral("m") + name[0].toUpper() + name.midRef(1);
        }
    };

    CtorNode(const QVector<Argument> &args, ClassNode *parent);
    ~CtorNode();

    QVector<Argument> arguments() const;
    void setArgumentType(const QString &name, const QString &type);

private:
    QVector<Argument> mArgs;
};


class EnumNode : public Node
{
public:
    enum EnumType {
        TypeInvalid,
        TypeEnum,
        TypeFlag
    };

    EnumNode(const QString &name, EnumType type, ClassNode *parent);

    QString name() const;
    EnumType enumType() const;

    static EnumType elementNameToType(const QStringRef &name);

private:
    QString mName;
    EnumType mEnumType;
};


class EnumValueNode : public Node
{
public:
    EnumValueNode(const QString &name, EnumNode *parent);

    QString name() const;
    void setValue(int value);
    int value() const;

private:
    QString mName;
    int mValue;
};


class PropertyNode : public Node
{
public:
    struct Setter {
        QString name;
        QString type;
        QString append;
        QString remove;
    };

    PropertyNode(const QString &name, const QString &type, ClassNode *parent);

    QString type() const;
    QString name() const;

    void setDefaultValue(const QString &defaultValue);
    QString defaultValue() const;

    bool readOnly() const;
    void setReadOnly(bool readOnly);

    bool asReference() const;
    void setAsReference(bool asReference);

    QMultiMap<QString, QString> dependencies() const;
    void addDependency(const QString &enumVar, const QString &enumValue);

    Setter *setter() const;
    void setSetter(Setter *setter);

    QString mVariableName() const;
    QString setterName() const;

private:
    QString mName;
    QString mType;
    QString mDefaultValue;
    QMultiMap<QString, QString> mDepends;
    Setter *mSetter;
    bool mReadOnly;
    bool mAsReference;
};

#endif // NODETREE_H
