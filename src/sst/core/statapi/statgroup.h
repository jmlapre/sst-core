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

#ifndef SST_CORE_STATAPI_STATGROUP_H
#define SST_CORE_STATAPI_STATGROUP_H

#include "sst/core/serialization/serializer.h"
#include "sst/core/sst_types.h"
#include "sst/core/unitAlgebra.h"

#include <string>
#include <vector>

namespace SST {
class ConfigStatGroup;
namespace Statistics {
class StatisticBase;
class StatisticOutput;
class StatisticProcessingEngine;

/* A group of statistics that share a statistic output object*/
class StatisticGroup
{
public:
    StatisticGroup() :
        isDefault(true),
        name("default") {};
    StatisticGroup(const ConfigStatGroup& csg, StatisticProcessingEngine* engine);

    bool containsStatistic(const StatisticBase* stat) const;
    bool claimsStatistic(const StatisticBase* stat) const;
    void addStatistic(StatisticBase* stat);

    bool             isDefault;
    std::string      name;
    StatisticOutput* output;
    UnitAlgebra      outputFreq;
    size_t           outputId;

    std::vector<ComponentId_t>  components;
    std::vector<std::string>    statNames;
    std::vector<StatisticBase*> stats;

    void restartGroup(StatisticProcessingEngine* engine);
    void serialize_order(SST::Core::Serialization::serializer& ser);
};

} // namespace Statistics
} // namespace SST

#endif // SST_CORE_STATAPI_STATGROUP_H
