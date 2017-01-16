/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "controlgui.h"
#include "servermanager.h"
#include "ui_controlprogressindicator.h"
#include "selftestdialog.h"
#include "erroroverlay_p.h"
#include "akonadiwidgets_debug.h"
#include "helper_p.h"

#include <KLocalizedString>

#include <QEventLoop>
#include <QCoreApplication>
#include <QTimer>
#include <QPointer>
#include <QFrame>

using namespace Akonadi;

namespace Akonadi
{
namespace Internal
{

class ControlProgressIndicator : public QFrame
{
public:
    ControlProgressIndicator(QWidget *parent = nullptr)
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
public:
    StaticControlGui()
        : ControlGui()
    {
    }
};

}

Q_GLOBAL_STATIC(Internal::StaticControlGui, s_instance)

/**
 * @internal
 */
class Q_DECL_HIDDEN ControlGui::Private
{
public:
    Private(ControlGui *parent)
        : mParent(parent)
        , mEventLoop(nullptr)
        , mProgressIndicator(nullptr)
        , mSuccess(false)
        , mStarting(false)
        , mStopping(false)
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
        for (QWidget *widget : qAsConst(mPendingOverlays)) {
            if (widget) {
                new ErrorOverlay(widget);
            }
        }
        mPendingOverlays.clear();
    }

    void cleanup()
    {
        //delete s_instance;
    }

    bool exec();
    void serverStateChanged(ServerManager::State state);

    QPointer<ControlGui> mParent;
    QEventLoop *mEventLoop;
    QPointer<Internal::ControlProgressIndicator> mProgressIndicator;
    QList<QPointer<QWidget> > mPendingOverlays;
    bool mSuccess;

    bool mStarting;
    bool mStopping;
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
    qCDebug(AKONADIWIDGETS_LOG) << state;
    if (mEventLoop && mEventLoop->isRunning()) {
        // ignore transient states going into the right direction
        if ((mStarting && (state == ServerManager::Starting || state == ServerManager::Upgrading)) ||
                (mStopping && state == ServerManager::Stopping)) {
            return;
        }
        mEventLoop->quit();
        mSuccess = (mStarting && state == ServerManager::Running) || (mStopping && state == ServerManager::NotRunning);
    }
}

ControlGui::ControlGui()
    : d(new Private(this))
{
    connect(ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)),
            SLOT(serverStateChanged(Akonadi::ServerManager::State)));
    // mProgressIndicator is a widget, so it better be deleted before the QApplication is deleted
    // Otherwise we get a crash in QCursor code with Qt-4.5
    if (QCoreApplication::instance()) {
        connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(cleanup()));
    }
}

ControlGui::~ControlGui()
{
    delete d;
}

bool ControlGui::start()
{
    if (ServerManager::state() == ServerManager::Stopping) {
        qCDebug(AKONADIWIDGETS_LOG) << "Server is currently being stopped, wont try to start it now";
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
    QTimer::singleShot(0, s_instance, SLOT(createErrorOverlays()));
}

}

#include "moc_controlgui.cpp"
