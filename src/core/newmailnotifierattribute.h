/*
    Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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

#ifndef NEWMAILNOTIFIERATTRIBUTE_H
#define NEWMAILNOTIFIERATTRIBUTE_H

#include "akonadicore_export.h"
#include <attribute.h>

namespace Akonadi
{

class NewMailNotifierAttributePrivate;
class AKONADICORE_EXPORT NewMailNotifierAttribute : public Akonadi::Attribute
{
public:
    NewMailNotifierAttribute();
    ~NewMailNotifierAttribute();

    /* reimpl */
    NewMailNotifierAttribute *clone() const Q_DECL_OVERRIDE;
    QByteArray type() const Q_DECL_OVERRIDE;
    QByteArray serialized() const Q_DECL_OVERRIDE;
    void deserialize(const QByteArray &data) Q_DECL_OVERRIDE;

    bool ignoreNewMail() const;
    void setIgnoreNewMail(bool b);
    bool operator==(const NewMailNotifierAttribute &other) const;

private:
    friend class NewMailNotifierAttributePrivate;
    NewMailNotifierAttributePrivate *const d;
};
}

#endif // NEWMAILNOTIFIERATTRIBUTE_H
