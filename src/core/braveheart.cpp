/*
    Copyright (c) 2016 Daniel Vrátil <dvratil@kde.org>

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

#include <config-akonadi.h>

#ifdef HAVE_MALLOC_TRIM

#include <QCoreApplication>
#include <QTimer>
#include <QVariant>
#include <QThread>

#include <malloc.h>

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
        QObject::connect(freedom, &QTimer::timeout,
        freedom, []() {
            // They may take our lives, but they will never
            // take our memory!
            malloc_trim(50 * 1024 * 1024);
        });
        // Fight for freedom every 15 minutes
        freedom->start(15 * 60 * 1000);
        qApp->setProperty("__Akonadi__Braveheart", true);
    };

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

Braveheart Wallace;

}

} // namespace Akonadi

#endif // HAVE_MALLOC_TRIM
