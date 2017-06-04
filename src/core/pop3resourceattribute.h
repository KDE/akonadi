/*
    Copyright (c) 2013-2017 Montel Laurent <montel@kde.org>

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

#ifndef AKONADICORE_POP3RESOURCEATTRIBUTE_H
#define AKONADICORE_POP3RESOURCEATTRIBUTE_H

#include <attribute.h>
#include "akonadicore_export.h"

namespace Akonadi
{

class Pop3ResourceAttributePrivate;
class AKONADICORE_EXPORT Pop3ResourceAttribute : public Akonadi::Attribute
{
public:
    Pop3ResourceAttribute();
    ~Pop3ResourceAttribute();

    /* reimpl */
    Pop3ResourceAttribute *clone() const override;
    QByteArray type() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

    QString pop3AccountName() const;
    void setPop3AccountName(const QString &accountName);

    bool operator==(const Pop3ResourceAttribute &other) const;
private:
    friend class Pop3ResourceAttributePrivate;
    Pop3ResourceAttributePrivate *const d;
};
}
#endif // AKONADICORE_POP3RESOURCEATTRIBUTE_H
