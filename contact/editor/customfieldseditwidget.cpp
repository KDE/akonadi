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

#include "customfieldseditwidget.h"

#include "customfieldeditordialog.h"
#include "customfieldmanager_p.h"
#include "customfieldsdelegate.h"
#include "customfieldsmodel.h"

#include <kabc/addressee.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>

#include <QtCore/QPointer>
#include <QtCore/QUuid>
#include <QGridLayout>
#include <QPushButton>
#include <QTreeView>
#include <QSortFilterProxyModel>

void splitCustomField(const QString &str, QString &app, QString &name, QString &value)
{
    const int colon = str.indexOf(QLatin1Char(':'));
    if (colon != -1) {
        const QString tmp = str.left(colon);
        value = str.mid(colon + 1);

        const int dash = tmp.indexOf(QLatin1Char('-'));
        if (dash != -1) {
            app = tmp.left(dash);
            name = tmp.mid(dash + 1);
        }
    }
}

CustomFieldsEditWidget::CustomFieldsEditWidget(QWidget *parent)
    : QWidget(parent)
    , mReadOnly(false)
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);

    mView = new QTreeView;
    mView->setSortingEnabled(true);
    mView->setRootIsDecorated(false);
    mView->setItemDelegate(new CustomFieldsDelegate(this));

    mAddButton = new QPushButton(i18n("Add..."));
    mEditButton = new QPushButton(i18n("Edit..."));
    mRemoveButton = new QPushButton(i18n("Remove"));

    layout->addWidget(mView, 0, 0, 4, 1);
    layout->addWidget(mAddButton, 0, 1);
    layout->addWidget(mEditButton, 1, 1);
    layout->addWidget(mRemoveButton, 2, 1);

    mModel = new CustomFieldsModel(this);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel;
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSourceModel(mModel);
    mView->setModel(proxyModel);
    mView->setColumnHidden(2, true);   // hide the 'key' column

    connect(mView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(slotUpdateButtons()));
    connect(mAddButton, SIGNAL(clicked()), this, SLOT(slotAdd()));
    connect(mEditButton, SIGNAL(clicked()), this, SLOT(slotEdit()));
    connect(mRemoveButton, SIGNAL(clicked()), this, SLOT(slotRemove()));
    slotUpdateButtons();
}

CustomFieldsEditWidget::~CustomFieldsEditWidget()
{
}

void CustomFieldsEditWidget::loadContact(const KABC::Addressee &contact)
{
    CustomField::List externalCustomFields;

    CustomField::List globalCustomFields = CustomFieldManager::globalCustomFieldDescriptions();

    const QStringList customs = contact.customs();
    foreach (const QString &custom, customs) {

        QString app, name, value;
        splitCustomField(custom, app, name, value);

        // skip all well-known fields that have separated editor widgets
        if (custom.startsWith(QStringLiteral("messaging/"))) {       // IM addresses
            continue;
        }

        if (app == QLatin1String("KADDRESSBOOK")) {
            static QSet<QString> blacklist;
            if (blacklist.isEmpty()) {
                blacklist << QStringLiteral("BlogFeed")
                          << QStringLiteral("X-IMAddress")
                          << QStringLiteral("X-Profession")
                          << QStringLiteral("X-Office")
                          << QStringLiteral("X-ManagersName")
                          << QStringLiteral("X-AssistantsName")
                          << QStringLiteral("X-Anniversary")
                          << QStringLiteral("X-ANNIVERSARY")
                          << QStringLiteral("X-SpousesName")
                          << QStringLiteral("X-Profession")
                          << QStringLiteral("MailPreferedFormatting")
                          << QStringLiteral("MailAllowToRemoteContent")
                          << QStringLiteral("CRYPTOPROTOPREF")
                          << QStringLiteral("OPENPGPFP")
                          << QStringLiteral("SMIMEFP")
                          << QStringLiteral("CRYPTOSIGNPREF")
                          << QStringLiteral("CRYPTOENCRYPTPREF");
            }

            if (blacklist.contains(name)) {     // several KAddressBook specific fields
                continue;
            }
        }

        // check whether it correspond to a local custom field
        bool isLocalCustomField = false;
        for (int i = 0; i < mLocalCustomFields.count(); ++i) {
            if (mLocalCustomFields[i].key() == name) {
                mLocalCustomFields[i].setValue(value);
                isLocalCustomField = true;
                break;
            }
        }

        // check whether it correspond to a global custom field
        bool isGlobalCustomField = false;
        for (int i = 0; i < globalCustomFields.count(); ++i) {
            if (globalCustomFields[i].key() == name) {
                globalCustomFields[i].setValue(value);
                isGlobalCustomField = true;
                break;
            }
        }

        // if not local and not global it must be external
        if (!isLocalCustomField && !isGlobalCustomField) {
            if (app == QLatin1String("KADDRESSBOOK")) {
                // however if it starts with our prefix it might be that this is an outdated
                // global custom field, in this case treat it as local field of type text
                CustomField customField(name, name, CustomField::TextType, CustomField::LocalScope);
                customField.setValue(value);

                mLocalCustomFields << customField;
            } else {
                // it is really an external custom field
                const QString key = app + QLatin1Char('-') + name;
                CustomField customField(key, key, CustomField::TextType, CustomField::ExternalScope);
                customField.setValue(value);

                externalCustomFields << customField;
            }
        }
    }

    mModel->setCustomFields(CustomField::List() << mLocalCustomFields << globalCustomFields << externalCustomFields);
}

void CustomFieldsEditWidget::storeContact(KABC::Addressee &contact) const
{
    const CustomField::List customFields = mModel->customFields();
    foreach (const CustomField &customField, customFields) {
        // write back values for local and global scope, leave external untouched
        if (customField.scope() != CustomField::ExternalScope) {
            if (!customField.value().isEmpty()) {
                contact.insertCustom(QStringLiteral("KADDRESSBOOK"), customField.key(), customField.value());
            } else {
                contact.removeCustom(QStringLiteral("KADDRESSBOOK"), customField.key());
            }
        }
    }

    // Now remove all fields that were available in loadContact (these are stored in mLocalCustomFields)
    // but are not part of customFields now, which means they have been removed or renamed by the user
    // in the editor dialog.
    foreach (const CustomField &oldCustomField, mLocalCustomFields) {
        if (oldCustomField.scope() != CustomField::ExternalScope) {

            bool fieldStillExists = false;
            foreach (const CustomField &newCustomField, customFields) {
                if (newCustomField.scope() != CustomField::ExternalScope) {
                    if (newCustomField.key() == oldCustomField.key()) {
                        fieldStillExists = true;
                        break;
                    }
                }
            }

            if (!fieldStillExists) {
                contact.removeCustom(QStringLiteral("KADDRESSBOOK"), oldCustomField.key());
            }
        }
    }

    // And store the global custom fields descriptions as well
    CustomField::List globalCustomFields;
    foreach (const CustomField &customField, customFields) {
        if (customField.scope() == CustomField::GlobalScope) {
            globalCustomFields << customField;
        }
    }

    CustomFieldManager::setGlobalCustomFieldDescriptions(globalCustomFields);
}

void CustomFieldsEditWidget::setReadOnly(bool readOnly)
{
    mReadOnly = readOnly;

    mView->setEnabled(!mReadOnly);

    slotUpdateButtons();
}

void CustomFieldsEditWidget::setLocalCustomFieldDescriptions(const QVariantList &descriptions)
{
    mLocalCustomFields.clear();

    foreach (const QVariant &description, descriptions) {
        mLocalCustomFields.append(CustomField::fromVariantMap(description.toMap(), CustomField::LocalScope));
    }
}

QVariantList CustomFieldsEditWidget::localCustomFieldDescriptions() const
{
    const CustomField::List customFields = mModel->customFields();

    QVariantList descriptions;
    foreach (const CustomField &field, customFields) {
        if (field.scope() == CustomField::LocalScope) {
            descriptions.append(field.toVariantMap());
        }
    }

    return descriptions;
}

void CustomFieldsEditWidget::slotAdd()
{
    CustomField field;

    // We use a Uuid as default key, so we won't have any duplicated keys,
    // the user can still change it to something else in the editor dialog.
    // Since the key only allows [A-Za-z0-9\-]*, we have to remove the curly
    // braces as well.
    QString key = QUuid::createUuid().toString();
    key.remove(QLatin1Char('{'));
    key.remove(QLatin1Char('}'));

    field.setKey(key);

    QPointer<CustomFieldEditorDialog> dlg = new CustomFieldEditorDialog(this);
    dlg->setCustomField(field);

    if (dlg->exec() == QDialog::Accepted) {
        const int lastRow = mModel->rowCount();
        mModel->insertRow(lastRow);

        field = dlg->customField();
        mModel->setData(mModel->index(lastRow, 2), field.key(), Qt::EditRole);
        mModel->setData(mModel->index(lastRow, 0), field.title(), Qt::EditRole);
        mModel->setData(mModel->index(lastRow, 0), field.type(), CustomFieldsModel::TypeRole);
        mModel->setData(mModel->index(lastRow, 0), field.scope(), CustomFieldsModel::ScopeRole);
    }

    delete dlg;
}

void CustomFieldsEditWidget::slotEdit()
{
    const QModelIndex currentIndex = mView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    CustomField field;
    field.setKey(mModel->index(currentIndex.row(), 2).data(Qt::DisplayRole).toString());
    field.setTitle(mModel->index(currentIndex.row(), 0).data(Qt::DisplayRole).toString());
    field.setType(static_cast<CustomField::Type>(currentIndex.data(CustomFieldsModel::TypeRole).toInt()));
    field.setScope(static_cast<CustomField::Scope>(currentIndex.data(CustomFieldsModel::ScopeRole).toInt()));

    QPointer<CustomFieldEditorDialog> dlg = new CustomFieldEditorDialog(this);
    dlg->setCustomField(field);

    if (dlg->exec() == QDialog::Accepted) {
        field = dlg->customField();
        mModel->setData(mModel->index(currentIndex.row(), 2), field.key(), Qt::EditRole);
        mModel->setData(mModel->index(currentIndex.row(), 0), field.title(), Qt::EditRole);
        mModel->setData(currentIndex, field.type(), CustomFieldsModel::TypeRole);
        mModel->setData(currentIndex, field.scope(), CustomFieldsModel::ScopeRole);
    }

    delete dlg;
}

void CustomFieldsEditWidget::slotRemove()
{
    const QModelIndex currentIndex = mView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    if (KMessageBox::warningContinueCancel(this,
                                           i18nc("Custom Fields", "Do you really want to delete the selected custom field?"),
                                           i18n("Confirm Delete"), KStandardGuiItem::del()) != KMessageBox::Continue) {
        return;
    }

    mModel->removeRow(currentIndex.row());
}

void CustomFieldsEditWidget::slotUpdateButtons()
{
    const bool hasCurrent = mView->currentIndex().isValid();
    const bool isExternal = (hasCurrent &&
                             (static_cast<CustomField::Scope>(mView->currentIndex().data(CustomFieldsModel::ScopeRole).toInt()) == CustomField::ExternalScope));

    mAddButton->setEnabled(!mReadOnly);
    mEditButton->setEnabled(!mReadOnly && hasCurrent && !isExternal);
    mRemoveButton->setEnabled(!mReadOnly && hasCurrent && !isExternal);
}
