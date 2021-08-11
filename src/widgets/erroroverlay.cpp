/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "erroroverlay_p.h"
#include "ui_erroroverlay.h"
#if 0
#include "selftestdialog_p.h"
#endif

#include <KLocalizedString>
#include <KStandardGuiItem>
#include <QIcon>

#include <QEvent>
#include <QPalette>

using namespace Akonadi;

/// @cond PRIVATE

class ErrorOverlayStatic
{
public:
    QVector<QPair<QPointer<QWidget>, QPointer<QWidget>>> baseWidgets;
};

Q_GLOBAL_STATIC(ErrorOverlayStatic, sInstanceOverlay) // NOLINT(readability-redundant-member-init)

// return true if o1 is a parent of o2
static bool isParentOf(QWidget *o1, QWidget *o2)
{
    if (!o1 || !o2) {
        return false;
    }
    if (o1 == o2) {
        return true;
    }
    if (o2->isWindow()) {
        return false;
    }
    return isParentOf(o1, o2->parentWidget());
}

ErrorOverlay::ErrorOverlay(QWidget *baseWidget, QWidget *parent)
    : QWidget(parent ? parent : baseWidget->window())
    , mBaseWidget(baseWidget)
    , ui(new Ui::ErrorOverlay)
{
    Q_ASSERT(baseWidget);

    mBaseWidgetIsParent = isParentOf(mBaseWidget, this);

    // check existing overlays to detect cascading
    for (QVector<QPair<QPointer<QWidget>, QPointer<QWidget>>>::Iterator it = sInstanceOverlay->baseWidgets.begin();
         it != sInstanceOverlay->baseWidgets.end();) {
        if ((*it).first == nullptr || (*it).second == nullptr) {
            // garbage collection
            it = sInstanceOverlay->baseWidgets.erase(it);
            continue;
        }
        if (isParentOf((*it).first, baseWidget)) {
            // parent already has an overlay, kill ourselves
            mBaseWidget = nullptr;
            hide();
            deleteLater();
            return;
        }
        if (isParentOf(baseWidget, (*it).first)) {
            // child already has overlay, kill that one
            delete (*it).second;
            it = sInstanceOverlay->baseWidgets.erase(it);
            continue;
        }
        ++it;
    }
    sInstanceOverlay->baseWidgets.append(qMakePair(mBaseWidget, QPointer<QWidget>(this)));

    connect(baseWidget, &QObject::destroyed, this, &QObject::deleteLater);
    mPreviousState = !mBaseWidget->testAttribute(Qt::WA_ForceDisabled);

    ui->setupUi(this);
    ui->notRunningIcon->setPixmap(QIcon::fromTheme(QStringLiteral("akonadi")).pixmap(64));
    ui->brokenIcon->setPixmap(QIcon::fromTheme(QStringLiteral("dialog-error")).pixmap(64));
    ui->progressIcon->setPixmap(QIcon::fromTheme(QStringLiteral("akonadi")).pixmap(32));
    ui->quitButton->setText(KStandardGuiItem::quit().text());
    ui->detailsQuitButton->setText(KStandardGuiItem::quit().text());

    ui->quitButton->hide();
    ui->detailsQuitButton->hide();

    connect(ui->startButton, &QAbstractButton::clicked, this, &ErrorOverlay::startClicked);
    connect(ui->quitButton, &QAbstractButton::clicked, this, &ErrorOverlay::quitClicked);
    connect(ui->detailsQuitButton, &QAbstractButton::clicked, this, &ErrorOverlay::quitClicked);
    connect(ui->selfTestButton, &QAbstractButton::clicked, this, &ErrorOverlay::selfTestClicked);

    const ServerManager::State state = ServerManager::state();
    mOverlayActive = (state == ServerManager::Running);
    serverStateChanged(state);

    connect(ServerManager::self(), &ServerManager::stateChanged, this, &ErrorOverlay::serverStateChanged);

    QPalette p = palette();
    p.setColor(backgroundRole(), QColor(0, 0, 0, 128));
    p.setColor(foregroundRole(), Qt::white);
    setPalette(p);
    setAutoFillBackground(true);

    mBaseWidget->installEventFilter(this);

    reposition();
}

ErrorOverlay::~ErrorOverlay()
{
    if (mBaseWidget && !mBaseWidgetIsParent) {
        mBaseWidget->setEnabled(mPreviousState);
    }
}

void ErrorOverlay::reposition()
{
    if (!mBaseWidget) {
        return;
    }

    // reparent to the current top level widget of the base widget if needed
    // needed eg. in dock widgets
    if (parentWidget() != mBaseWidget->window()) {
        setParent(mBaseWidget->window());
    }

    // follow base widget visibility
    // needed eg. in tab widgets
    if (!mBaseWidget->isVisible()) {
        hide();
        return;
    }
    if (mOverlayActive) {
        show();
    }

    // follow position changes
    const QPoint topLevelPos = mBaseWidget->mapTo(window(), QPoint(0, 0));
    const QPoint parentPos = parentWidget()->mapFrom(window(), topLevelPos);
    move(parentPos);

    // follow size changes
    // TODO: hide/scale icon if we don't have enough space
    resize(mBaseWidget->size());
}

bool ErrorOverlay::eventFilter(QObject *object, QEvent *event)
{
    if (object == mBaseWidget && mOverlayActive
        && (event->type() == QEvent::Move || event->type() == QEvent::Resize || event->type() == QEvent::Show || event->type() == QEvent::Hide
            || event->type() == QEvent::ParentChange)) {
        reposition();
    }
    return QWidget::eventFilter(object, event);
}

void ErrorOverlay::startClicked()
{
    const ServerManager::State state = ServerManager::state();
    if (state == ServerManager::Running) {
        serverStateChanged(state);
    } else {
        ServerManager::start();
    }
}

void ErrorOverlay::quitClicked()
{
    qApp->quit();
}

void ErrorOverlay::selfTestClicked()
{
#if 0
    SelfTestDialog dlg;
    dlg.exec();
#endif
}

void ErrorOverlay::serverStateChanged(ServerManager::State state)
{
    if (!mBaseWidget) {
        return;
    }

    if (state == ServerManager::Running) {
        if (mOverlayActive) {
            mOverlayActive = false;
            hide();
            if (!mBaseWidgetIsParent) {
                mBaseWidget->setEnabled(mPreviousState);
            }
        }
    } else if (!mOverlayActive) {
        mOverlayActive = true;
        if (mBaseWidget->isVisible()) {
            show();
        }

        if (!mBaseWidgetIsParent) {
            mPreviousState = !mBaseWidget->testAttribute(Qt::WA_ForceDisabled);
            mBaseWidget->setEnabled(false);
        }

        reposition();
    }

    if (mOverlayActive) {
        switch (state) {
        case ServerManager::NotRunning:
            ui->stackWidget->setCurrentWidget(ui->notRunningPage);
            break;
        case ServerManager::Broken:
            ui->stackWidget->setCurrentWidget(ui->brokenPage);
            if (!ServerManager::brokenReason().isEmpty()) {
                ui->brokenDescription->setText(
                    i18nc("%1 is a reason why", "Cannot connect to the Personal information management service.\n\n%1", ServerManager::brokenReason()));
            }
            break;
        case ServerManager::Starting:
            ui->progressPage->setToolTip(i18n("Personal information management service is starting..."));
            ui->progressDescription->setText(i18n("Personal information management service is starting..."));
            ui->stackWidget->setCurrentWidget(ui->progressPage);
            break;
        case ServerManager::Stopping:
            ui->progressPage->setToolTip(i18n("Personal information management service is shutting down..."));
            ui->progressDescription->setText(i18n("Personal information management service is shutting down..."));
            ui->stackWidget->setCurrentWidget(ui->progressPage);
            break;
        case ServerManager::Upgrading:
            ui->progressPage->setToolTip(i18n("Personal information management service is performing a database upgrade."));
            ui->progressDescription->setText(
                i18n("Personal information management service is performing a database upgrade.\n"
                     "This happens after a software update and is necessary to optimize performance.\n"
                     "Depending on the amount of personal information, this might take a few minutes."));
            ui->stackWidget->setCurrentWidget(ui->progressPage);
            break;
        case ServerManager::Running:
            break;
        }
    }
}

/// @endcond

#include "moc_erroroverlay_p.cpp"
