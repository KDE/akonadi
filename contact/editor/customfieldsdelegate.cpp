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

#include "customfieldsdelegate.h"

#include "customfieldsmodel.h"

#include <kicon.h>
#include <klocalizedstring.h>

#include <QDateEdit>
#include <QDateTimeEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QTimeEdit>

CustomFieldsDelegate::CustomFieldsDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

CustomFieldsDelegate::~CustomFieldsDelegate()
{
}

QWidget *CustomFieldsDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &item, const QModelIndex &index) const
{
    if (index.column() == 1) {
        const CustomField::Type type = static_cast<CustomField::Type>(index.data(CustomFieldsModel::TypeRole).toInt());

        switch (type) {
        case CustomField::TextType:
        case CustomField::UrlType:
        default:
            return QStyledItemDelegate::createEditor(parent, item, index);
            break;
        case CustomField::NumericType: {
            QSpinBox *editor = new QSpinBox(parent);
            editor->setFrame(false);
            editor->setAutoFillBackground(true);
            return editor;
            break;
        }
        case CustomField::BooleanType: {
            QCheckBox *editor = new QCheckBox(parent);
            return editor;
            break;
        }
        case CustomField::DateType: {
            QDateEdit *editor = new QDateEdit(parent);
            editor->setFrame(false);
            editor->setAutoFillBackground(true);
            return editor;
            break;
        }
        case CustomField::TimeType: {
            QTimeEdit *editor = new QTimeEdit(parent);
            editor->setFrame(false);
            editor->setAutoFillBackground(true);
            return editor;
            break;
        }
        case CustomField::DateTimeType: {
            QDateTimeEdit *editor = new QDateTimeEdit(parent);
            editor->setFrame(false);
            editor->setAutoFillBackground(true);
            return editor;
            break;
        }
        }
    } else {
        return QStyledItemDelegate::createEditor(parent, item, index);
    }
}

void CustomFieldsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() == 1) {
        const CustomField::Type type = static_cast<CustomField::Type>(index.data(CustomFieldsModel::TypeRole).toInt());

        switch (type) {
        case CustomField::TextType:
        case CustomField::UrlType:
            QStyledItemDelegate::setEditorData(editor, index);
            break;
        case CustomField::NumericType: {
            QSpinBox *widget = qobject_cast<QSpinBox *>(editor);
            widget->setValue(index.data(Qt::EditRole).toInt());
            break;
        }
        case CustomField::BooleanType: {
            QCheckBox *widget = qobject_cast<QCheckBox *>(editor);
            widget->setChecked(index.data(Qt::EditRole).toString() == QLatin1String("true"));
            break;
        }
        case CustomField::DateType: {
            QDateEdit *widget = qobject_cast<QDateEdit *>(editor);
            widget->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
            widget->setDate(QDate::fromString(index.data(Qt::EditRole).toString(), Qt::ISODate));
            break;
        }
        case CustomField::TimeType: {
            QTimeEdit *widget = qobject_cast<QTimeEdit *>(editor);
            widget->setDisplayFormat(QStringLiteral("hh:mm"));
            widget->setTime(QTime::fromString(index.data(Qt::EditRole).toString(), Qt::ISODate));
            break;
        }
        case CustomField::DateTimeType: {
            QDateTimeEdit *widget = qobject_cast<QDateTimeEdit *>(editor);
            widget->setDisplayFormat(QStringLiteral("dd.MM.yyyy hh:mm"));
            widget->setDateTime(QDateTime::fromString(index.data(Qt::EditRole).toString(), Qt::ISODate));
            break;
        }
        }
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void CustomFieldsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (index.column() == 1) {
        const CustomField::Type type = static_cast<CustomField::Type>(index.data(CustomFieldsModel::TypeRole).toInt());

        switch (type) {
        case CustomField::TextType:
        case CustomField::UrlType:
            QStyledItemDelegate::setModelData(editor, model, index);
            break;
        case CustomField::NumericType: {
            QSpinBox *widget = qobject_cast<QSpinBox *>(editor);
            model->setData(index, QString::number(widget->value()));
            break;
        }
        case CustomField::BooleanType: {
            QCheckBox *widget = qobject_cast<QCheckBox *>(editor);
            model->setData(index, widget->isChecked() ? QStringLiteral("true") : QStringLiteral("false"));
            break;
        }
        case CustomField::DateType: {
            QDateEdit *widget = qobject_cast<QDateEdit *>(editor);
            model->setData(index, widget->date().toString(Qt::ISODate));
            break;
        }
        case CustomField::TimeType: {
            QTimeEdit *widget = qobject_cast<QTimeEdit *>(editor);
            model->setData(index, widget->time().toString(Qt::ISODate));
            break;
        }
        case CustomField::DateTimeType: {
            QDateTimeEdit *widget = qobject_cast<QDateTimeEdit *>(editor);
            model->setData(index, widget->dateTime().toString(Qt::ISODate));
            break;
        }
        }
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

void CustomFieldsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //TODO: somehow mark local/global/external fields
    QStyledItemDelegate::paint(painter, option, index);
}
