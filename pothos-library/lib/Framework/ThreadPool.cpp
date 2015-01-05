// Copyright (c) 2014-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework/ThreadPool.hpp>
#include <Pothos/Framework/Exception.hpp>
#include "Framework/ThreadEnvironment.hpp"

Pothos::ThreadPoolArgs::ThreadPoolArgs(void):
    numThreads(0),
    priority(0.0)
{
    return;
}

Pothos::ThreadPoolArgs::ThreadPoolArgs(const size_t numThreads):
    numThreads(numThreads),
    priority(0.0)
{
    return;
}

Pothos::ThreadPool::ThreadPool(void)
{
    return;
}

Pothos::ThreadPool::ThreadPool(const std::shared_ptr<void> &impl):
    _impl(impl)
{
    return;
}

Pothos::ThreadPool::ThreadPool(const ThreadPoolArgs &args)
{
    //validate the arguments
    if (args.affinityMode.empty()){}
    else if (args.affinityMode == "ALL"){}
    else if (args.affinityMode == "CPU"){}
    else if (args.affinityMode == "NUMA"){}
    else throw ThreadPoolError("Pothos::ThreadPool()", "unknown affinityMode " + args.affinityMode);

    //validate the yield strategy
    if (args.yieldMode.empty()){}
    else if (args.yieldMode == "CONDITION"){}
    else if (args.yieldMode == "HYBRID"){}
    else if (args.yieldMode == "SPIN"){}
    else throw ThreadPoolError("Pothos::ThreadPool()", "unknown yieldMode " + args.yieldMode);

    //validate the thread priority
    if (args.priority > +1.0 or args.priority < -1.0)
    {
        throw ThreadPoolError("Pothos::ThreadPool()", "priority out of range " + std::to_string(args.priority));
    }

    //safe to create the thread environment
    _impl.reset(new ThreadEnvironment(args));
}

Pothos::ThreadPool::operator bool(void) const
{
    return bool(_impl);
}

const std::shared_ptr<void> &Pothos::ThreadPool::getContainer(void) const
{
    return _impl;
}

bool Pothos::operator==(const ThreadPool &lhs, const ThreadPool &rhs)
{
    return lhs.getContainer() == rhs.getContainer();
}

#include <Pothos/Managed.hpp>

static auto managedThreadPoolArgs = Pothos::ManagedClass()
    .registerConstructor<Pothos::ThreadPoolArgs>()
    .registerConstructor<Pothos::ThreadPoolArgs, const size_t>()
    .registerField(POTHOS_FCN_TUPLE(Pothos::ThreadPoolArgs, numThreads))
    .registerField(POTHOS_FCN_TUPLE(Pothos::ThreadPoolArgs, priority))
    .registerField(POTHOS_FCN_TUPLE(Pothos::ThreadPoolArgs, affinityMode))
    .registerField(POTHOS_FCN_TUPLE(Pothos::ThreadPoolArgs, affinity))
    .registerField(POTHOS_FCN_TUPLE(Pothos::ThreadPoolArgs, yieldMode))
    .commit("Pothos/ThreadPoolArgs");

static auto managedThreadPool = Pothos::ManagedClass()
    .registerConstructor<Pothos::ThreadPool>()
    .registerConstructor<Pothos::ThreadPool, const std::shared_ptr<void> &>()
    .registerConstructor<Pothos::ThreadPool, const Pothos::ThreadPoolArgs &>()
    .registerMethod(POTHOS_FCN_TUPLE(Pothos::ThreadPool, getContainer))
    .registerStaticMethod<const Pothos::ThreadPool &, const Pothos::ThreadPool &>("equal", Pothos::operator==)
    .commit("Pothos/ThreadPool");

#include <Pothos/Object/Serialize.hpp>

namespace Pothos { namespace serialization {
template <class Archive>
void serialize(Archive &ar, Pothos::ThreadPoolArgs &t, const unsigned int)
{
    ar & t.numThreads;
    ar & t.priority;
    ar & t.affinityMode;
    ar & t.affinity;
    ar & t.yieldMode;
}
}}

POTHOS_OBJECT_SERIALIZE(Pothos::ThreadPoolArgs)
