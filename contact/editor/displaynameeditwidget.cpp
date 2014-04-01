/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "displaynameeditwidget.h"

#include <QtCore/QEvent>
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QPainter>
#include <QStyledItemDelegate>

#include <kabc/addressee.h>
#include <kcombobox.h>
#include <kdialog.h>
#include <klocalizedstring.h>

// Tries to guess the display type that is used for the passed contact
static DisplayNameEditWidget::DisplayType guessedDisplayType(const KABC::Addressee &contact)
{
    if (contact.formattedName() == (contact.givenName() + QLatin1Char(' ') + contact.familyName())) {
        return DisplayNameEditWidget::SimpleName;
    } else if (contact.formattedName() == contact.assembledName()) {
        return DisplayNameEditWidget::FullName;
    } else if (contact.formattedName() == (contact.familyName() + QStringLiteral(", ") + contact.givenName())) {
        return DisplayNameEditWidget::ReverseNameWithComma;
    } else if (contact.formattedName() == (contact.familyName() + QLatin1Char(' ') + contact.givenName())) {
        return DisplayNameEditWidget::ReverseName;
    } else if (contact.formattedName() == contact.organization()) {
        return DisplayNameEditWidget::Organization;
    } else {
        return DisplayNameEditWidget::CustomName;
    }
}

class DisplayNameDelegate : public QStyledItemDelegate
{
public:
    DisplayNameDelegate(QAbstractItemView *view, QObject *parent = 0)
        : QStyledItemDelegate(parent)
        , mMaxDescriptionWidth(0)
    {
        mDescriptions.append(i18n("Short Name"));
        mDescriptions.append(i18n("Full Name"));
        mDescriptions.append(i18n("Reverse Name with Comma"));
        mDescriptions.append(i18n("Reverse Name"));
        mDescriptions.append(i18n("Organization"));
        mDescriptions.append(i18nc("@item:inlistbox A custom name format", "Custom"));

        QFont font = view->font();
        font.setStyle(QFont::StyleItalic);
        QFontMetrics metrics(font);
        foreach (const QString &description, mDescriptions) {
            mMaxDescriptionWidth = qMax(mMaxDescriptionWidth, metrics.width(description));
        }

        mMaxDescriptionWidth += 3;
    }

    int maximumDescriptionWidth() const
    {
        return mMaxDescriptionWidth;
    }

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyledItemDelegate::paint(painter, option, index);
        const QRect rect(option.rect.width() - mMaxDescriptionWidth, option.rect.y(), mMaxDescriptionWidth, option.rect.height());
        painter->save();
        QFont font(painter->font());
        font.setStyle(QFont::StyleItalic);
        painter->setFont(font);
        if (option.state & QStyle::State_Selected) {
            painter->setPen(option.palette.color(QPalette::Normal, QPalette::BrightText));
        } else {
            painter->setPen(option.palette.color(QPalette::Disabled, QPalette::Text));
        }
        painter->drawText(rect, Qt::AlignLeft, mDescriptions.at(index.row()));
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setWidth(size.width() + mMaxDescriptionWidth);

        return size;
    }

private:
    QStringList mDescriptions;
    int mMaxDescriptionWidth;
};

DisplayNameEditWidget::DisplayNameEditWidget(QWidget *parent)
    : QWidget(parent)
    , mDisplayType(FullName)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(KDialog::spacingHint());

    mView = new KComboBox(this);
    mView->addItems(QStringList() << QString() << QString() << QString()
                                  << QString() << QString() << QString());

    layout->addWidget(mView);
    setFocusProxy(mView);
    setFocusPolicy(Qt::StrongFocus);
    connect(mView, SIGNAL(activated(int)), SLOT(displayTypeChanged(int)));

    DisplayNameDelegate *delegate = new DisplayNameDelegate(mView->view());
    mView->view()->setItemDelegate(delegate);

    mAdditionalPopupWidth = delegate->maximumDescriptionWidth();

    mViewport = mView->view()->viewport();
    mViewport->installEventFilter(this);
}

DisplayNameEditWidget::~DisplayNameEditWidget()
{
}

void DisplayNameEditWidget::setReadOnly(bool readOnly)
{
    mView->setEnabled(!readOnly);
}

void DisplayNameEditWidget::setDisplayType(DisplayType type)
{
    if (type == -1) {
        // guess the used display type
        mDisplayType = guessedDisplayType(mContact);
    } else {
        mDisplayType = type;
    }

    updateView();
}

DisplayNameEditWidget::DisplayType DisplayNameEditWidget::displayType() const
{
    return mDisplayType;
}

void DisplayNameEditWidget::loadContact(const KABC::Addressee &contact)
{
    mContact = contact;

    mDisplayType = guessedDisplayType(mContact);

    updateView();
}

void DisplayNameEditWidget::storeContact(KABC::Addressee &contact) const
{
    contact.setFormattedName(mView->currentText());
}

void DisplayNameEditWidget::changeName(const KABC::Addressee &contact)
{
    const QString organization = mContact.organization();
    mContact = contact;
    mContact.setOrganization(organization);
    if (mDisplayType == CustomName) {
        mContact.setFormattedName(mView->currentText());
    }

    updateView();
}

void DisplayNameEditWidget::changeOrganization(const QString &organization)
{
    mContact.setOrganization(organization);

    updateView();
}

void DisplayNameEditWidget::displayTypeChanged(int type)
{
    mDisplayType = (DisplayType)type;

    updateView();
}

bool DisplayNameEditWidget::eventFilter(QObject *object, QEvent *event)
{
    if (object == mViewport) {
        if (event->type() == QEvent::Show) {
            // retrieve the widget that contains the popup view
            QWidget *parentWidget = mViewport->parentWidget()->parentWidget();

            int maxWidth = 0;
            QFontMetrics metrics(mView->font());
            for (int i = 0; i < mView->count(); ++i) {
                maxWidth = qMax(maxWidth, metrics.width(mView->itemText(i)));
            }

            // resize it to show the complete content
            parentWidget->resize(maxWidth + mAdditionalPopupWidth + 20, parentWidget->height());
        }
        return false;
    }

    return QWidget::eventFilter(object, event);
}

void DisplayNameEditWidget::updateView()
{
    // SimpleName:
    mView->setItemText(0, mContact.givenName() + QLatin1Char(' ') + mContact.familyName());

    // FullName:
    mView->setItemText(1, mContact.assembledName());

    // ReverseNameWithComma:
    mView->setItemText(2, mContact.familyName() + QStringLiteral(", ") + mContact.givenName());

    // ReverseName:
    mView->setItemText(3, mContact.familyName() + QLatin1Char(' ') + mContact.givenName());

    // Organization:
    mView->setItemText(4, mContact.organization());

    // CustomName:
    mView->setItemText(5, mContact.formattedName());

    // delay the state change here, since we might have been called from mView via a signal
    QMetaObject::invokeMethod(this, "setComboBoxEditable", Qt::QueuedConnection, Q_ARG(bool, mDisplayType == CustomName));

    mView->setCurrentIndex((int)mDisplayType);
}

void DisplayNameEditWidget::setComboBoxEditable(bool value)
{
    mView->setEditable(value);
}
