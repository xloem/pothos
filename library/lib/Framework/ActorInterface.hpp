// Copyright (c) 2015-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include <Pothos/Config.hpp>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

/*!
 * The implementation of the exclusive access to the actor.
 */
class ActorInterface
{
public:

    ActorInterface(void):
        _waitModeEnabled(true),
        _externalAcquired(0)
    {
        _changeFlagged.test_and_set();
    }

    virtual ~ActorInterface(void)
    {
        return;
    }

    /*!
     * External callers from outside of the thread context
     * may use this to acquire exclusive access to the actor.
     */
    void externalCallAcquire(void);

    //! Release external caller's exclusive access to the actor
    void externalCallRelease(void);

    /*!
     * Acquire exclusive access to the actor context.
     * \param waitEnabled true to enable CV waiting
     * \return true when acquired, false otherwise
     */
    bool workerThreadAcquire(const bool waitEnabled);

    //! Release exclusive access to the actor context.
    void workerThreadRelease(void);

    /*!
     * An external caller from outside the worker thread context
     * may use this to indicate that a state change has occurred.
     * This call marks the change and wakes up a sleeping thread.
     */
    void flagExternalChange(void);

    /*!
     * An internal call from within the worker thread context
     * may use this call to indicate an internal state change.
     * This call only marks the change because unlike flag external,
     * the worker thread is assumed to be active or making this call.
     */
    void flagInternalChange(void);

    /*!
     * Wake up a potentially sleeping thread without flagging a change.
     */
    void wakeNoChange(void);

    //! Enable or disable use of condition variables
    void enableWaitMode(const bool enb)
    {
        _waitModeEnabled = enb;
    }

private:
    bool _waitModeEnabled;
    std::atomic_flag _changeFlagged;
    std::atomic<size_t> _externalAcquired;
    std::mutex _contextMutex;
    std::mutex _acquireMutex;
    std::condition_variable _cond;
};

/*!
 * A lock-like class for ActorInterface to acquire exclusive access.
 * Use this to lock the actor interface when making external calls.
 */
class ActorInterfaceLock
{
public:
    ActorInterfaceLock(ActorInterface *actor):
        _actor(actor)
    {
        _actor->externalCallAcquire();
    }
    ~ActorInterfaceLock(void)
    {
        _actor->externalCallRelease();
    }
private:
    ActorInterface *_actor;
};

inline void ActorInterface::externalCallAcquire(void)
{
    _externalAcquired++;
    _contextMutex.lock();
}

inline void ActorInterface::externalCallRelease(void)
{
    _externalAcquired--;
    _contextMutex.unlock();
    this->flagExternalChange();
}

inline bool ActorInterface::workerThreadAcquire(const bool waitEnabled)
{
    //external context requested or in progress
    //block in here on a mutex lock and bail out
    if (_externalAcquired.load(std::memory_order_acquire) != 0)
    {
        std::lock_guard<std::mutex> lock(_contextMutex);
        return false;
    }

    //fast-check for already flagged case
    if (not _changeFlagged.test_and_set(std::memory_order_acquire))
    {
        _contextMutex.lock();
        return true;
    }

    //wait mode enabled -- lock and wait on condition variable
    if (waitEnabled)
    {
        std::unique_lock<std::mutex> lock(_acquireMutex);
        if (not _changeFlagged.test_and_set(std::memory_order_acquire))
        {
           _contextMutex.lock();
            return true;
        }
        _cond.wait(lock);
        if (not _changeFlagged.test_and_set(std::memory_order_acquire))
        {
           _contextMutex.lock();
            return true;
        }
        return false;
    }

    return false;
}

inline void ActorInterface::workerThreadRelease(void)
{
    _contextMutex.unlock();
}

inline void ActorInterface::flagExternalChange(void)
{
    //asynchronous indication
    _changeFlagged.clear(std::memory_order_release);

    //wake a blocked thread to process the change
    if (_waitModeEnabled) this->wakeNoChange();
}

inline void ActorInterface::wakeNoChange(void)
{
    //if lock fails, the worker context is busy
    if (not _contextMutex.try_lock()) return;
    _contextMutex.unlock();

    //otherwise we need to notify the waiting cv
    std::lock_guard<std::mutex> lock(_acquireMutex);
    _cond.notify_one();
}

inline void ActorInterface::flagInternalChange(void)
{
    _changeFlagged.clear(std::memory_order_release);
}
