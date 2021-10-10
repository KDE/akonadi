/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "controlgui.h"
#include "akonadiwidgets_debug.h"
#include "erroroverlay_p.h"
#include "selftestdialog.h"
#include "servermanager.h"
#include "ui_controlprogressindicator.h"

#include <KLocalizedString>

#include <QCoreApplication>
#include <QEventLoop>
#include <QFrame>
#include <QPointer>
#include <QTimer>

using namespace Akonadi;
using namespace std::chrono_literals;
namespace Akonadi
{
namespace Internal
{
class ControlProgressIndicator : public QFrame
{
    Q_OBJECT
public:
    explicit ControlProgressIndicator(QWidget *parent = nullptr)
        : QFrame(parent)
    {
        setWindowModality(Qt::ApplicationModal);
        resize(400, 100);
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        ui.setupUi(this);

        setFrameShadow(QFrame::Plain);
        setFrameShape(QFrame::Box);
    }

    void setMessage(const QString &msg)
    {
        ui.statusLabel->setText(msg);
    }

    Ui::ControlProgressIndicator ui;
};

class StaticControlGui : public ControlGui
{
    Q_OBJECT
};

} // namespace Internal

Q_GLOBAL_STATIC(Internal::StaticControlGui, s_instance) // NOLINT(readability-redundant-member-init)

/**
 * @internal
 */
class Q_DECL_HIDDEN ControlGui::Private
{
public:
    explicit Private(ControlGui *parent)
        : mParent(parent)
        , mProgressIndicator(nullptr)
    {
    }

    ~Private()
    {
        delete mProgressIndicator;
    }

    void setupProgressIndicator(const QString &msg, QWidget *parent = nullptr)
    {
        if (!mProgressIndicator) {
            mProgressIndicator = new Internal::ControlProgressIndicator(parent);
        }

        mProgressIndicator->setMessage(msg);
    }

    void createErrorOverlays()
    {
        for (QWidget *widget : std::as_const(mPendingOverlays)) {
            if (widget) {
                new ErrorOverlay(widget);
            }
        }
        mPendingOverlays.clear();
    }

    void cleanup()
    {
        // delete s_instance;
    }

    bool exec();
    void serverStateChanged(ServerManager::State state);

    QPointer<ControlGui> mParent;
    QEventLoop *mEventLoop = nullptr;
    QPointer<Internal::ControlProgressIndicator> mProgressIndicator;
    QList<QPointer<QWidget>> mPendingOverlays;
    bool mSuccess = false;

    bool mStarting = false;
    bool mStopping = false;
};

bool ControlGui::Private::exec()
{
    if (mProgressIndicator) {
        mProgressIndicator->show();
    }
    qCDebug(AKONADIWIDGETS_LOG) << "Starting/Stopping Akonadi (using an event loop).";
    mEventLoop = new QEventLoop(mParent);
    mEventLoop->exec();
    mEventLoop->deleteLater();
    mEventLoop = nullptr;

    if (!mSuccess) {
        qCWarning(AKONADIWIDGETS_LOG) << "Could not start/stop Akonadi!";
        if (mProgressIndicator && mStarting) {
            QPointer<SelfTestDialog> dlg = new SelfTestDialog(mProgressIndicator->parentWidget());
            dlg->exec();
            delete dlg;
            if (!mParent) {
                return false;
            }
        }
    }

    delete mProgressIndicator;
    mProgressIndicator = nullptr;
    mStarting = false;
    mStopping = false;

    const bool rv = mSuccess;
    mSuccess = false;
    return rv;
}

void ControlGui::Private::serverStateChanged(ServerManager::State state)
{
    qCDebug(AKONADIWIDGETS_LOG) << "Server state changed to" << state;
    if (mEventLoop && mEventLoop->isRunning()) {
        // ignore transient states going into the right direction
        if ((mStarting && (state == ServerManager::Starting || state == ServerManager::Upgrading)) || (mStopping && state == ServerManager::Stopping)) {
            return;
        }
        mEventLoop->quit();
        mSuccess = (mStarting && state == ServerManager::Running) || (mStopping && state == ServerManager::NotRunning);
    }
}

ControlGui::ControlGui()
    : d(new Private(this))
{
    connect(ServerManager::self(), &ServerManager::stateChanged, this, [this](Akonadi::ServerManager::State state) {
        d->serverStateChanged(state);
    });
    // mProgressIndicator is a widget, so it better be deleted before the QApplication is deleted
    // Otherwise we get a crash in QCursor code with Qt-4.5
    if (QCoreApplication::instance()) {
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
            d->cleanup();
        });
    }
}

ControlGui::~ControlGui() = default;

bool ControlGui::start()
{
    if (ServerManager::state() == ServerManager::Stopping) {
        qCDebug(AKONADIWIDGETS_LOG) << "Server is currently being stopped, won't try to start it now";
        return false;
    }
    if (ServerManager::isRunning() || s_instance->d->mEventLoop) {
        qCDebug(AKONADIWIDGETS_LOG) << "Server is already running";
        return true;
    }
    s_instance->d->mStarting = true;
    if (!ServerManager::start()) {
        qCDebug(AKONADIWIDGETS_LOG) << "ServerManager::start failed -> return false";
        return false;
    }
    return s_instance->d->exec();
}

bool ControlGui::stop()
{
    if (ServerManager::state() == ServerManager::Starting) {
        return false;
    }
    if (!ServerManager::isRunning() || s_instance->d->mEventLoop) {
        return true;
    }
    s_instance->d->mStopping = true;
    if (!ServerManager::stop()) {
        return false;
    }
    return s_instance->d->exec();
}

bool ControlGui::restart()
{
    if (ServerManager::isRunning()) {
        if (!stop()) {
            return false;
        }
    }
    return start();
}

bool ControlGui::start(QWidget *parent)
{
    s_instance->d->setupProgressIndicator(i18n("Starting Akonadi server..."), parent);
    return start();
}

bool ControlGui::stop(QWidget *parent)
{
    s_instance->d->setupProgressIndicator(i18n("Stopping Akonadi server..."), parent);
    return stop();
}

bool ControlGui::restart(QWidget *parent)
{
    if (ServerManager::isRunning()) {
        if (!stop(parent)) {
            return false;
        }
    }
    return start(parent);
}

void ControlGui::widgetNeedsAkonadi(QWidget *widget)
{
    s_instance->d->mPendingOverlays.append(widget);
    // delay the overlay creation since we rely on widget being reparented
    // correctly already
    QTimer::singleShot(0s, s_instance, []() {
        s_instance->d->createErrorOverlays();
    });
}

} // namespace Akonadi

#include "controlgui.moc"
