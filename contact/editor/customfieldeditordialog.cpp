/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "customfieldeditordialog.h"

#include <kcombobox.h>
#include <klineedit.h>
#include <klocalizedstring.h>

#include <QCheckBox>
#include <QFormLayout>
#include <QRegExpValidator>

CustomFieldEditorDialog::CustomFieldEditorDialog(QWidget *parent)
    : KDialog(parent)
{
    setCaption(i18n("Edit Custom Field"));
    setButtons(Ok | Cancel | Details);

    QWidget *widget = new QWidget(this);
    setMainWidget(widget);

    QFormLayout *layout = new QFormLayout(widget);

    mKey = new KLineEdit;
    mTitle = new KLineEdit;
    mType = new KComboBox;
    mScope = new QCheckBox(i18n("Use field for all contacts"));

    layout->addRow(i18nc("The title of a custom field", "Title"), mTitle);
    layout->addRow(i18nc("The type of a custom field", "Type"), mType);
    layout->addRow(QString(), mScope);

    QWidget *detailsWidget = new QWidget;
    QFormLayout *detailsLayout = new QFormLayout(detailsWidget);
    detailsLayout->addRow(i18n("Key"), mKey);

    setDetailsWidget(detailsWidget);
    setButtonText(Details, i18nc("@label Opens the advanced dialog", "Advanced"));

    mType->addItem(i18n("Text"), CustomField::TextType);
    mType->addItem(i18n("Numeric"), CustomField::NumericType);
    mType->addItem(i18n("Boolean"), CustomField::BooleanType);
    mType->addItem(i18n("Date"), CustomField::DateType);
    mType->addItem(i18n("Time"), CustomField::TimeType);
    mType->addItem(i18n("DateTime"), CustomField::DateTimeType);
    mType->addItem(i18n("Url"), CustomField::UrlType);

    mKey->setValidator(new QRegExpValidator(QRegExp(QStringLiteral("[a-zA-Z0-9\\-]+")), this));
    mTitle->setFocus();
}

void CustomFieldEditorDialog::setCustomField(const CustomField &field)
{
    mCustomField = field;

    mKey->setText(mCustomField.key());
    mTitle->setText(mCustomField.title());
    mType->setCurrentIndex(mType->findData(mCustomField.type()));
    mScope->setChecked((mCustomField.scope() == CustomField::GlobalScope));
}

CustomField CustomFieldEditorDialog::customField() const
{
    CustomField customField(mCustomField);

    customField.setKey(mKey->text());
    customField.setTitle(mTitle->text());
    customField.setType(static_cast<CustomField::Type>(mType->itemData(mType->currentIndex()).toInt()));

    if (customField.scope() != CustomField::ExternalScope) {
        // do not change the scope for externally defined custom fields
        customField.setScope(mScope->isChecked() ? CustomField::GlobalScope : CustomField::LocalScope);
    }

    return customField;
}
