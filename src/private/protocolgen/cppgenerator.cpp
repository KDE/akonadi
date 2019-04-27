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

#include "cppgenerator.h"
#include "nodetree.h"
#include "typehelper.h"
#include "cpphelper.h"

#include <QDebug>

#include <iostream>

CppGenerator::CppGenerator()
{
}

CppGenerator::~CppGenerator()
{
}

bool CppGenerator::generate(Node const *node)
{
    Q_ASSERT(node->type() == Node::Document);

    mHeaderFile.setFileName(QStringLiteral("protocol_gen.h"));
    if (!mHeaderFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        std::cerr << qPrintable(mHeaderFile.errorString()) << std::endl;
        return false;
    }
    mHeader.setDevice(&mHeaderFile);

    mImplFile.setFileName(QStringLiteral("protocol_gen.cpp"));
    if (!mImplFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        std::cerr << qPrintable(mImplFile.errorString()) << std::endl;
        return false;
    }
    mImpl.setDevice(&mImplFile);

    return generateDocument(static_cast<DocumentNode const *>(node));
}

void CppGenerator::writeHeaderHeader(DocumentNode const *node)
{
    mHeader << "// This is an auto-generated file.\n"
               "// Any changes to this file will be overwritten\n"
               "\n"
               "namespace Akonadi {\n"
               "namespace Protocol {\n"
               "\n"
               "AKONADIPRIVATE_EXPORT int version();\n"
               "\n";

    // Forward declarations
    for (auto child : qAsConst(node->children())) {
        if (child->type() == Node::Class) {
            mHeader << "class " << static_cast<const ClassNode*>(child)->className() << ";\n";
        }
    }

    mHeader << "\n"
               "} // namespace Protocol\n"
               "} // namespace Akonadi\n\n";
}

void CppGenerator::writeHeaderFooter(DocumentNode const * /*node*/)
{
    // Nothing to do
}

void CppGenerator::writeImplHeader(DocumentNode const *node)
{
    mImpl << "// This is an auto-generated file.\n"
             "// Any changes to this file will be overwritten\n"
             "\n"
             "namespace Akonadi {\n"
             "namespace Protocol {\n"
             "\n"
             "int version()\n"
             "{\n"
             "    return " << node->version() << ";\n"
             "}\n"
             "\n";
}

void CppGenerator::writeImplFooter(DocumentNode const *)
{
    mImpl << "} // namespace Protocol\n"
             "} // namespace Akonadi\n";
}


bool CppGenerator::generateDocument(DocumentNode  const *node)
{
    writeHeaderHeader(node);
    writeImplHeader(node);

    writeImplSerializer(node);

    for (auto classNode : node->children()) {
        if (!generateClass(static_cast<ClassNode const *>(classNode))) {
            return false;
        }
    }

    writeHeaderFooter(node);
    writeImplFooter(node);

    return true;
}

void CppGenerator::writeImplSerializer(DocumentNode  const *node)
{
    mImpl << "void serialize(QIODevice *device, const CommandPtr &cmd)\n"
             "{\n"
             "    DataStream stream(device);\n"
             "    switch (static_cast<int>(cmd->type() | (cmd->isResponse() ? Command::_ResponseBit : 0))) {\n"
             "    case Command::Invalid:\n"
             "        stream << cmdCast<Command>(cmd);\n"
             "        break;\n"
             "    case Command::Invalid | Command::_ResponseBit:\n"
             "        stream << cmdCast<Response>(cmd);\n"
             "        break;\n";
    for (auto child : qAsConst(node->children())) {
        auto classNode = static_cast<ClassNode const *>(child);
        if (classNode->classType() == ClassNode::Response) {
            mImpl << "    case Command::" << classNode->name() << " | Command::_ResponseBit:\n"
                     "        stream << cmdCast<" << classNode->className() << ">(cmd);\n"
                     "        break;\n";
        } else if (classNode->classType() == ClassNode::Command) {
            mImpl << "    case Command::" << classNode->name() << ":\n"
                     "        stream << cmdCast<" << classNode->className() << ">(cmd);\n"
                     "        break;\n";
        } else if (classNode->classType() == ClassNode::Notification) {
            mImpl << "    case Command::" << classNode->name() << "Notification:\n"
                     "        stream << cmdCast<" << classNode->className() << ">(cmd);\n"
                     "        break;\n";
        }
    }
    mImpl << "    }\n"
             "}\n\n";

    mImpl << "CommandPtr deserialize(QIODevice *device)\n"
             "{\n"
             "    DataStream stream(device);\n"
             "    stream.waitForData(sizeof(Command::Type));\n"
             "    Command::Type cmdType;\n"
             "    if (Q_UNLIKELY(device->peek((char *) &cmdType, sizeof(Command::Type)) != sizeof(Command::Type))) {\n"
             "        throw ProtocolException(\"Failed to peek command type\");\n"
             "    }\n"
             "    CommandPtr cmd;\n"
             "    if (cmdType & Command::_ResponseBit) {\n"
             "        cmd = Factory::response(Command::Type(cmdType & ~Command::_ResponseBit));\n"
             "    } else {\n"
             "        cmd = Factory::command(cmdType);\n"
             "    }\n\n"
             "    switch (static_cast<int>(cmdType)) {\n"
             "    case Command::Invalid:\n"
             "        stream >> cmdCast<Command>(cmd);\n"
             "        return cmd;\n"
             "    case Command::Invalid | Command::_ResponseBit:\n"
             "        stream >> cmdCast<Response>(cmd);\n"
             "        return cmd;\n";
    for (auto child : qAsConst(node->children())) {
        auto classNode = static_cast<ClassNode const *>(child);
        if (classNode->classType() == ClassNode::Response) {
            mImpl << "    case Command::" << classNode->name() << " | Command::_ResponseBit:\n"
                     "        stream >> cmdCast<" << classNode->className() << ">(cmd);\n"
                     "        return cmd;\n";
        } else if (classNode->classType() == ClassNode::Command) {
            mImpl << "    case Command::" << classNode->name() << ":\n"
                     "        stream >> cmdCast<" << classNode->className() << ">(cmd);\n"
                     "        return cmd;\n";
        } else if (classNode->classType() == ClassNode::Notification) {
            mImpl << "    case Command::" << classNode->name() << "Notification:\n"
                     "        stream >> cmdCast<" << classNode->className() << ">(cmd);\n"
                     "        return cmd;\n";
        }
    }
    mImpl << "    }\n"
             "    return CommandPtr::create();\n"
             "}\n"
             "\n";


    mImpl << "QString debugString(const Command &cmd)\n"
             "{\n"
             "    QString out;\n"
             "    switch (static_cast<int>(cmd.type() | (cmd.isResponse() ? Command::_ResponseBit : 0))) {\n"
             "    case Command::Invalid:\n"
             "        QDebug(&out).noquote() << static_cast<const Command &>(cmd);\n"
             "        return out;\n"
             "    case Command::Invalid | Command::_ResponseBit:\n"
             "        QDebug(&out).noquote() << static_cast<const Response &>(cmd);\n"
             "        return out;\n";
    for (auto child : qAsConst(node->children())) {
        auto classNode = static_cast<ClassNode const *>(child);
        if (classNode->classType() == ClassNode::Response) {
            mImpl << "    case Command::" << classNode->name() << " | Command::_ResponseBit:\n"
                     "        QDebug(&out).noquote() << static_cast<const " << classNode->className() << " &>(cmd);\n"
                     "        return out;\n";
        } else if (classNode->classType() == ClassNode::Command) {
            mImpl << "    case Command::" << classNode->name() << ":\n"
                     "        QDebug(&out).noquote() << static_cast<const " << classNode->className() << " &>(cmd);\n"
                     "        return out;\n";
        } else if (classNode->classType() == ClassNode::Notification) {
            mImpl << "    case Command::" << classNode->name() << "Notification:\n"
                     "        QDebug(&out).noquote() << static_cast<const " << classNode->className() << " &>(cmd);\n"
                     "        return out;\n";
        }
    }
    mImpl << "    }\n"
             "    return QString();\n"
             "}\n"
             "\n";
}


void CppGenerator::writeHeaderEnum(EnumNode const *node)
{
    mHeader << "    enum " << node->name() << " {\n";
    for (auto enumChild : node->children()) {
        Q_ASSERT(enumChild->type() == Node::EnumValue);
        const auto valueNode = static_cast<EnumValueNode const *>(enumChild);
        mHeader << "        " << valueNode->name();
        if (!valueNode->value().isEmpty()) {
            mHeader << " = " << valueNode->value();
        }
        mHeader << ",\n";
    }
    mHeader << "    };\n";
    if (node->enumType() == EnumNode::TypeFlag) {
        mHeader << "    Q_DECLARE_FLAGS(" << node->name() << "s, " << node->name() << ")\n\n";
    }
}

void CppGenerator::writeHeaderClass(ClassNode const *node)
{
    // Begin class
    const QString parentClass = node->parentClassName();
    const bool isTypeClass = node->classType() == ClassNode::Class;

    mHeader << "namespace Akonadi {\n"
               "namespace Protocol {\n\n"
               "AKONADIPRIVATE_EXPORT DataStream &operator<<(DataStream &stream, const " << node->className() << " &obj);\n"
               "AKONADIPRIVATE_EXPORT DataStream &operator>>(DataStream &stream, " << node->className() << " &obj);\n"
               "AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, const " << node->className() << " &obj);\n"
               "\n"
               "using " << node->className() << "Ptr = QSharedPointer<" << node->className() << ">;\n"
               "\n";
    if (isTypeClass) {
        mHeader << "class AKONADIPRIVATE_EXPORT " << node->className() << "\n";
    } else {
        mHeader << "class AKONADIPRIVATE_EXPORT " << node->className() << " : public " << parentClass << "\n";
    }
    mHeader << "{\n\n"
               "public:\n";

    // Enums
    for (auto child : node->children()) {
        if (child->type() == Node::Enum) {
            const auto enumNode = static_cast<EnumNode const *>(child);
            writeHeaderEnum(enumNode);
        }
    }

    // Ctors, dtor
    for (auto child : qAsConst(node->children())) {
        if (child->type() == Node::Ctor) {
            const auto ctor = static_cast<const CtorNode*>(child);
            const auto args = ctor->arguments();
            mHeader << "    explicit " << node->className() << "(";
            for (int i = 0; i < args.count(); ++i) {
                const auto &arg = args[i];
                if (TypeHelper::isNumericType(arg.type) || TypeHelper::isBoolType(arg.type)) {
                    mHeader << arg.type << " " << arg.name;
                } else {
                    mHeader << "const " << arg.type << " &" << arg.name;
                }
                if (!arg.defaultValue.isEmpty()) {
                    mHeader << " = " << arg.defaultValue;
                }
                if (i < args.count() - 1) {
                    mHeader << ", ";
                }
            }
            mHeader << ");";
        }
    }

    mHeader << "    " << node->className() << "(const " << node->className() << " &) = default;\n"
               "    " << node->className() << "(" << node->className() << " &&) = default;\n"
               "    ~" << node->className() << "() = default;\n"
               "\n"
               "    " << node->className() << " &operator=(const " << node->className() << " &) = default;\n"
               "    " << node->className() << " &operator=(" << node->className() << " &&) = default;\n"
               "    bool operator==(const " << node->className() << " &other) const;\n"
               "    inline bool operator!=(const " << node->className() << " &other) const { return !operator==(other); }\n";

    // Properties
    for (auto child : node->children()) {
        if (child->type() == Node::Property) {
            const auto prop = static_cast<PropertyNode const *>(child);
            if (prop->asReference()) {
                mHeader << "    inline const " << prop->type() << " &" << prop->name() << "() const { return " << prop->mVariableName() << "; }\n"
                           "    inline " << prop->type() << " &" << prop->name() << "() { return " << prop->mVariableName() << "; }\n";
            } else {
                mHeader << "    inline " << prop->type() << " " << prop->name() << "() const { return " << prop->mVariableName() << "; }\n";
            }
            if (!prop->readOnly()) {
                if (auto setter = prop->setter()) {
                    mHeader << "    void " << setter->name << "(const " << setter->type
                            << " &" << prop->name() << ");\n";
                } else if (!prop->dependencies().isEmpty()) {
                    QString varType;
                    if (TypeHelper::isNumericType(prop->type()) || TypeHelper::isBoolType(prop->type())) {
                        varType = QLatin1Char('(') + prop->type() + QLatin1Char(' ');
                    } else {
                        varType = QLatin1String("(const ") + prop->type() + QLatin1String(" &");
                    }
                    mHeader << "    void " << prop->setterName() << varType << prop->name() << ");\n";
                } else {
                    QString varType;
                    if (TypeHelper::isNumericType(prop->type()) || TypeHelper::isBoolType(prop->type())) {
                        varType = QLatin1Char('(') + prop->type() + QLatin1Char(' ');
                    } else {
                        varType = QLatin1String("(const ") + prop->type() + QLatin1String(" &");
                    }
                    mHeader << "    inline void " << prop->setterName() << varType
                            << prop->name() << ") { " << prop->mVariableName() << " = "
                            << prop->name() << "; }\n";
                    if (!TypeHelper::isNumericType(prop->type()) && !TypeHelper::isBoolType(prop->type())) {
                        mHeader << "    inline void " << prop->setterName() << "(" << prop->type() << " &&" << prop->name() << ") { "
                                << prop->mVariableName() << " = std::move(" << prop->name() << "); }\n";
                    }
                }
            }
            mHeader << "\n";
        }
    }
    mHeader << "    void toJson(QJsonObject &stream) const;\n";

    // End of class
    mHeader << "protected:\n";
    const auto properties = node->properties();
    for (auto prop : properties) {
        mHeader << "    " << prop->type() << " " << prop->mVariableName();
        const auto defaultValue = prop->defaultValue();
        const bool isDefaultValue = !defaultValue.isEmpty();
        const bool isNumeric = TypeHelper::isNumericType(prop->type());
        const bool isBool = TypeHelper::isBoolType(prop->type());
        if (isDefaultValue) {
            mHeader << " = " << defaultValue;
        } else if (isNumeric) {
            mHeader << " = 0";
        } else if (isBool) {
            mHeader << " = false";
        }
        mHeader << ";\n";
    }

    mHeader << "\n"
               "private:\n"
               "    friend AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const Akonadi::Protocol::" << node->className() << " &obj);\n"
               "    friend AKONADIPRIVATE_EXPORT Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Akonadi::Protocol::" << node->className() << " &obj);\n"
               "    friend AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, const Akonadi::Protocol::" << node->className() << " &obj);\n"
               "};\n\n"
               "} // namespace Protocol\n"
               "} // namespace Akonadi\n"
               "\n";
    mHeader << "Q_DECLARE_METATYPE(Akonadi::Protocol::" << node->className() << ")\n\n";
    if (node->classType() != ClassNode::Class) {
        mHeader << "Q_DECLARE_METATYPE(Akonadi::Protocol::" << node->className() << "Ptr)\n\n";
    }
}


void CppGenerator::writeImplSerializer(PropertyNode const *node,
                                       const char *streamingOperator)
{
    const auto deps = node->dependencies();
    if (deps.isEmpty()) {
        mImpl << "    stream " << streamingOperator << " obj." << node->mVariableName() << ";\n";
    } else {
        mImpl << "    if (";
        auto it = deps.cend();
        while (1 + 1 == 2) {
            --it;
            const QString mVar = it.key();
            mImpl << "(obj." << "m" << mVar[0].toUpper() << mVar.midRef(1) << " & " <<  it.value() << ")";
            if (it == deps.cbegin()) {
                break;
            } else {
                mImpl << " && ";
            }
        }
        mImpl << ") {\n"
                 "        stream " << streamingOperator << " obj." << node->mVariableName() << ";\n"
                 "    }\n";
    }
}

void CppGenerator::writeImplClass(ClassNode const *node)
{
    const QString parentClass = node->parentClassName();
    const auto children = node->children();
    const auto properties = node->properties();

    // Ctors
    for (auto child : children) {
        if (child->type() == Node::Ctor) {
            const auto ctor = static_cast<CtorNode const *>(child);
            const auto args = ctor->arguments();
            mImpl << node->className() << "::" << node->className() << "(";
            for (int i = 0; i < args.count(); ++i) {
                const auto &arg = args[i];
                if (TypeHelper::isNumericType(arg.type) || TypeHelper::isBoolType(arg.type)) {
                    mImpl << arg.type << " " << arg.name;
                } else {
                    mImpl << "const " << arg.type << " &" << arg.name;
                }
                if (i < args.count() - 1) {
                    mImpl << ", ";
                }
            }
            mImpl << ")\n";
            char startChar = ',';
            if (!parentClass.isEmpty()) {
                const QString type = node->name() + ((node->classType() == ClassNode::Notification) ? QStringLiteral("Notification") : QString());
                mImpl << "    : " << parentClass << "(Command::" << type << ")\n";
            } else {
                startChar = ':';
            }
            for (auto prop : properties) {
                auto arg = std::find_if(args.cbegin(), args.cend(),
                                        [prop](const CtorNode::Argument &arg) {
                                            return arg.name == prop->name();
                                        });
                if (arg != args.cend()) {
                    mImpl << "    " << startChar << " " << prop->mVariableName() << "(" << arg->name << ")\n";
                    startChar = ',';
                }
            }
            mImpl << "{\n"
                     "}\n"
                     "\n";
        }
    }


    // Comparison operator
    mImpl << "bool " << node->className() << "::operator==(const " << node->className() << " &other) const\n"
             "{\n";
    mImpl << "    return true // simplifies generation\n";
    if (!parentClass.isEmpty()) {
        mImpl << "        && " << parentClass << "::operator==(other)\n";
    }
    for (auto prop : properties) {
        if (prop->isPointer()) {
            mImpl << "        && *" << prop->mVariableName() << " == *other." << prop->mVariableName() << "\n";
        } else if (TypeHelper::isContainer(prop->type())) {
            mImpl << "        && containerComparator(" << prop->mVariableName() << ", other." << prop->mVariableName() << ")\n";
        } else {
            mImpl << "        && " << prop->mVariableName() << " == other." << prop->mVariableName() << "\n";
        }
    }
    mImpl << "    ;\n"
             "}\n"
             "\n";

    // non-trivial setters
    for (auto prop : properties) {
        if (prop->readOnly()) {
            continue;
        }

        if (const auto setter = prop->setter()) {
            mImpl << "void " << node->className() << "::" << setter->name
                    << "(const " << setter->type << " &val)\n"
                        "{\n";
            if (!setter->append.isEmpty()) {
                mImpl << "    m" << setter->append[0].toUpper() << setter->append.midRef(1) << " << val;\n";
            }
            if (!setter->remove.isEmpty()) {
                const QString mVar = QStringLiteral("m") + setter->remove[0].toUpper() + setter->remove.midRef(1);
                mImpl << "    auto it = std::find(" << mVar << ".begin(), " << mVar << ".end(), val);\n"
                        "    if (it != " << mVar << ".end()) {\n"
                        "        " << mVar << ".erase(it);\n"
                        "    }\n";
            }
            writeImplPropertyDependencies(prop);
            mImpl << "}\n\n";
        } else if (!prop->dependencies().isEmpty()) {
            if (TypeHelper::isNumericType(prop->type()) || TypeHelper::isBoolType(prop->type())) {
                mImpl << "void " << node->className() << "::" << prop->setterName()
                      << "(" << prop->type() << " val)\n"
                                                      "{\n"
                                                      "    " << prop->mVariableName() << " = val;\n";

            } else {
                mImpl << "void " << node->className() << "::" << prop->setterName()
                      << "(const " << prop->type() << " &val)\n"
                                                      "{\n"
                                                      "    " << prop->mVariableName() << " = val;\n";
            }
            writeImplPropertyDependencies(prop);
            mImpl << "}\n\n";
        }
    }

    // serialize
    auto serializeProperties = properties;
    CppHelper::sortMembersForSerialization(serializeProperties);

    mImpl << "DataStream &operator<<(DataStream &stream, const " << node->className() << " &obj)\n"
             "{\n";
    if (!parentClass.isEmpty()) {
             mImpl << "    stream << static_cast<const " << parentClass << " &>(obj);\n";
    }
    for (auto prop : qAsConst(serializeProperties)) {
        writeImplSerializer(prop, "<<");
    }
    mImpl << "    return stream;\n"
             "}\n"
             "\n";

    // deserialize
    mImpl << "DataStream &operator>>(DataStream &stream, " << node->className() << " &obj)\n"
             "{\n";
    if (!parentClass.isEmpty()) {
        mImpl << "    stream >> static_cast<" << parentClass << " &>(obj);\n";
    }
    for (auto prop : qAsConst(serializeProperties)) {
        writeImplSerializer(prop, ">>");
    }
    mImpl << "    return stream;\n"
             "}\n"
             "\n";

    // debug
    mImpl << "QDebug operator<<(QDebug dbg, const " << node->className() << " &obj)\n"
             "{\n";
    if (!parentClass.isEmpty()) {
        mImpl << "    dbg.noquote() << static_cast<const " << parentClass << " &>(obj)\n";
    } else {
        mImpl << "    dbg.noquote()\n";
    }

    for (auto prop : qAsConst(serializeProperties)) {
        if (prop->isPointer()) {
            mImpl << "        << \"" << prop->name() << ":\" << *obj." << prop->mVariableName() << " << \"\\n\"\n";
        } else if (TypeHelper::isContainer(prop->type())) {
            mImpl << "        << \"" << prop->name() << ": [\\n\";\n"
                     "    for (const auto &type : qAsConst(obj." << prop->mVariableName() << ")) {\n"
                     "        dbg.noquote() << \"    \" << ";
            if (TypeHelper::isPointerType(TypeHelper::containerType(prop->type()))) {
                mImpl << "*type";
            } else {
                mImpl << "type";
            }
            mImpl << " << \"\\n\";\n"
                     "    }\n"
                     "    dbg.noquote() << \"]\\n\"\n";
        } else {
            mImpl << "        << \"" << prop->name() << ":\" << obj." << prop->mVariableName() << " << \"\\n\"\n";
        }
    }
    mImpl << "    ;\n"
             "    return dbg;\n"
             "}\n"
             "\n";

    // toJson
    mImpl << "void " << node->className() << "::toJson(QJsonObject &json) const\n"
             "{\n";
    if (!parentClass.isEmpty()) {
        mImpl << "    static_cast<const " << parentClass << " *>(this)->toJson(json);\n";
    } else if (serializeProperties.isEmpty()) {
        mImpl << "    Q_UNUSED(json);\n";
    }
    for (auto prop : qAsConst(serializeProperties)) {
        if (prop->isPointer()) {
            mImpl << "    {\n"
                     "        QJsonObject jsonObject;\n"
                     "        " << prop->mVariableName() << "->toJson(jsonObject);\n"
                     "        json[QStringLiteral(\"" << prop->name() << "\")] = jsonObject;\n"
                     "    }\n";
        } else if (TypeHelper::isContainer(prop->type())) {
            const auto &containerType = TypeHelper::containerType(prop->type());
            mImpl << "    {\n"
                     "        QJsonArray jsonArray;\n"
                     "        for (const auto &type : qAsConst(" << prop->mVariableName() << ")) {\n";
            if (TypeHelper::isPointerType(containerType)) {
                mImpl << "            QJsonObject jsonObject;\n"
                         "            type->toJson(jsonObject); /* " << containerType << " */\n"
                         "            jsonArray.append(jsonObject);\n";
            } else if (TypeHelper::isNumericType(containerType)) {
                mImpl << "            jsonArray.append(type); /* "<<  containerType << " */\n";
            } else if (TypeHelper::isBoolType(containerType)) {
                mImpl << "            jsonArray.append(type); /* "<<  containerType << " */\n";
            } else if (containerType == QStringLiteral("QByteArray")) {
                mImpl << "            jsonArray.append(QString::fromUtf8(type)); /* "<<  containerType << "*/\n";
            } else if (TypeHelper::isBuiltInType(containerType)) {
                if (TypeHelper::containerType(prop->type()) == QStringLiteral("Akonadi::Protocol::ChangeNotification::Relation")) {
                    mImpl << "            QJsonObject jsonObject;\n"
                             "            type.toJson(jsonObject); /* " << containerType << " */\n"
                             "            jsonArray.append(jsonObject);\n";
                } else {
                    mImpl << "            jsonArray.append(type); /* "<<  containerType << " */\n";
                }
            } else {
                mImpl << "            QJsonObject jsonObject;\n"
                         "            type.toJson(jsonObject); /* " << containerType << " */\n"
                         "            jsonArray.append(jsonObject);\n";
            }
            mImpl << "        }\n"
                  << "        json[QStringLiteral(\"" << prop->name() << "\")] = jsonArray;\n"
                  << "    }\n";
        } else if (prop->type() == QStringLiteral("uint")) {
            mImpl << "    json[QStringLiteral(\"" << prop->name() << "\")] = static_cast<int>(" <<  prop->mVariableName() << ");/* "<<  prop->type() << " */\n";
        } else if (TypeHelper::isNumericType(prop->type())) {
            mImpl << "    json[QStringLiteral(\"" << prop->name() << "\")] = " <<  prop->mVariableName() << ";/* "<<  prop->type() << " */\n";
        } else if (TypeHelper::isBoolType(prop->type())) {
            mImpl << "    json[QStringLiteral(\"" << prop->name() << "\")] = " <<  prop->mVariableName() << ";/* "<<  prop->type() << " */\n";
        } else if (TypeHelper::isBuiltInType(prop->type())) {
            if (prop->type() == QStringLiteral("QStringList")) {
                mImpl << "    json[QStringLiteral(\"" << prop->name() << "\")] = QJsonArray::fromStringList(" <<  prop->mVariableName() << ");/* "<<  prop->type() << " */\n";
            } else if (prop->type() == QStringLiteral("QDateTime")) {
                mImpl << "    json[QStringLiteral(\"" << prop->name() << "\")] = " <<  prop->mVariableName() << ".toString()/* "<<  prop->type() << " */;\n";
            } else if (prop->type() == QStringLiteral("QByteArray")) {
                mImpl << "    json[QStringLiteral(\"" << prop->name() << "\")] = QString::fromUtf8(" <<  prop->mVariableName() << ")/* "<<  prop->type() << " */;\n";
            } else if (prop->type() == QStringLiteral("Scope")) {
                mImpl << "    {\n"
                         "        QJsonObject jsonObject;\n"
                         "        " << prop->mVariableName() << ".toJson(jsonObject); /* " << prop->type() << " */\n"
                         "        json[QStringLiteral(\"" << prop->name() << "\")] = " << "jsonObject;\n"
                         "    }\n";
            } else if (prop->type() == QStringLiteral("Tristate")) {
                mImpl << "    switch (" << prop->mVariableName() << ") {\n;"
                         "    case Tristate::True:\n"
                         "        json[QStringLiteral(\"" << prop->name() << "\")] = QStringLiteral(\"True\");\n"
                         "        break;\n"
                         "    case Tristate::False:\n"
                         "        json[QStringLiteral(\"" << prop->name() << "\")] = QStringLiteral(\"False\");\n"
                         "        break;\n"
                         "    case Tristate::Undefined:\n"
                         "        json[QStringLiteral(\"" << prop->name() << "\")] = QStringLiteral(\"Undefined\");\n"
                         "        break;\n"
                         "    }\n";
            } else if (prop->type() == QStringLiteral("Akonadi::Protocol::Attributes")) {
               mImpl << "    {\n"
                        "        QJsonObject jsonObject;\n"
                        "        auto i = " << prop->mVariableName() << ".constBegin();\n"
                        "        const auto &end = " << prop->mVariableName() << ".constEnd();\n"
                        "        while (i != end) {\n"
                        "            jsonObject[QString::fromUtf8(i.key())] = QString::fromUtf8(i.value());\n"
                        "            ++i;\n"
                        "        }\n"
                        "        json[QStringLiteral(\"" << prop->name() << "\")] = jsonObject;\n"
                        "    }\n";
            } else if (prop->type() == QStringLiteral("ModifySubscriptionCommand::ModifiedParts") ||
                       prop->type() == QStringLiteral("ModifyTagCommand::ModifiedParts") ||
                       prop->type() == QStringLiteral("ModifyCollectionCommand::ModifiedParts") ||
                       prop->type() == QStringLiteral("ModifyItemsCommand::ModifiedParts") ||
                       prop->type() == QStringLiteral("CreateItemCommand::MergeModes")) {
                mImpl << "    json[QStringLiteral(\"" << prop->name() << "\")] = static_cast<int>(" << prop->mVariableName() << ");/* "<<  prop->type() << "*/\n";
            } else {
                mImpl << "    json[QStringLiteral(\"" << prop->name() << "\")] = " << prop->mVariableName() << ";/* "<<  prop->type() << "*/\n";
            }
        } else {
            mImpl << "    {\n"
                     "         QJsonObject jsonObject;\n"
                     "         " << prop->mVariableName() << ".toJson(jsonObject); /* " << prop->type() << " */\n"
                     "         json[QStringLiteral(\"" << prop->name() << "\")] = jsonObject;\n"
                     "    }\n";
        }
    }
    mImpl << "}\n"
             "\n";
}

void CppGenerator::writeImplPropertyDependencies(const PropertyNode* node)
{
    const auto deps = node->dependencies();
    QString key;
    QStringList values;
    QString enumType;
    for (auto it = deps.cbegin(), end = deps.cend(); it != end; ++it) {
        if (key != it.key()) {
            key = it.key();
            const auto children = node->parent()->children();
            for (auto child : children) {
                if (child->type() == Node::Property && child != node) {
                    auto prop = static_cast<const PropertyNode*>(child);
                    if (prop->name() == key) {
                        enumType = prop->type();
                        break;
                    }
                }
            }
            if (!values.isEmpty()) {
                mImpl << "    m" << key[0].toUpper() << key.midRef(1) << " |= " << enumType << "("
                      << values.join(QStringLiteral(" | ")) << ");\n";
                values.clear();
            }
        }
        values << *it;
    }

    if (!values.isEmpty()) {
        mImpl << "    m" << key[0].toUpper() << key.midRef(1) << " |= " << enumType << "("
              << values.join(QStringLiteral(" | ")) << ");\n";
    }
}

bool CppGenerator::generateClass(ClassNode const *node)
{
    writeHeaderClass(node);

    mImpl << "\n\n/************************* " << node->className() << " *************************/\n\n";
    writeImplClass(node);

    return true;
}
