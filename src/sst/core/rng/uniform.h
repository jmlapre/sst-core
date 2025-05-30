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

#ifndef SST_CORE_RNG_UNIFORM_H
#define SST_CORE_RNG_UNIFORM_H

#include "distrib.h"
#include "math.h"
#include "mersenne.h"
#include "rng.h"

#include <cstdint>

namespace SST::RNG {

/**
    \class UniformDistribution uniform.h "sst/core/rng/uniform.h"

    Creates a Uniform distribution for use within SST. This distribution is the same across
    platforms and compilers.
*/
class UniformDistribution : public RandomDistribution
{

public:
    /**
        Creates an uniform distribution with a specific number of bins
        \param probsCount Number of probability bins in this distribution
    */
    explicit UniformDistribution(const uint32_t probsCount) :
        RandomDistribution(),
        deleteDistrib(true),
        probCount(probsCount),
        probPerBin(1)
    {

        if ( probCount > 0 ) {
            probPerBin = 1.0 / static_cast<double>(probCount);
        }

        baseDistrib = new MersenneRNG();
    }

    /**
            Creates a Uniform distribution with a specific number of bins and user supplied
            random number generator
            \param probsCount Number of probability bins in the distribution
            \param baseDist The base random number generator to take the distribution from.
    */
    UniformDistribution(const uint32_t probsCount, Random* baseDist) :
        RandomDistribution(),
        deleteDistrib(false),
        probCount(probsCount),
        probPerBin(1)
    {

        if ( probCount > 0 ) {
            probPerBin = 1.0 / static_cast<double>(probCount);
        }

        baseDistrib = baseDist;
    }

    /**
        Destroys the distribution and will delete locally allocated RNGs
    */
    ~UniformDistribution()
    {
        if ( deleteDistrib ) {
            delete baseDistrib;
        }
    }

    /**
        Gets the next (random) double value in the distribution
        \return The next random double from the distribution, this is the double converted of the index where the
       probability is located
    */
    double getNextDouble() override
    {
        const double nextD       = baseDistrib->nextUniform();
        uint32_t     current_bin = 1;

        while ( nextD > (static_cast<double>(current_bin) * probPerBin) ) {
            current_bin++;
        }

        return static_cast<double>(current_bin - 1);
    }

    /**
        Default constructor. FOR SERIALIZATION ONLY.
     */
    UniformDistribution() :
        RandomDistribution(),
        deleteDistrib(true),
        probCount(0)
    {}

    /**
        Serialization function for checkpoint
    */
    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(baseDistrib);
        SST_SER(const_cast<bool&>(deleteDistrib));
        SST_SER(const_cast<uint32_t&>(probCount));
        SST_SER(probPerBin);
    }

    /**
        Serialization macro
    */
    ImplementSerializable(SST::RNG::UniformDistribution)

protected:
    /**
        Sets the base random number generator for the distribution.
    */
    Random* baseDistrib;

    /**
        Controls whether the base distribution should be deleted when this class is destructed.
    */
    const bool deleteDistrib;

    /**
        Count of discrete probabilities
    */
    const uint32_t probCount;

    /**
        Range 0..1 split into discrete bins
    */
    double probPerBin;
};

} // namespace SST::RNG

using SSTUniformDistribution = SST::RNG::UniformDistribution;

#endif // SST_CORE_RNG_UNIFORM_H
