/*
    Copyright (c) 2010 KDAB
    Author: Tobias Koenig <tokoe@kde.org>

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

#include "conflictresolvedialog_p.h"

#include "abstractdifferencesreporter.h"
#include "differencesalgorithminterface.h"
#include "typepluginloader_p.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>

#include <kcolorscheme.h>
#include <klocalizedstring.h>
#include <kglobal.h>
#include <qtextbrowser.h>
#include <KLocale>
#include <QDialogButtonBox>

using namespace Akonadi;

static inline QString textToHTML(const QString &text)
{
    return Qt::convertFromPlainText(text);
}

class HtmlDifferencesReporter : public AbstractDifferencesReporter
{
public:
    HtmlDifferencesReporter()
    {
    }

    QString toHtml() const
    {
        return header() + mContent + footer();
    }

    void setPropertyNameTitle(const QString &title) Q_DECL_OVERRIDE
    {
        mNameTitle = title;
    }

    void setLeftPropertyValueTitle(const QString &title) Q_DECL_OVERRIDE
    {
        mLeftTitle = title;
    }

    void setRightPropertyValueTitle(const QString &title) Q_DECL_OVERRIDE
    {
        mRightTitle = title;
    }

    void addProperty(Mode mode, const QString &name, const QString &leftValue, const QString &rightValue) Q_DECL_OVERRIDE
    {
        switch (mode) {
        case NormalMode:
            mContent.append(QString::fromLatin1("<tr><td align=\"right\"><b>%1:</b></td><td>%2</td><td></td><td>%3</td></tr>")
                            .arg(name,
                                 textToHTML(leftValue),
                                 textToHTML(rightValue)));
            break;
        case ConflictMode:
            mContent.append(QString::fromLatin1("<tr><td align=\"right\"><b>%1:</b></td><td bgcolor=\"#ff8686\">%2</td><td></td><td bgcolor=\"#ff8686\">%3</td></tr>")
                            .arg(name,
                                 textToHTML(leftValue),
                                 textToHTML(rightValue)));
            break;
        case AdditionalLeftMode:
            mContent.append(QString::fromLatin1("<tr><td align=\"right\"><b>%1:</b></td><td bgcolor=\"#9cff83\">%2</td><td></td><td></td></tr>")
                            .arg(name,
                                 textToHTML(leftValue)));
            break;
        case AdditionalRightMode:
            mContent.append(QString::fromLatin1("<tr><td align=\"right\"><b>%1:</b></td><td></td><td></td><td bgcolor=\"#9cff83\">%2</td></tr>")
                            .arg(name,
                                 textToHTML(rightValue)));
            break;
        }
    }

private:
    QString header() const
    {
        QString header = QLatin1String("<html>");
        header += QString::fromLatin1("<body text=\"%1\" bgcolor=\"%2\">")
                  .arg(KColorScheme(QPalette::Active, KColorScheme::View).foreground().color().name())
                  .arg(KColorScheme(QPalette::Active, KColorScheme::View).background().color().name());
        header += QLatin1String("<center><table>");
        header += QString::fromLatin1("<tr><th align=\"center\">%1</th><th align=\"center\">%2</th><td>&nbsp;</td><th align=\"center\">%3</th></tr>")
                  .arg(mNameTitle)
                  .arg(mLeftTitle)
                  .arg(mRightTitle);

        return header;
    }

    QString footer() const
    {
        return QLatin1String("</table></center>"
                             "</body>"
                             "</html>");
    }

    QString mContent;
    QString mNameTitle;
    QString mLeftTitle;
    QString mRightTitle;
};

static void compareItems(AbstractDifferencesReporter *reporter, const Akonadi::Item &localItem, const Akonadi::Item &otherItem)
{
    if (localItem.modificationTime() != otherItem.modificationTime()) {
        reporter->addProperty(AbstractDifferencesReporter::ConflictMode, i18n("Modification Time"),
                              KLocale::global()->formatDateTime(localItem.modificationTime(), KLocale::ShortDate, true),
                              KLocale::global()->formatDateTime(otherItem.modificationTime(), KLocale::ShortDate, true));
    }

    if (localItem.flags() != otherItem.flags()) {
        QStringList localFlags;
        foreach (const QByteArray &localFlag, localItem.flags()) {
            localFlags.append(QString::fromUtf8(localFlag));
        }

        QStringList otherFlags;
        foreach (const QByteArray &otherFlag, otherItem.flags()) {
            otherFlags.append(QString::fromUtf8(otherFlag));
        }

        reporter->addProperty(AbstractDifferencesReporter::ConflictMode, i18n("Flags"),
                              localFlags.join(QLatin1String(", ")),
                              otherFlags.join(QLatin1String(", ")));
    }

    QHash<QByteArray, QByteArray> localAttributes;
    foreach (Akonadi::Attribute *attribute, localItem.attributes()) {
        localAttributes.insert(attribute->type(), attribute->serialized());
    }

    QHash<QByteArray, QByteArray> otherAttributes;
    foreach (Akonadi::Attribute *attribute, otherItem.attributes()) {
        otherAttributes.insert(attribute->type(), attribute->serialized());
    }

    if (localAttributes != otherAttributes) {
        foreach (const QByteArray &localKey, localAttributes) {
            if (!otherAttributes.contains(localKey)) {
                reporter->addProperty(AbstractDifferencesReporter::AdditionalLeftMode, i18n("Attribute: %1", QString::fromUtf8(localKey)),
                                      QString::fromUtf8(localAttributes.value(localKey)),
                                      QString());
            } else {
                const QByteArray localValue = localAttributes.value(localKey);
                const QByteArray otherValue = otherAttributes.value(localKey);
                if (localValue != otherValue) {
                    reporter->addProperty(AbstractDifferencesReporter::ConflictMode, i18n("Attribute: %1", QString::fromUtf8(localKey)),
                                          QString::fromUtf8(localValue),
                                          QString::fromUtf8(otherValue));
                }
            }
        }

        foreach (const QByteArray &otherKey, otherAttributes) {
            if (!localAttributes.contains(otherKey)) {
                reporter->addProperty(AbstractDifferencesReporter::AdditionalRightMode, i18n("Attribute: %1", QString::fromUtf8(otherKey)),
                                      QString(),
                                      QString::fromUtf8(otherAttributes.value(otherKey)));
            }
        }
    }
}

ConflictResolveDialog::ConflictResolveDialog(QWidget *parent)
    : QDialog(parent), mResolveStrategy(ConflictHandler::UseBothItems)
{
    setWindowTitle(i18nc("@title:window", "Conflict Resolution"));
    //PORTING SCRIPT: Move QDialogButtonBox at the end of init of widget to add it in layout.
    QDialogButtonBox *buttonBox = new QDialogButtonBox();
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *user1Button = new QPushButton;
    buttonBox->addButton(user1Button, QDialogButtonBox::ActionRole);
    QPushButton *user2Button = new QPushButton;
    buttonBox->addButton(user2Button, QDialogButtonBox::ActionRole);
    QPushButton *user3Button = new QPushButton;
    buttonBox->addButton(user3Button, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConflictResolveDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConflictResolveDialog::reject);
    user3Button->setDefault(true);

    user3Button->setText(i18n("Take left one"));
    user2Button->setText(i18n("Take right one"));
    user1Button->setText(i18n("Keep both"));

    connect(user1Button, &QPushButton::clicked, this, &ConflictResolveDialog::slotUseBothItemsChoosen);
    connect(user2Button, &QPushButton::clicked, this, &ConflictResolveDialog::slotUseOtherItemChoosen);
    connect(user3Button, &QPushButton::clicked, this, &ConflictResolveDialog::slotUseLocalItemChoosen);

    QLabel *label = new QLabel(xi18nc("@label", "Two updates conflict with each other.<nl/>Please choose which update(s) to apply."), mainWidget);
    mainLayout->addWidget(label);

    mView = new QTextBrowser;

    mainLayout->addWidget(mView);
    mainLayout->addWidget(buttonBox);

}

void ConflictResolveDialog::setConflictingItems(const Akonadi::Item &localItem, const Akonadi::Item &otherItem)
{
    mLocalItem = localItem;
    mOtherItem = otherItem;

    HtmlDifferencesReporter reporter;
    compareItems(&reporter, localItem, otherItem);

    if (mLocalItem.hasPayload() && mOtherItem.hasPayload()) {

        QObject *object = TypePluginLoader::objectForMimeTypeAndClass(localItem.mimeType(), localItem.availablePayloadMetaTypeIds());
        if (object) {
            DifferencesAlgorithmInterface *algorithm = qobject_cast<DifferencesAlgorithmInterface *>(object);
            if (algorithm) {
                algorithm->compare(&reporter, localItem, otherItem);
                mView->setHtml(reporter.toHtml());
                return;
            }
        }

        reporter.addProperty(HtmlDifferencesReporter::NormalMode, i18n("Data"),
                             QString::fromUtf8(mLocalItem.payloadData()),
                             QString::fromUtf8(mOtherItem.payloadData()));
    }

    mView->setHtml(reporter.toHtml());
}

ConflictHandler::ResolveStrategy ConflictResolveDialog::resolveStrategy() const
{
    return mResolveStrategy;
}

void ConflictResolveDialog::slotUseLocalItemChoosen()
{
    mResolveStrategy = ConflictHandler::UseLocalItem;
    accept();
}

void ConflictResolveDialog::slotUseOtherItemChoosen()
{
    mResolveStrategy = ConflictHandler::UseOtherItem;
    accept();
}

void ConflictResolveDialog::slotUseBothItemsChoosen()
{
    mResolveStrategy = ConflictHandler::UseBothItems;
    accept();
}

#include "moc_conflictresolvedialog_p.cpp"
