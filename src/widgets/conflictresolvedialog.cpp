/*
    SPDX-FileCopyrightText: 2010 KDAB
    SPDX-FileContributor: Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "conflictresolvedialog_p.h"

#include "abstractdifferencesreporter.h"
#include "differencesalgorithminterface.h"
#include "typepluginloader_p.h"

#include <shared/akranges.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTemporaryFile>
#include <QDir>
#include <QDesktopServices>
#include <QWindow>
#include <QScreen>

#include <KColorScheme>
#include <KLocalizedString>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <KWindowConfig>

using namespace Akonadi;
using namespace AkRanges;

static inline QString textToHTML(const QString &text)
{
    return Qt::convertFromPlainText(text);
}

class HtmlDifferencesReporter : public AbstractDifferencesReporter
{
public:
    HtmlDifferencesReporter() = default;

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
        const auto toQString = [](const QByteArray &s) { return QString::fromUtf8(s); };
        const auto localFlags = localItem.flags() | Views::transform(toQString) | Actions::toQList;
        const auto otherFlags = otherItem.flags() | Views::transform(toQString) | Actions::toQList;
        reporter->addProperty(AbstractDifferencesReporter::ConflictMode, i18n("Flags"),
                              localFlags.join(QLatin1String(", ")),
                              otherFlags.join(QLatin1String(", ")));
    }

    const auto toPair = [](Attribute *attr) { return std::pair{attr->type(), attr->serialized()}; };
    const auto localAttributes = localItem.attributes() | Views::transform(toPair) | Actions::toQHash;
    const auto otherAttributes = otherItem.attributes() | Views::transform(toPair) | Actions::toQHash;

    if (localAttributes != otherAttributes) {
        for (const QByteArray &localKey : localAttributes) {
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

        for (const QByteArray &otherKey : otherAttributes) {
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

    auto *mainLayout = new QVBoxLayout(this);
    // Don't use QDialogButtonBox, order is very important (left on the left, right on the right)
    auto *buttonLayout = new QHBoxLayout();
    auto *takeLeftButton = new QPushButton(this);
    takeLeftButton->setText(i18nc("@action:button", "Take my version"));
    connect(takeLeftButton, &QPushButton::clicked, this, &ConflictResolveDialog::slotUseLocalItemChoosen);
    buttonLayout->addWidget(takeLeftButton);
    takeLeftButton->setObjectName(QStringLiteral("takeLeftButton"));

    auto *takeRightButton = new QPushButton(this);
    takeRightButton->setText(i18nc("@action:button", "Take their version"));
    takeRightButton->setObjectName(QStringLiteral("takeRightButton"));
    connect(takeRightButton, &QPushButton::clicked, this, &ConflictResolveDialog::slotUseOtherItemChoosen);
    buttonLayout->addWidget(takeRightButton);

    auto *keepBothButton = new QPushButton(this);
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
    windowHandle()->resize(static_cast<int>(availableSize.width() * 0.7),
                           static_cast<int>(availableSize.height() * 0.5));
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
