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
#include <QTemporaryFile>
#include <QDir>
#include <QDesktopServices>
#include <QWindow>
#include <QScreen>

#include <kcolorscheme.h>
#include <KLocalizedString>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <KWindowConfig>

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

    QString plainText() const
    {
        return mTextContent;
    }

    void setPropertyNameTitle(const QString &title) override {
        mNameTitle = title;
    }

    void setLeftPropertyValueTitle(const QString &title) override {
        mLeftTitle = title;
    }

    void setRightPropertyValueTitle(const QString &title) override {
        mRightTitle = title;
    }

    void addProperty(Mode mode, const QString &name, const QString &leftValue, const QString &rightValue) override {
        switch (mode)
        {
        case NormalMode:
            mContent.append(QStringLiteral("<tr><td align=\"right\"><b>%1:</b></td><td>%2</td><td></td><td>%3</td></tr>")
            .arg(name,
            textToHTML(leftValue),
            textToHTML(rightValue)));
            mTextContent.append(QStringLiteral("%1:\n%2\n%3\n\n").arg(name, leftValue, rightValue));
            break;
        case ConflictMode:
            mContent.append(QStringLiteral("<tr><td align=\"right\"><b>%1:</b></td><td bgcolor=\"#ff8686\">%2</td><td></td><td bgcolor=\"#ff8686\">%3</td></tr>")
            .arg(name,
            textToHTML(leftValue),
            textToHTML(rightValue)));
            mTextContent.append(QStringLiteral("%1:\n%2\n%3\n\n").arg(name, leftValue, rightValue));
            break;
        case AdditionalLeftMode:
            mContent.append(QStringLiteral("<tr><td align=\"right\"><b>%1:</b></td><td bgcolor=\"#9cff83\">%2</td><td></td><td></td></tr>")
            .arg(name,
            textToHTML(leftValue)));
            mTextContent.append(QStringLiteral("%1:\n%2\n\n").arg(name, leftValue));
            break;
        case AdditionalRightMode:
            mContent.append(QStringLiteral("<tr><td align=\"right\"><b>%1:</b></td><td></td><td></td><td bgcolor=\"#9cff83\">%2</td></tr>")
            .arg(name,
            textToHTML(rightValue)));
            mTextContent.append(QStringLiteral("%1:\n%2\n\n").arg(name, rightValue));
            break;
        }
    }

private:
    QString header() const
    {
        QString header = QStringLiteral("<html>");
        header += QStringLiteral("<body text=\"%1\" bgcolor=\"%2\">")
                  .arg(KColorScheme(QPalette::Active, KColorScheme::View).foreground().color().name(),
                       KColorScheme(QPalette::Active, KColorScheme::View).background().color().name());
        header += QLatin1String("<center><table>");
        header += QStringLiteral("<tr><th align=\"center\">%1</th><th align=\"center\">%2</th><td>&nbsp;</td><th align=\"center\">%3</th></tr>")
                  .arg(mNameTitle, mLeftTitle, mRightTitle);

        return header;
    }

    QString footer() const
    {
        return QStringLiteral("</table></center>"
                              "</body>"
                              "</html>");
    }

    QString mContent;
    QString mNameTitle;
    QString mLeftTitle;
    QString mRightTitle;
    QString mTextContent;
};

static void compareItems(AbstractDifferencesReporter *reporter, const Akonadi::Item &localItem, const Akonadi::Item &otherItem)
{
    if (localItem.modificationTime() != otherItem.modificationTime()) {
        reporter->addProperty(AbstractDifferencesReporter::ConflictMode, i18n("Modification Time"),
                              QLocale().toString(localItem.modificationTime(), QLocale::ShortFormat),
                              QLocale().toString(otherItem.modificationTime(), QLocale::ShortFormat));
    }

    if (localItem.flags() != otherItem.flags()) {
        QStringList localFlags;
        localFlags.reserve(localItem.flags().count());
        foreach (const QByteArray &localFlag, localItem.flags()) {
            localFlags.append(QString::fromUtf8(localFlag));
        }

        QStringList otherFlags;
        otherFlags.reserve(otherItem.flags().count());
        foreach (const QByteArray &otherFlag, otherItem.flags()) {
            otherFlags.append(QString::fromUtf8(otherFlag));
        }

        reporter->addProperty(AbstractDifferencesReporter::ConflictMode, i18n("Flags"),
                              localFlags.join(QStringLiteral(", ")),
                              otherFlags.join(QStringLiteral(", ")));
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

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // Don't use QDialogButtonBox, order is very important (left on the left, right on the right)
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *takeLeftButton = new QPushButton(this);
    takeLeftButton->setText(i18nc("@action:button", "Take my version"));
    connect(takeLeftButton, &QPushButton::clicked, this, &ConflictResolveDialog::slotUseLocalItemChoosen);
    buttonLayout->addWidget(takeLeftButton);
    takeLeftButton->setObjectName(QStringLiteral("takeLeftButton"));

    QPushButton *takeRightButton = new QPushButton(this);
    takeRightButton->setText(i18nc("@action:button", "Take their version"));
    takeRightButton->setObjectName(QStringLiteral("takeRightButton"));
    connect(takeRightButton, &QPushButton::clicked, this, &ConflictResolveDialog::slotUseOtherItemChoosen);
    buttonLayout->addWidget(takeRightButton);

    QPushButton *keepBothButton = new QPushButton(this);
    keepBothButton->setText(i18nc("@action:button", "Keep both versions"));
    keepBothButton->setObjectName(QStringLiteral("keepBothButton"));
    buttonLayout->addWidget(keepBothButton);
    connect(keepBothButton, &QPushButton::clicked, this, &ConflictResolveDialog::slotUseBothItemsChoosen);

    keepBothButton->setDefault(true);


    mView = new QTextBrowser(this);
    mView->setObjectName(QStringLiteral("view"));
    mView->setOpenLinks(false);

    QLabel *docuLabel = new QLabel(i18n("<qt>Your changes conflict with those made by someone else meanwhile.<br>"
                                        "Unless one version can just be thrown away, you will have to integrate those changes manually.<br>"
                                        "Click on <a href=\"opentexteditor\">\"Open text editor\"</a> to keep a copy of the texts, then select which version is most correct, then re-open it and modify it again to add what's missing."));
    connect(docuLabel, &QLabel::linkActivated, this, &ConflictResolveDialog::slotOpenEditor);
    docuLabel->setContextMenuPolicy(Qt::NoContextMenu);

    docuLabel->setWordWrap(true);
    docuLabel->setObjectName(QStringLiteral("doculabel"));

    mainLayout->addWidget(mView);
    mainLayout->addWidget(docuLabel);
    mainLayout->addLayout(buttonLayout);

    // default size is tiny, and there's usually lots of text, so make it much bigger
    create(); // ensure a window is created
    const QSize availableSize = windowHandle()->screen()->availableSize();
    windowHandle()->resize(availableSize.width() * 0.7, availableSize.height() * 0.5);
    KWindowConfig::restoreWindowSize(windowHandle(), KSharedConfig::openConfig()->group("ConflictResolveDialog"));
    resize(windowHandle()->size()); // workaround for QTBUG-40584
}

ConflictResolveDialog::~ConflictResolveDialog()
{
    KConfigGroup group(KSharedConfig::openConfig()->group("ConflictResolveDialog"));
    KWindowConfig::saveWindowSize(windowHandle(), group);
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
                mTextContent = reporter.plainText();
                return;
            }
        }

        reporter.addProperty(HtmlDifferencesReporter::NormalMode, i18n("Data"),
                             QString::fromUtf8(mLocalItem.payloadData()),
                             QString::fromUtf8(mOtherItem.payloadData()));
    }

    mView->setHtml(reporter.toHtml());
    mTextContent = reporter.plainText();
}

void ConflictResolveDialog::slotOpenEditor()
{
    QTemporaryFile file(QDir::tempPath() + QStringLiteral("/akonadi-XXXXXX.txt"));
    if (file.open()) {
        file.setAutoRemove(false);
        file.write(mTextContent.toLocal8Bit());
        const QString fileName = file.fileName();
        file.close();
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
    }
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
