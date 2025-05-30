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

#ifndef SST_CORE_RNG_DISTRIB_H
#define SST_CORE_RNG_DISTRIB_H

#include "sst/core/serialization/serializable.h"

namespace SST::RNG {

/**
 * \class RandomDistribution
 * Base class of statistical distributions in SST.
 */
class RandomDistribution : public SST::Core::Serialization::serializable
{

public:
    /**
        Obtains the next double from the distribution
        \return The next double in the distribution being sampled
    */
    virtual double getNextDouble() = 0;

    /**
        Destroys the distribution
    */
    virtual ~RandomDistribution() {}

    /**
        Creates the base (abstract) class of a distribution
    */
    RandomDistribution() {}

    virtual void serialize_order(SST::Core::Serialization::serializer& UNUSED(ser)) override {}

    ImplementVirtualSerializable(SST::RNG::RandomDistribution)
};

} // namespace SST::RNG

using SSTRandomDistribution = SST::RNG::RandomDistribution;

#endif // SST_CORE_RNG_DISTRIB_H
