/*
    Copyright (c) 2020  Daniel Vrátil <dvratil@kde.org>

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

#include "task.h"

using namespace Akonadi;

Q_LOGGING_CATEGORY(AKONADICORE_ASYNC_LOG, "org.kde.pim.akonadicore.async", QtInfoMsg)

void TaskDataBase::addObserver(Observer *observer)
{
    m_observers.push_back(observer);
}

void TaskDataBase::removeObserver(Observer *observer)
{
    m_observers.erase(std::remove(m_observers.begin(), m_observers.end(), observer));
}

void TaskDataBase::notifyObservers()
{
    std::for_each(m_observers.cbegin(), m_observers.cend(), std::mem_fn(&Observer::notify));

    if (m_cont) {
        m_cont();
    }
}

