/*
    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QThread>

#include <atomic>
#include <type_traits>

namespace Akonadi
{
namespace Server
{

class AkThread : public QObject
{
    Q_OBJECT
public:
    enum StartMode {
        AutoStart,
        ManualStart,
        NoThread // for unit-tests
    };

    template<typename T, typename... Args>
    static std::enable_if_t<std::is_base_of_v<AkThread, T>, std::unique_ptr<T>> create(Args &&...args) noexcept
    {
        // Workaround T having a protected constructor
        struct TConstructor : T {
            explicit TConstructor(Args &&...args)
                : T(std::forward<Args>(args)...)
            {
            }
        };
        std::unique_ptr<T> thread = std::make_unique<TConstructor>(std::forward<Args>(args)...);
        if (thread->m_startMode == AkThread::AutoStart) {
            thread->startThread();
        }
        return thread;
    }

    ~AkThread() override;

    bool isInitialized() const
    {
        return m_initialized;
    }

    void waitForInitialized();

Q_SIGNALS:
    void initialized();

protected:
    explicit AkThread(const QString &objectName, QThread::Priority priority = QThread::InheritPriority, QObject *parent = nullptr);
    explicit AkThread(const QString &objectName, StartMode startMode, QThread::Priority priority = QThread::InheritPriority, QObject *parent = nullptr);

    void quitThread();
    void startThread();

protected Q_SLOTS:
    virtual void init();
    virtual void quit();

private:
    StartMode m_startMode = AutoStart;
    bool m_quitCalled = false;
    std::atomic_bool m_initialized = false;
};

} // namespace Server
} // namespace Akonadi
