// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/uninitializedQueue.h"

#include "sst/core/warnmacros.h"

#include <ostream>

namespace SST {

UninitializedQueue::UninitializedQueue(const std::string& message) :
    ActivityQueue(),
    message(message)
{}

DISABLE_WARN_MISSING_NORETURN
bool
UninitializedQueue::empty()
{
    std::cout << message << std::endl;
    abort();
}

int
UninitializedQueue::size()
{
    std::cout << message << std::endl;
    abort();
}

void
UninitializedQueue::insert(Activity* UNUSED(activity))
{
    std::cout << message << std::endl;
    abort();
}

Activity*
UninitializedQueue::pop()
{
    std::cout << message << std::endl;
    abort();
}

Activity*
UninitializedQueue::front()
{
    std::cout << message << std::endl;
    abort();
}
REENABLE_WARNING

} // namespace SST
