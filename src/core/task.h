/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#if ((defined(_MSVC_LANG) && _MSVC_LANG < 201703L) || __cplusplus < 201703L)
#error "Usage of the Akonadi Async Storage API requires C++17 or higher"
#endif

#include "akonadicore_export.h"
#include "error.h"

#include <optional>
#include <memory>
#include <vector>
#include <functional>

#include <QEventLoop>
#include <QMetaType>
#include <QLoggingCategory>

AKONADICORE_EXPORT Q_DECLARE_LOGGING_CATEGORY(AKONADICORE_ASYNC_LOG)

namespace Akonadi
{

/**
 * API design
 *
 * @code
 * doStuff() -> Akonadi::Task<T>
 *
 * doStuff().then([](const T &) -> void { ... }) -> void;
 * doStuff().then([](const T &) -> Task<U> { ... }) -> Task<U>
 * doStuff().then([](const T &) -> void { ... }, [](const std::error_condition &) { ... });
 * doStuff().then([](const T &) -> Task<U> { ... }, [](const std::error_condition &) { ... }) -> Task<U>;
 * @endcode
 *
 * The error handler gets only called if the Task fails. When the error handler is invoked, any further
 * chained tasks is not called.
 *
 * When the error handler is not specified and the task fails, the default handler is not invoked
 * and the error is silently consumed. Further chained tasks are not called.
 */

template<typename Result, typename Error = Akonadi::Error> class TaskBase;
template<typename Result, typename Error = Akonadi::Error> class Task;
class TaskDataBase;
template<typename Result, typename Error = Akonadi::Error> class TaskData;

namespace traits {

template<typename T>
struct is_task : std::false_type {};

template<typename T>
struct is_task<Task<T>> : std::true_type {};

template<typename Func, typename ErrorType>
static constexpr bool is_error_handler_v = std::is_invocable_v<Func, ErrorType> || std::is_invocable_v<Func, const ErrorType &>;

template<typename T>
static constexpr bool is_task_v = is_task<T>::value;

template<typename Cont, typename Arg>
struct invoke_result : std::invoke_result<Cont, Arg> {};

template<typename Cont>
struct invoke_result<Cont, void> : std::invoke_result<Cont> {};

template<typename Cont, typename Arg>
using invoke_result_t = typename invoke_result<Cont, Arg>::type;

template<typename T, typename = void>
struct is_iterable : std::false_type {};

template<typename T>
struct is_iterable<T, std::void_t<decltype(std::begin(std::declval<T>())),
                                  decltype(std::end(std::declval<T>()))>> : std::true_type {};

template<typename T>
static constexpr bool is_iterable_v = is_iterable<T>::value;

} // namespace traits

class Observer;
class AKONADICORE_EXPORT TaskDataBase
{
public:
    void addObserver(Observer *observer);
    void removeObserver(Observer *observer);

protected:
    void notifyObservers();

protected:
    template<typename, typename> friend class TaskBase;
    template<typename, typename> friend class Task;

    std::vector<Observer *> m_observers;
    std::function<void()> m_cont;
};

class AKONADICORE_EXPORT Observer
{
public:
    virtual ~Observer() = default;
    virtual void notify() = 0;

protected:
    explicit Observer() = default;
};

template<typename T>
class AKONADICORE_EXPORT EventLoopObserver : public Observer
{
public:
    explicit EventLoopObserver(const TaskBase<TaskData<T>> &task)
        : m_task(task)
    {
        m_task.d->addObserver(this);
    }

    ~EventLoopObserver() override
    {
        m_task.d->removeObserver(this);
    }

    void wait()
    {
        if (m_task.isFinished()) {
            return;
        }

        m_loop.exec();
    }

private:
    void notify() override
    {
        m_loop.quit();
    }

    QEventLoop m_loop;
    TaskBase<TaskData<T>> m_task;
};

template<typename T>
EventLoopObserver(const Task<T> &) -> EventLoopObserver<T>;


class MultiObserver : public Observer
{
public:
    explicit MultiObserver() = default;
    ~MultiObserver() override = default;

    template<typename T>
    void addTask(TaskBase<TaskData<T>> task)
    {
        task.d->addObserver(this);
        ++mCount;
    }

    void notify() override
    {
        --mCount;
    }

private:
    int mCount = 0;
};



template<typename Result, typename Error>
class TaskData : public TaskDataBase
{
public:
    using result_type = Result;
    using error_type = Error;

    void setResult(const result_type &val)
    {
        Q_ASSERT(std::holds_alternative<std::monostate>(m_result));
        m_result = val;
        notifyObservers();
    }

    void setResult(result_type &&val)
    {
        Q_ASSERT(std::holds_alternative<std::monostate>(m_result));
        m_result = std::move(val);
        notifyObservers();
    }

    void setError(error_type &&error)
    {
        Q_ASSERT(std::holds_alternative<std::monostate>(m_result));
        m_result = std::move(error);
        notifyObservers();
    }

    bool hasError() const
    {
        return std::holds_alternative<error_type>(m_result);
    }

    bool hasResult() const
    {
        return std::holds_alternative<result_type>(m_result);
    }

    error_type error() const
    {
        return std::get<error_type>(m_result);
    }

    const result_type &result() const
    {
        return std::get<result_type>(m_result);
    }

    result_type &&result()
    {
        return std::move(std::get<result_type>(m_result));
    }

protected:
    template<typename, typename> friend class TaskBase;
    template<typename, typename> friend class Task;

    std::variant<std::monostate, result_type, error_type> m_result;
};

template<typename Error>
class TaskData<void, Error> : public TaskDataBase
{
public:
    using result_type = void;
    using error_type = Error;

    void setResult()
    {
        Q_ASSERT(std::holds_alternative<std::monostate>(m_result));
        m_result = true;
        notifyObservers();
    }

    void setError(error_type &&error)
    {
        Q_ASSERT(std::holds_alternative<std::monostate>(m_result));
        m_result = std::move(error);
        notifyObservers();
    }

    bool hasResult() const
    {
        return std::holds_alternative<bool>(m_result);
    }

    bool hasError() const
    {
        return std::holds_alternative<error_type>(m_result);
    }

    error_type error() const
    {
        return std::get<error_type>(m_result);
    }

protected:
    template<typename, typename> friend class TaskBase;
    template<typename, typename> friend class Task;

    std::variant<std::monostate, bool, error_type> m_result;
};

template<typename Result, typename Error>
class TaskBase;

template<typename Result, typename Error>
class TaskBase<TaskData<Result, Error>>
{
public:
    using result_type = Result;
    using error_type = Error;


    TaskBase(const TaskBase &) = default;
    TaskBase(TaskBase &&) noexcept = default;
    TaskBase &operator=(const TaskBase &) = default;
    TaskBase &operator=(TaskBase &&) noexcept = default;

    bool isFinished() const
    {
        return d->hasResult() || d->hasError();
    }

    bool hasError() const
    {
        return d->hasError();
    }

    error_type error() const
    {
        Q_ASSERT(hasError());
        return d->error();
    }

    void wait() const
    {
        EventLoopObserver<result_type>{*this}.wait();
    }

    template<typename Then, typename ErrorHandler, typename Out = traits::invoke_result_t<Then, Result>,
             typename = std::enable_if_t<!traits::is_task_v<Out>>>
    auto then(Then &&then, ErrorHandler &&error) -> void
    {
        static_assert(!traits::is_task_v<Out>);
        static_assert(traits::is_error_handler_v<ErrorHandler, error_type>, "ErrorHandler must be an invocable with error_type argument");

        this->d->m_cont = [then = std::forward<Then>(then), error = std::forward<ErrorHandler>(error), dptr = this->d]() mutable {
            if (dptr->hasError()) {
                std::invoke(error, dptr->error());
            } else {
                invoke(std::forward<Then>(then), dptr);
            }
        };

        if (this->isFinished()) {
            this->d->m_cont();
        }
    }

    template<typename Then, typename ErrorHandler, typename Out = traits::invoke_result_t<Then, result_type>,
             typename = std::enable_if_t<traits::is_task_v<Out>>>
    auto then(Then &&then, ErrorHandler &&error) -> Task<typename Out::result_type, error_type>
    {
        static_assert(traits::is_task_v<Out>);
        static_assert(traits::is_error_handler_v<ErrorHandler, error_type>, "ErrorHandler must be an invocable with error_type argument");

        Task<typename Out::result_type, error_type> outTask;

        this->d->m_cont = [then = std::forward<Then>(then), error = std::forward<ErrorHandler>(error), dptr = this->d, outTask]() mutable {
            if (dptr->hasError()) {
                std::invoke(error, dptr->error());
            } else {
                invokeContinuation(std::forward<Then>(then), dptr, outTask);
            }
        };

        if (this->isFinished()) {
            this->d->m_cont();
        }

        return outTask;
    }


    template<typename Then, typename Out = traits::invoke_result_t<Then, result_type>,
             typename = std::enable_if_t<!traits::is_task_v<Out>>>
    auto then(Then &&then) -> void
    {
        static_assert(!traits::is_task_v<Out>);

        this->d->m_cont = [then = std::forward<Then>(then), dptr = this->d]() mutable {
            if (dptr->hasError()) {
                // consume the error, do nothing
            } else {
                invoke(then, dptr);
            }
        };

        if (this->isFinished()) {
            this->d->m_cont();
        }
    }


    template<typename Then, typename Out = traits::invoke_result_t<Then, result_type>,
             typename = std::enable_if_t<traits::is_task_v<Out>>>
    auto then(Then &&then) -> Task<typename Out::result_type, error_type>
    {
        Task<typename Out::result_type, error_type> outTask;

        this->d->m_cont = [then = std::forward<Then>(then), dptr = this->d, outTask]() mutable {
            if (dptr->hasError()) {
                // consume the error, do nothing
            } else {
                invokeContinuation(std::forward<Then>(then), dptr, outTask);
            }
        };

        if (this->isFinished()) {
            this->d->m_cont();
        }

        return outTask;
    }

protected:
    template<typename, typename> friend class Task;
    template<typename> friend class EventLoopObserver;
    friend class MultiObserver;

    explicit TaskBase() = default;

    std::shared_ptr<TaskData<result_type, error_type>> d = std::make_shared<TaskData<result_type, error_type>>();

private:
    template<typename Then>
    static auto invoke(Then &&then, decltype(d) dptr)
    {
        if constexpr (std::is_void_v<Result>) {
            return std::invoke(std::forward<Then>(then));
        } else {
            return std::invoke(std::forward<Then>(then), dptr->result());
        }
    }

    template<typename Then, typename Out>
    static auto invokeContinuation(Then &&then, decltype(d) dptr, Out &&outTask)
    {
        if constexpr (std::is_void_v<typename std::remove_reference_t<Out>::result_type>) {
            invoke(std::forward<Then>(then), dptr).then([outTask]() mutable { outTask.setResult(); });
        } else {
            invoke(std::forward<Then>(then), dptr).then([outTask](auto &&result) mutable { outTask.setResult(result); });
        }
    }

};

/**
 * @brief Asynchronous task
 *
 * Error propagation
 *
 * Use onError() to handle and consume the error. Note, that then() continuation will
 * be always invoked afterwards. Use otherwise() or onSuccess() continuations to execute
 * code only if no error has occured in the parent task.
 */
template<typename Result, typename Error>
class Task : public TaskBase<TaskData<Result, Error>>
{
public:
    using result_type = Result;
    using error_type = Error;

    explicit Task()
    {
        qCDebug(AKONADICORE_ASYNC_LOG, "Task<%s> %p created",  typeName(), static_cast<void *>(this->d.get()));
    }

    void setResult(const result_type &result)
    {
        qCDebug(AKONADICORE_ASYNC_LOG, "Task<%s> %p has result set", typeName(), static_cast<void *>(this->d.get()));
        this->d->setResult(result);
    }

    void setResult(result_type &&result)
    {
        qCDebug(AKONADICORE_ASYNC_LOG, "Task<%s> %p has result set", typeName(), static_cast<void *>(this->d.get()));
        this->d->setResult(std::move(result));
    }

    std::enable_if_t<std::is_constructible_v<error_type, int, QString>>
    setError(int code, const QString &text = {})
    {
        qCDebug(AKONADICORE_ASYNC_LOG, "Task<%s> %p error set (code=%d)", typeName(), static_cast<void *>(this->d.get()), code);
        this->d->setError(error_type{code, text});
    }

    void setError(error_type error)
    {
        qCDebug(AKONADICORE_ASYNC_LOG, "Task<%s> %p error set (code=%d)", typeName(), static_cast<void *>(this->d.get()), error.code());
        this->d->setError(std::move(error));
    }

    const result_type &result() const
    {
        Q_ASSERT(this->d->hasResult());
        return this->d->result();
    }

    result_type &&result()
    {
        Q_ASSERT(this->d->hasResult());
        return this->d->result();
    }

private:
    static const char *typeName()
    {
        return QMetaType::typeName(qMetaTypeId<result_type>());
    }
};

template<typename Error>
class Task<void, Error> : public TaskBase<TaskData<void, Error>>
{
public:
    using result_type = void;
    using error_type = Error;

    explicit Task()
    {
        qCDebug(AKONADICORE_ASYNC_LOG, "Task<void> %p created", static_cast<void *>(this->d.get()));
    }

    void setResult()
    {
        qCDebug(AKONADICORE_ASYNC_LOG, "Task<void> %p has result set", static_cast<void *>(this->d.get()));
        this->d->setResult();
    }

    std::enable_if_t<std::is_constructible_v<error_type, int, QString>>
    setError(int code, const QString &text = {})
    {
        qCDebug(AKONADICORE_ASYNC_LOG, "Task<void> %p error set (code=%d)", static_cast<void *>(this->d.get()), code);
        this->d->setError(error_type{code, text});
    }

    void setError(error_type error)
    {
        qCDebug(AKONADICORE_ASYNC_LOG, "Task<void> %p error set (code=%d)", static_cast<void *>(this->d.get()), error.code());
        this->d->setError(std::move(error));
    }

};

template<template<typename> class Collection, typename Entity, typename ForEach>
auto taskForEach(const Collection<Entity> &collection, ForEach &&forEach) -> Task<void>
{
    Task<void> task;
    int *counter = new int{collection.size()};
    for (const auto &item : collection) {
        forEach(item).then(
            [task, counter](auto && ...) mutable {
                --(*counter);
                if (*counter == 0) {
                    delete counter;
                    if (!task.hasError()) {
                        task.setResult();
                    }
                }
            },
            [task, counter](const Error &error) mutable {
                --(*counter);
                task.setError(error);
            });
    }

    return task;
}

template<typename ForEach, typename Entity>
struct collectTasksHelper
{
    using task_type = std::invoke_result_t<ForEach, Entity>;
    using task_result_type = typename task_type::result_type;
    static constexpr bool is_result_iterable = traits::is_iterable_v<task_result_type>;
};

template<template<typename> class Collection, typename Entity, typename ForEach>
auto collectTasks(const Collection<Entity> &collection, ForEach &&forEach)
    -> std::enable_if_t<!collectTasksHelper<ForEach, Entity>::is_result_iterable,
                        Task<QVector<typename collectTasksHelper<ForEach, Entity>::task_result_type>>>
{
    static_assert(traits::is_task_v<std::invoke_result_t<ForEach, Entity>>);

    using TaskResultType = typename collectTasksHelper<ForEach, Entity>::task_result_type;

    struct RunData {
        int count = 0;
        QVector<TaskResultType> results;
    };
    RunData *results = new RunData{collection.size(), {}};

    Task<QVector<TaskResultType>> task;

    for (const auto &item : collection) {
        forEach(item).then(
            [task, results](auto && result) mutable {
                results->results.push_back(result);
                --(results->count);
                if (results->count == 0) {
                    if (!task.hasError()) {
                        task.setResult(results->results);
                    }
                    delete results;
                }
            },
            [task, results](const Error &error) mutable {
                --(results->count);
                task.setError(error);
            });
    }

    return task;
}

template<template<typename> class Collection, typename Entity, typename ForEach,
         typename = std::enable_if_t<collectTasksHelper<ForEach, Entity>::is_result_iterable>>
auto collectTasks(const Collection<Entity> &collection, ForEach &&forEach)
    -> std::enable_if_t<collectTasksHelper<ForEach, Entity>::is_result_iterable,
                        Task<QVector<typename collectTasksHelper<ForEach, Entity>::task_result_type::value_type>>>
{
    static_assert(traits::is_task_v<std::invoke_result_t<ForEach, Entity>>);

    using TaskResultType = typename collectTasksHelper<ForEach, Entity>::task_result_type;

    struct RunData {
        int count = 0;
        QVector<typename TaskResultType::value_type> results;
    };
    RunData *results = new RunData{collection.size(), {}};

    Task<QVector<typename TaskResultType::value_type>> task;

    for (const auto &item : collection) {
        forEach(item).then(
            [task, results](auto && result) mutable {
                std::copy(std::begin(result), std::end(result), std::back_inserter(results->results));
                --(results->count);
                if (results->count == 0) {
                    if (!task.hasError()) {
                        task.setResult(results->results);
                    }
                    delete results;
                }
            },
            [task, results](const Error &error) mutable {
                --(results->count);
                task.setError(error);
            });
    }

    return task;
}

template<typename T>
auto finishedTask(const T &val)
{
    Task<T> task;
    task.setResult(val);
    return task;
}

inline auto finishedTask()
{
    Task<void> task;
    task.setResult();
    return task;
}

} // namespace Akonadi

