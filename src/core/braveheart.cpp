/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <config-akonadi.h>

#if HAVE_MALLOC_TRIM

#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <QVariant>

#include <chrono>
#include <malloc.h>

using namespace std::chrono_literals;

namespace Akonadi
{
class Braveheart
{
private:
    static void sonOfScotland()
    {
        Q_ASSERT(qApp->thread() == QThread::currentThread());

        if (!qApp->property("__Akonadi__Braveheart").isNull()) {
            // One Scottish warrior is enough....
            return;
        }
        auto freedom = new QTimer(qApp);
        QObject::connect(freedom, &QTimer::timeout, freedom, []() {
            // They may take our lives, but they will never
            // take our memory!
            malloc_trim(50 * 1024 * 1024);
        });
        // Fight for freedom every 15 minutes
        freedom->start(15min);
        qApp->setProperty("__Akonadi__Braveheart", true);
    }

public:
    explicit Braveheart()
    {
        qAddPreRoutine([]() {
            if (qApp->thread() != QThread::currentThread()) {
                QTimer::singleShot(0, qApp, sonOfScotland);
            } else {
                sonOfScotland();
            }
        });
    }
};

namespace
{
Braveheart Wallace; // clazy:exclude=non-pod-global-static

}

} // namespace Akonadi

#endif // HAVE_MALLOC_TRIM
