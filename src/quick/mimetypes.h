// SPDX-FileCopyrightText: 2007-2009 Tobias Koenig <tokoe@kde.org>
// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QObject>
#include <qqmlregistration.h>

namespace Akonadi
{
namespace Quick
{
class MimeTypes : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString calendar READ calendar CONSTANT)
    Q_PROPERTY(QString todo READ todo CONSTANT)
    Q_PROPERTY(QString address READ address CONSTANT)
    Q_PROPERTY(QString contactGroup READ contactGroup CONSTANT)
    Q_PROPERTY(QString mail READ mail CONSTANT)

public:
    explicit MimeTypes(QObject *parent = nullptr);
    [[nodiscard]] QString calendar() const;
    [[nodiscard]] QString todo() const;
    [[nodiscard]] QString address() const;
    [[nodiscard]] QString contactGroup() const;
    [[nodiscard]] QString mail() const;
    [[nodiscard]] QString journal() const;
};
}
}
