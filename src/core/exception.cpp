/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "exceptionbase.h"

#include <QString>

#include <memory>

using namespace Akonadi;

class Akonadi::ExceptionPrivate
{
public:
    explicit ExceptionPrivate(const QByteArray &what)
        : what(what)
    {
    }

    QByteArray what;
    QByteArray assembledWhat;
};

Exception::Exception(const char *what)
{
    try {
        d = std::make_unique<ExceptionPrivate>(what);
    } catch (...) {
    }
}

Exception::Exception(const QByteArray &what)
{
    try {
        d = std::make_unique<ExceptionPrivate>(what);
    } catch (...) {
    }
}

Exception::Exception(const QString &what)
{
    try {
        d = std::make_unique<ExceptionPrivate>(what.toUtf8());
    } catch (...) {
    }
}

Exception::Exception(Exception &&) noexcept = default;

Exception::~Exception() = default;

QByteArray Exception::type() const
{
    static constexpr char mytype[] = "Akonadi::Exception";
    try {
        return QByteArray::fromRawData("Akonadi::Exception", sizeof(mytype) - 1);
    } catch (...) {
        return QByteArray();
    }
}

const char *Exception::what() const noexcept
{
    static constexpr char fallback[] = "<some exception was thrown during construction: message lost>";
    if (!d) {
        return fallback;
    }
    if (d->assembledWhat.isEmpty()) {
        try {
            d->assembledWhat = QByteArray(type() + ": " + d->what);
        } catch (...) {
            return "caught some exception while assembling Akonadi::Exception::what() return value";
        }
    }
    return d->assembledWhat.constData();
}

#define AKONADI_EXCEPTION_IMPLEMENT_TRIVIAL_INSTANCE(classname)                                                                                                \
    Akonadi::classname::~classname() = default;                                                                                                                \
    QByteArray Akonadi::classname::type() const                                                                                                                \
    {                                                                                                                                                          \
        static constexpr char mytype[] = "Akonadi::" #classname;                                                                                               \
        try {                                                                                                                                                  \
            return QByteArray::fromRawData(mytype, sizeof(mytype) - 1);                                                                                        \
        } catch (...) {                                                                                                                                        \
            return QByteArray();                                                                                                                               \
        }                                                                                                                                                      \
    }

AKONADI_EXCEPTION_IMPLEMENT_TRIVIAL_INSTANCE(PayloadException)

#undef AKONADI_EXCEPTION_IMPLEMENT_TRIVIAL_INSTANCE
