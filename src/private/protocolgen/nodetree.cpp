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


#include "nodetree.h"
#include "cpphelper.h"
#include "typehelper.h"

Node::Node(NodeType type, Node *parent)
    : mParent(parent)
    , mType(type)
{
    if (parent) {
        parent->appendNode(this);
    }
}

Node::~Node()
{
    qDeleteAll(mChildren);
}

Node::NodeType Node::type() const
{
    return mType;
}

Node *Node::parent() const
{
    return mParent;
}

void Node::appendNode(Node *child)
{
    child->mParent = this;
    mChildren.push_back(child);
}

const QVector<Node const *> &Node::children() const
{
    return mChildren;
}



DocumentNode::DocumentNode(int version)
    : Node(Document, nullptr)
    , mVersion(version)
{}

int DocumentNode::version() const
{
    return mVersion;
}






ClassNode::ClassNode(const QString &name, ClassType type, DocumentNode *parent)
    : Node(Node::Class, parent)
    , mName(name)
    , mClassType(type)
{}

QString ClassNode::name() const
{
    return mName;
}

ClassNode::ClassType ClassNode::classType() const
{
    return mClassType;
}

QString ClassNode::className() const
{
    switch (mClassType) {
    case Class: return mName;
    case Command: return mName + QStringLiteral("Command");
    case Response: return mName + QStringLiteral("Response");
    case Notification: return mName + QStringLiteral("Notification");
    default: Q_ASSERT(false); return QString();
    }
}

QString ClassNode::parentClassName() const
{
    switch (mClassType) {
    case Class: return QString();
    case Command: return QStringLiteral("Command");
    case Response: return QStringLiteral("Response");
    case Notification: return QStringLiteral("ChangeNotification");
    case Invalid: Q_ASSERT(false); return QString();
    }
    Q_UNREACHABLE();
}

ClassNode::ClassType ClassNode::elementNameToType(const QStringRef &name)
{
    if (name == QLatin1String("class")) {
        return Class;
    } else if (name == QLatin1String("command")) {
        return Command;
    } else if (name == QLatin1String("response")) {
        return Response;
    } else if (name == QLatin1String("notification")) {
        return Notification;
    } else {
        return Invalid;
    }
}

QVector<PropertyNode const *> ClassNode::properties() const
{
    QVector<const PropertyNode *> rv;
    for (auto node : qAsConst(mChildren)) {
        if (node->type() == Node::Property) {
            rv << static_cast<PropertyNode const *>(node);
        }
    }
    CppHelper::sortMembers(rv);
    return rv;
}





CtorNode::CtorNode(const QVector<Argument> &args, ClassNode *parent)
    : Node(Ctor, parent)
    , mArgs(args)
{}

CtorNode::~CtorNode()
{}

QVector<CtorNode::Argument> CtorNode::arguments() const
{
    return mArgs;
}

void CtorNode::setArgumentType(const QString &name, const QString &type)
{
    for (auto &arg : mArgs) {
        if (arg.name == name) {
            arg.type = type;
            break;
        }
    }
}




EnumNode::EnumNode(const QString &name, EnumType type, ClassNode *parent)
    : Node(Enum, parent)
    , mName(name)
    , mEnumType(type)
{}

QString EnumNode::name() const
{
    return mName;
}

EnumNode::EnumType EnumNode::enumType() const
{
    return mEnumType;
}

EnumNode::EnumType EnumNode::elementNameToType(const QStringRef &name)
{
    if (name == QLatin1String("enum")) {
        return TypeEnum;
    } else if (name == QLatin1String("flag")) {
        return TypeFlag;
    } else {
        return TypeInvalid;
    }
}



EnumValueNode::EnumValueNode(const QString &name, EnumNode *parent)
    : Node(EnumValue, parent)
    , mName(name)
    , mValue()
{}

QString EnumValueNode::name() const
{
    return mName;
}

void EnumValueNode::setValue(const QString &value)
{
    mValue = value;
}

QString EnumValueNode::value() const
{
    return mValue;
}



PropertyNode::PropertyNode(const QString &name, const QString &type, ClassNode *parent)
    : Node(Property, parent)
    , mName(name)
    , mType(type)
    , mSetter(nullptr)
    , mReadOnly(false)
    , mAsReference(false)
{}

QString PropertyNode::type() const
{
    return mType;
}

QString PropertyNode::name() const
{
    return mName;
}

void PropertyNode::setDefaultValue(const QString &defaultValue)
{
    mDefaultValue = defaultValue;
}

QString PropertyNode::defaultValue() const
{
    return mDefaultValue;
}

bool PropertyNode::readOnly() const
{
    return mReadOnly;
}

void PropertyNode::setReadOnly(bool readOnly)
{
    mReadOnly = readOnly;
}

bool PropertyNode::asReference() const
{
    return mAsReference;
}

void PropertyNode::setAsReference(bool asReference)
{
    mAsReference = asReference;
}

bool PropertyNode::isPointer() const
{
    return TypeHelper::isPointerType(mType);
}

QMultiMap<QString, QString> PropertyNode::dependencies() const
{
    return mDepends;
}

void PropertyNode::addDependency(const QString &enumVar, const QString &enumValue)
{
    mDepends.insert(enumVar, enumValue);
}

void PropertyNode::setSetter(Setter *setter)
{
    mSetter = setter;
}

PropertyNode::Setter *PropertyNode::setter() const
{
    return mSetter;
}

QString PropertyNode::mVariableName() const
{
    return QStringLiteral("m") + mName[0].toUpper() + mName.midRef(1);
}

QString PropertyNode::setterName() const
{
    return QStringLiteral("set") + mName[0].toUpper() + mName.midRef(1);
}
