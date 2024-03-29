/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.og>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "nodetree.h"
#include "cpphelper.h"
#include "typehelper.h"

namespace
{

QString qualifiedName(const Node *node)
{
    if (node->type() == Node::Class) {
        return static_cast<const ClassNode *>(node)->className();
    }
    if (node->type() == Node::Enum) {
        auto enumNode = static_cast<const EnumNode *>(node);
        if (enumNode->enumType() == EnumNode::TypeFlag) {
            return QStringLiteral("%1::%2").arg(qualifiedName(node->parent()), enumNode->flagsName());
        }
        return QStringLiteral("%1::%2").arg(qualifiedName(node->parent()), enumNode->name());
    }

    Q_ASSERT_X(false, "qualifiedName", "Invalid node type");
    return {};
}

} // namespace

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

const QList<Node const *> &Node::children() const
{
    return mChildren;
}

DocumentNode::DocumentNode(int version)
    : Node(Document, nullptr)
    , mVersion(version)
{
}

int DocumentNode::version() const
{
    return mVersion;
}

ClassNode::ClassNode(const QString &name, ClassType type, DocumentNode *parent)
    : Node(Node::Class, parent)
    , mName(name)
    , mClassType(type)
{
}

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
    case Class:
        return mName;
    case Command:
        return mName + QStringLiteral("Command");
    case Response:
        return mName + QStringLiteral("Response");
    case Notification:
        return mName + QStringLiteral("Notification");
    default:
        Q_ASSERT(false);
        return QString();
    }
}

QString ClassNode::parentClassName() const
{
    switch (mClassType) {
    case Class:
        return QString();
    case Command:
        return QStringLiteral("Command");
    case Response:
        return QStringLiteral("Response");
    case Notification:
        return QStringLiteral("ChangeNotification");
    case Invalid:
        Q_ASSERT(false);
        return QString();
    }
    Q_UNREACHABLE();
}

ClassNode::ClassType ClassNode::elementNameToType(QStringView name)
{
    if (name == QLatin1StringView("class")) {
        return Class;
    } else if (name == QLatin1StringView("command")) {
        return Command;
    } else if (name == QLatin1StringView("response")) {
        return Response;
    } else if (name == QLatin1StringView("notification")) {
        return Notification;
    } else {
        return Invalid;
    }
}

QList<PropertyNode const *> ClassNode::properties() const
{
    QList<const PropertyNode *> rv;
    for (const auto node : std::as_const(mChildren)) {
        if (node->type() == Node::Property) {
            rv << static_cast<PropertyNode const *>(node);
        }
    }
    CppHelper::sortMembers(rv);
    return rv;
}

CtorNode::CtorNode(const QList<Argument> &args, ClassNode *parent)
    : Node(Ctor, parent)
    , mArgs(args)
{
}

CtorNode::~CtorNode()
{
}

QList<CtorNode::Argument> CtorNode::arguments() const
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
{
}

QString EnumNode::name() const
{
    return mName;
}

EnumNode::EnumType EnumNode::enumType() const
{
    return mEnumType;
}

QString EnumNode::flagsName() const
{
    if (mEnumType == TypeFlag) {
        return mName + QStringLiteral("s");
    }

    return {};
}

EnumNode::EnumType EnumNode::elementNameToType(QStringView name)
{
    if (name == QLatin1StringView("enum")) {
        return TypeEnum;
    } else if (name == QLatin1StringView("flag")) {
        return TypeFlag;
    } else {
        return TypeInvalid;
    }
}

EnumValueNode::EnumValueNode(const QString &name, EnumNode *parent)
    : Node(EnumValue, parent)
    , mName(name)
    , mValue()
{
}

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
{
}

PropertyNode::~PropertyNode()
{
    delete mSetter;
}

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

bool PropertyNode::isEnum() const
{
    auto parentClass = static_cast<ClassNode *>(parent());
    for (const auto node : parentClass->children()) {
        if (node->type() == Node::Enum) {
            const auto enumNode = static_cast<const EnumNode *>(node);
            if (qualifiedName(enumNode) == mType) {
                return true;
            }
        }
    }

    return false;
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
    return QStringLiteral("m") + mName[0].toUpper() + QStringView(mName).mid(1);
}

QString PropertyNode::setterName() const
{
    return QStringLiteral("set") + mName[0].toUpper() + QStringView(mName).mid(1);
}
