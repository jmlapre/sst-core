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

#ifndef SST_CORE_STATAPI_STATENGINE_H
#define SST_CORE_STATAPI_STATENGINE_H

#include "sst/core/clock.h"
#include "sst/core/factory.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statfieldinfo.h"
#include "sst/core/statapi/statgroup.h"
#include "sst/core/statapi/statnull.h"
#include "sst/core/threadsafe.h"
#include "sst/core/unitAlgebra.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

/* Forward declare for Friendship */
extern int  main(int argc, char** argv);
extern void finalize_statEngineConfig();

namespace SST {
class BaseComponent;
class Simulation_impl;
class ConfigGraph;
class ConfigStatGroup;
class ConfigStatOutput;
class Params;
struct StatsConfig;

namespace Statistics {

// template<typename T> class Statistic;
// class StatisticBase;
class StatisticOutput;

/**
    \class StatisticProcessingEngine

    An SST core component that handles timing and event processing informing
    all registered Statistics to generate their outputs at desired rates.
*/

class StatisticProcessingEngine : public SST::Core::Serialization::serializable
{

public:
    /** Called by the Components and Subcomponent to perform a statistic Output.
     * @param stat - Pointer to the statistic.
     * @param EndOfSimFlag - Indicates that the output is occurring at the end of simulation.
     */
    void performStatisticOutput(StatisticBase* stat, bool endOfSimFlag = false);

    /** Called by the Components and Subcomponent to perform a global statistic Output.
     * This routine will force ALL Components and Subcomponents to output their statistic information.
     * This may lead to unexpected results if the statistic counts or data is reset on output.
     * @param endOfSimFlag - Indicates that the output is occurring at the end of simulation.
     */
    void performGlobalStatisticOutput(bool endOfSimFlag = false);

    template <class T>
    Statistic<T>* createStatistic(BaseComponent* comp, const std::string& type, const std::string& statName,
        const std::string& statSubId, Params& params)
    {
        return Factory::getFactory()->CreateWithParams<Statistic<T>>(type, params, comp, statName, statSubId, params);
    }

    bool registerStatisticWithEngine(StatisticBase* stat) { return registerStatisticCore(stat); }

    uint8_t statLoadLevel() const { return m_statLoadLevel; }

    // Outputs are per MPI rank, so have to be static data
    static const std::vector<StatisticOutput*>& getStatOutputs() { return m_statOutputs; }

    /** Called to setup the StatOutputs, which are shared across all
       the StatEngines on the same MPI rank.
     */
    static void static_setup(StatsConfig* stats_config);

    /** Called to notify StatOutputs that simulation has started
     */
    static void stat_outputs_simulation_start();

    /** Called to notify StatOutputs that simulation has ended
     */
    static void stat_outputs_simulation_end();

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Statistics::StatisticProcessingEngine)

private:
    friend class SST::Simulation_impl;
    friend int ::main(int argc, char** argv);
    friend void ::finalize_statEngineConfig();

    StatisticProcessingEngine();
    void setup(Simulation_impl* sim, StatsConfig* stats_config);
    void restart(Simulation_impl* sim);
    ~StatisticProcessingEngine();

    static StatisticOutput* createStatisticOutput(const ConfigStatOutput& cfg);

    bool registerStatisticCore(StatisticBase* stat);

    StatisticOutput* getOutputForStatistic(const StatisticBase* stat) const;
    StatisticGroup&  getGroupForStatistic(const StatisticBase* stat) const;
    bool             addPeriodicBasedStatistic(const UnitAlgebra& freq, StatisticBase* Stat);
    bool             addEventBasedStatistic(const UnitAlgebra& count, StatisticBase* Stat);
    bool             addEndOfSimStatistic(StatisticBase* Stat);
    UnitAlgebra      getParamTime(StatisticBase* stat, const std::string& pName) const;
    void             setStatisticStartTime(StatisticBase* Stat);
    void             setStatisticStopTime(StatisticBase* Stat);

    void finalizeInitialization(); /* Called when performWireUp() finished */
    void startOfSimulation();
    void endOfSimulation();

    void performStatisticOutputImpl(StatisticBase* stat, bool endOfSimFlag);
    void performStatisticGroupOutputImpl(StatisticGroup& group, bool endOfSimFlag);

    bool handleStatisticEngineClockEvent(Cycle_t CycleNum, SimTime_t timeFactor);
    bool handleGroupClockEvent(Cycle_t CycleNum, StatisticGroup* group);
    void handleStatisticEngineStartTimeEvent(SimTime_t timeFactor);
    void handleStatisticEngineStopTimeEvent(SimTime_t timeFactor);

    void addStatisticToCompStatMap(StatisticBase* Stat, StatisticFieldInfo::fieldType_t fieldType);

    [[noreturn]]
    void castError(const std::string& type, const std::string& statName, const std::string& fieldName);

private:
    using StatArray_t   = std::vector<StatisticBase*>;           /*!< Array of Statistics */
    using StatMap_t     = std::map<SimTime_t, StatArray_t*>;     /*!< Map of simtimes to Statistic Arrays */
    using CompStatMap_t = std::map<ComponentId_t, StatArray_t*>; /*!< Map of ComponentId's to StatInfo Arrays */

    StatArray_t   m_EventStatisticArray;  /*!< Array of Event Based Statistics */
    StatMap_t     m_PeriodicStatisticMap; /*!< Map of Array's of Periodic Based Statistics */
    StatMap_t     m_StartTimeMap;         /*!< Map of Array's of Statistics that are started at a sim time */
    StatMap_t     m_StopTimeMap;          /*!< Map of Array's of Statistics that are stopped at a sim time */
    CompStatMap_t m_CompStatMap;          /*!< Map of Arrays of Statistics tied to Component Id's */
    bool          m_SimulationStarted;    /*!< Flag showing if Simulation has started */

    Simulation_impl*            m_sim;
    Output&                     m_output;
    uint8_t                     m_statLoadLevel;
    StatisticGroup              m_defaultGroup;
    std::vector<StatisticGroup> m_statGroups;

    // Outputs are per MPI rank, so have to be static data
    static std::vector<StatisticOutput*> m_statOutputs;
};

} // namespace Statistics
} // namespace SST

#endif // SST_CORE_STATAPI_STATENGINE_H
