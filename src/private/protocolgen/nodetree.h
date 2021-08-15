/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.og>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QMultiMap>
#include <QVector>

class Node
{
public:
    enum NodeType {
        Document,
        Class,
        Ctor,
        Enum,
        EnumValue,
        Property,
    };

    Node(NodeType type, Node *parent);
    Node(const Node &) = delete;
    Node(Node &&) = delete;
    virtual ~Node();

    Node &operator=(const Node &) = delete;
    Node &operator=(Node &&) = delete;

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
        Notification,
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
            return QStringLiteral("m") + name[0].toUpper() +
                QStringView(name).mid(1);
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
        TypeFlag,
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
    void setValue(const QString &value);
    QString value() const;

private:
    QString mName;
    QString mValue;
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
    ~PropertyNode();

    QString type() const;
    QString name() const;

    void setDefaultValue(const QString &defaultValue);
    QString defaultValue() const;

    bool readOnly() const;
    void setReadOnly(bool readOnly);

    bool asReference() const;
    void setAsReference(bool asReference);

    bool isPointer() const;

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

