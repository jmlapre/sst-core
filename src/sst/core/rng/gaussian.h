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

#ifndef SST_CORE_RNG_GAUSSIAN_H
#define SST_CORE_RNG_GAUSSIAN_H

#include "distrib.h"
#include "math.h"
#include "mersenne.h"
#include "rng.h"

namespace SST::RNG {

/**
    \class GaussianDistribution gaussian.h "sst/core/rng/gaussian.h"

    Creates a Gaussian (normal) distribution for which to sample
*/
class GaussianDistribution : public RandomDistribution
{

public:
    /**
        Creates a new distribution with a predefined random number generator with a specified mean and standard
       deviation. \param mn The mean of the Gaussian distribution \param sd The standard deviation of the Gaussian
       distribution
    */
    GaussianDistribution(double mn, double sd) :
        RandomDistribution()
    {

        mean   = mn;
        stddev = sd;

        baseDistrib   = new MersenneRNG();
        unusedPair    = 0;
        usePair       = false;
        deleteDistrib = true;
    }

    /**
        Creates a new distribution with a predefined random number generator with a specified mean and standard
       deviation. \param mn The mean of the Gaussian distribution \param sd The standard deviation of the Gaussian
       distribution \param baseRNG The random number generator as the base of the distribution
    */
    GaussianDistribution(double mn, double sd, Random* baseRNG) :
        RandomDistribution()
    {

        mean   = mn;
        stddev = sd;

        baseDistrib   = baseRNG;
        unusedPair    = 0;
        usePair       = false;
        deleteDistrib = false;
    }

    /**
        Destroys the Gaussian distribution.
    */
    ~GaussianDistribution()
    {
        if ( deleteDistrib ) {
            delete baseDistrib;
        }
    }

    /**
        Gets the next double value in the distribution
        \return The next double value of the distribution (in this case a Gaussian distribution)
    */
    double getNextDouble() override
    {
        if ( usePair ) {
            usePair = false;
            return unusedPair;
        }
        else {
            double gauss_u, gauss_v, sq_sum;

            do {
                gauss_u = baseDistrib->nextUniform();
                gauss_v = baseDistrib->nextUniform();
                sq_sum  = (gauss_u * gauss_u) + (gauss_v * gauss_v);
            } while ( sq_sum >= 1 || sq_sum == 0 );

            if ( baseDistrib->nextUniform() < 0.5 ) {
                gauss_u *= -1.0;
            }

            if ( baseDistrib->nextUniform() < 0.5 ) {
                gauss_v *= -1.0;
            }

            double multiplier = sqrt(-2.0 * log(sq_sum) / sq_sum);
            unusedPair        = mean + stddev * gauss_v * multiplier;
            usePair           = true;

            return mean + stddev * gauss_u * multiplier;
        }
    }

    /**
        Gets the mean of the distribution
        \return The mean of the Guassian distribution
    */
    double getMean() { return mean; }

    /**
        Gets the standard deviation of the distribution
        \return The standard deviation of the Gaussian distribution
    */
    double getStandardDev() { return stddev; }

    /**
        Default constructor. FOR SERIALIZATION ONLY.
     */
    GaussianDistribution() :
        RandomDistribution()
    {}

    /**
        Serialization function for checkpoint
    */
    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(mean);
        SST_SER(stddev);
        SST_SER(baseDistrib);
        SST_SER(unusedPair);
        SST_SER(usePair);
        SST_SER(deleteDistrib);
    }

    /**
        Serialization macro
    */
    ImplementSerializable(SST::RNG::GaussianDistribution)

protected:
    /**
        The mean of the Gaussian distribution
    */
    double  mean;
    /**
        The standard deviation of the Gaussian distribution
    */
    double  stddev;
    /**
        The base random number generator for the distribution
    */
    Random* baseDistrib;
    /**
        Random numbers for the distribution are read in pairs, this stores the second of the pair
    */
    double  unusedPair;
    /**
        Random numbers for the distribution are read in pairs, this tells the code to use the second of the pair
    */
    bool    usePair;

    /**
        Controls whether the destructor deletes the distribution (we need to ensure we do this IF we created the
       distribution)
    */
    bool deleteDistrib;
};

} // namespace SST::RNG

using SSTGaussianDistribution = SST::RNG::GaussianDistribution;

#endif // SST_CORE_RNG_GAUSSIAN_H
