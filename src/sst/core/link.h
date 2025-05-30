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

#ifndef SST_CORE_LINK_H
#define SST_CORE_LINK_H

#include "sst/core/event.h"
#include "sst/core/rankInfo.h"
#include "sst/core/serialization/serialize_impl_fwd.h"
#include "sst/core/sst_types.h"
#include "sst/core/timeConverter.h"

#include <cstdint>
#include <string>
#include <vector>

namespace SST {

#define _LINK_DBG(fmt, args...) __DBG(DBG_LINK, Link, fmt, ##args)

class ActivityQueue;
class BaseComponent;
class TimeConverter;
class LinkPair;
class Simulation_impl;

class UnitAlgebra;

namespace Profile {
class EventHandlerProfileTool;
}

class Link;

template <>
class SST::Core::Serialization::serialize_impl<Link*>
{
    // Function implemented in link.cc
    void operator()(Link*& s, SST::Core::Serialization::serializer& ser, ser_opt_t options);
    void serialize_events(
        SST::Core::Serialization::serializer& ser, uintptr_t delivery_info, ActivityQueue* queue = nullptr);

    SST_FRIEND_SERIALIZE();
};


/** Link between two components. Carries events */
class alignas(64) Link
{
    enum Type_t : uint16_t { POLL, HANDLER, SYNC, UNINITIALIZED };
    enum Mode_t : uint16_t { INIT, RUN, COMPLETE };

    friend class SST::Core::Serialization::serialize_impl<Link*>;

public:
    /**
       Attach point for inspecting, modifying or dropping events
       sent on the Link.

       NOTE: Using the Link::AttachPoint will noticeably affect the
       performance of sending events on Links and it is recommended
       that, if possible, Event::HandlerBase::AttachPoint or
       Event::HandlerBase::InterceptPoint be used instead.
     */
    class AttachPoint
    {
    public:
        /**
           Function that will be called when an attach point is
           registered with the tool implementing the attach point.
           The metadata passed in will be dependent on what type of
           tool this is attached to.  The uintptr_t returned from this
           function will be passed into the eventSent() function.

           @param mdata Metadata to be passed into the tool

           @return Opaque key that will be passed back into
           eventSent() to identify the source of the call
         */
        virtual uintptr_t registerLinkAttachTool(const AttachPointMetaData& mdata) = 0;

        /**
           Function that will be called when an event is sent on a
           link with registered attach points.  If ev is set to
           nullptr, then the event will not be delivered and the tool
           should delete the original event.

           @param key Opaque key returned from registerLinkAttachTool()
         */
        virtual void eventSent(uintptr_t key, Event*& ev) = 0;

        /**
           Function that will be called to handle the key returned
           from registerLinkAttachTool, if the AttachPoint tool is
           serializable.  This is needed because the key is opaque to
           the Link, so it doesn't know how to handle it during
           serialization.  During SIZE and PACK phases of
           serialization, the tool needs to store out any information
           that will be needed to recreate data that is reliant on the
           key.  On UNPACK, the function needs to recreate any state
           and reinitialize the passed in key reference to the proper
           state to continue to make valid calls to eventSent().

           Since not all tools will be serializable, there is a
           default, empty implementation.

           @param ser Serializer to use for serialization

           @param key Key that would be passed into the eventSent() function.
         */
        virtual void serializeEventAttachPointKey(SST::Core::Serialization::serializer& ser, uintptr_t& key);

        virtual ~AttachPoint() = default;
    };

    friend class LinkPair;
    friend class RankSync;
    friend class ThreadSync;
    friend class Simulation_impl;
    friend class SyncManager;
    friend class ComponentInfo;

    ~Link();

    /** Set additional Latency to be added to events being sent out of this link
     * @param cycles Number of Cycles to be added
     * @param timebase Base Units of cycles
     */
    void addSendLatency(int cycles, const std::string& timebase);

    /** Set additional Latency to be added to events being sent out of this link
     * @param cycles Number of Cycles to be added
     * @param timebase Base Units of cycles
     */
    [[deprecated("Use of shared TimeConverter objects is deprecated. Use 'addSendLatency(SimTime_t cycles, "
                 "TimeConverter timebase)' (i.e., no pointer) instead.")]]
    void addSendLatency(SimTime_t cycles, TimeConverter* timebase);
    void addSendLatency(SimTime_t cycles, TimeConverter timebase);

    /** Set additional Latency to be added on to events coming in on this link.
     * @param cycles Number of Cycles to be added
     * @param timebase Base Units of cycles
     */
    void addRecvLatency(int cycles, const std::string& timebase);

    /** Set additional Latency to be added on to events coming in on this link.
     * @param cycles Number of Cycles to be added
     * @param timebase Base Units of cycles
     */
    [[deprecated("Use of shared TimeConverter objects is deprecated. Use 'addRecvLatency(SimTime_t cycles, "
                 "TimeConverter timebase)' (i.e., no pointer) instead.")]]
    void addRecvLatency(SimTime_t cycles, TimeConverter* timebase);
    void addRecvLatency(SimTime_t cycles, TimeConverter timebase);

    /** Set the callback function to be called when a message is
     * delivered. Not available for Polling links.
     * @param functor Functor to call when message is delivered
     */
    void setFunctor(Event::HandlerBase* functor);

    /** Replace the callback function to be called when a message is
     * delivered. Any previous handler will be deleted.
     * Not available for Polling links.
     * @param functor Functor to call when message is delivered
     */
    void replaceFunctor(Event::HandlerBase* functor);

    /** Get the callback function to be called when a message is
     * delivered. Polling links will return nullptr.
     */
    Event::HandlerBase* getFunctor();

    /** Send an event over the link with additional delay. Sends an event
     * over a link with an additional delay specified with a
     * TimeConverter. I.e. the total delay is the link's delay + the
     * additional specified delay.
     * @param delay - additional delay
     * @param tc - time converter to specify units for the additional delay
     * @param event - the Event to send
     */
    [[deprecated(
        "Use of shared TimeConverter objects is deprecated. Use 'send(SimTime_t delay, const TimeConverter& tc, "
        "Event* event)' instead.")]]
    inline void send(SimTime_t delay, TimeConverter* tc, Event* event)
    {
        send(delay, *tc, event);
    }

    /** Send an event over the link with additional delay. Sends an event
     * over a link with an additional delay specified with a
     * TimeConverter. I.e. the total delay is the link's delay + the
     * additional specified delay.
     * @param delay - additional delay
     * @param tc - time converter to specify units for the additional delay
     * @param event - the Event to send
     */
    inline void send(SimTime_t delay, TimeConverter tc, Event* event) { send_impl(tc.convertToCoreTime(delay), event); }


    /** Send an event with additional delay. Sends an event over a link
     * with additional delay specified by the Link's default
     * timebase.
     * @param delay The additional delay, in units of the default Link timebase
     * @param event The event to send
     */
    inline void send(SimTime_t delay, Event* event) { send_impl(delay * defaultTimeBase, event); }

    /** Send an event with the Link's default delay
     * @param event The event to send
     */
    inline void send(Event* event) { send_impl(0, event); }


    /** Retrieve a pending event from the Link. For links which do not
     * have a set event handler, they can be polled with this function.
     * Returns nullptr if there is no pending event.
     * Not available for HANDLER-type links.
     * @return Event if one is available
     * @return nullptr if no Event is available
     */
    Event* recv();

    /** Manually set the default defaultTimeBase
     * @param tc TimeConverter object for the timebase
     */
    [[deprecated("Use of shared TimeConverter objects is deprecated. Use 'setDefaultTimeBase(TimeConverter tc)', "
                 "(i.e., no pointer) instead.")]]
    void setDefaultTimeBase(TimeConverter* tc);

    /** Manually set the default defaultTimeBase
     * @param tc TimeConverter object for the timebase
     */
    void setDefaultTimeBase(TimeConverter tc);

    /** Return the default Time Base for this link
     * @return the default Time Base for this link
     */
    TimeConverter* getDefaultTimeBase();

    /** Return the default Time Base for this link
     * @return the default Time Base for this link
     */
    const TimeConverter* getDefaultTimeBase() const;

    /** Return the ID of this link
     * @return the unique ID for this link
     */
    LinkId_t getId() { return tag; }

    /** Send data during the init() or complete() phase.
     * @param data event to send
     */
    void sendUntimedData(Event* data);

    /** Receive an event (if any) during the init() or complete() phase.
     * @return Event if one is available
     * @return nullptr if no Event is available
     */
    Event* recvUntimedData();

    /** Return whether link has been configured
     * @return whether link is configured
     */
    bool isConfigured() { return type != UNINITIALIZED; }

#ifdef __SST_DEBUG_EVENT_TRACKING__
    void setSendingComponentInfo(const std::string& comp_in, const std::string& type_in, const std::string& port_in)
    {
        comp  = comp_in;
        ctype = type_in;
        port  = port_in;
    }

    const std::string& getSendingComponentName() { return comp; }
    const std::string& getSendingComponentType() { return ctype; }
    const std::string& getSendingPort() { return port; }

#endif

protected:
    Link();

    void setAsSyncLink() { type = SYNC; }

    /**
       Set the delivery_info for the link
     */
    void setDeliveryInfo(uintptr_t info) { delivery_info = info; }

    /** Send an event over the link with additional delay. Sends an event
     * over a link with an additional delay specified with a
     * TimeConverter. I.e. the total delay is the link's delay + the
     * additional specified delay.
     * @param delay - additional total delay to add
     * @param event - the Event to send
     */
    void send_impl(SimTime_t delay, Event* event);

    /**
       Updates the delivery info in an event.  This is used during a
       restart and is implemented here because Link is a friend of
       event.

       @param event - Event to update

       @param delivery_info New delivery info (pointer to handler cast
       as uintptr_t)
     */
    static void updateEventDeliveryInfo(Event* event, uintptr_t delivery_info)
    {
        event->updateDeliveryInfo(delivery_info);
    }

    // Since Links are found in pairs, I will keep all the information
    // needed for me to send and deliver an event to the other side of
    // the link.  That means, that I mostly keep my pair's
    // information.  The one consequence, is that polling links will
    // have to pull the data from the pair, but since this is a less
    // common case, that's okay (this decision makes the common case
    // faster and the less common case slower).

    /** Queue of events to be received by the owning component */
    ActivityQueue* send_queue;

    /** Holds the delivery information.  This is stored as a
      uintptr_t, but is actually a pointer converted using
      reinterpret_cast.  For links connected to a
      Component/SubComponent, this holds a pointer to the delivery
      functor.  For links connected to a Sync object, this holds a
      pointer to the remote link to send the event on after
      synchronization.
    */
    uintptr_t delivery_info;

    /** Timebase used if no other timebase is specified. Used to specify
      the units for added delays when sending, such as in
      Link::send(). Often set by the Component::registerClock()
      function if the regAll argument is true.
      */
    SimTime_t defaultTimeBase;

    /** Latency of the link. It is used by the partitioner as the
      weight. This latency is added to the delay put on the event by
      the component.
    */
    SimTime_t latency;

    /** Pointer to the opposite side of this link */
    Link* pair_link;

private:
    friend class BaseComponent;

    SimTime_t& current_time;
    Type_t     type;
    Mode_t     mode;
    LinkId_t   tag;

    /** Create a new link with a given tag

        The tag is used for two different things depending on where
        this link sends data:

        If it sends it to a Sync object, then it represents the
        remote_tag used to lookup the correct link on the other side.

        If it sends to a TimeVortex (or DirectLinkQueue), it is the
        value used for enforce_link_order (if that feature is
        enabled).
     */
    explicit Link(LinkId_t tag);

    Link(const Link& l);

    /** Specifies that this link has no callback, and is poll-based only */
    void setPolling();

    /** Causes an event to be delivered to the registered callback */
    inline void deliverEvent(Event* event) const { (*reinterpret_cast<Event::HandlerBase*>(delivery_info))(event); }

    /** Set minimum link latency */
    void setLatency(Cycle_t lat);

    void sendUntimedData_sync(Event* data);
    void finalizeConfiguration();
    void prepareForComplete();

    std::string createUniqueGlobalLinkName(
        RankInfo local_rank, uintptr_t local_ptr, RankInfo remote_rank, uintptr_t remote_ptr);


    void attachTool(AttachPoint* tool, const AttachPointMetaData& mdata);
    void detachTool(AttachPoint* tool);


    using ToolList = std::vector<std::pair<AttachPoint*, uintptr_t>>;
    ToolList* attached_tools;

    /** Manually set the default time base
     * @param factor SimTime_T defining the timebase factor
     */
    void setDefaultTimeBase(SimTime_t factor) { defaultTimeBase = factor; }

    /** Set the default time base fo uninitialized */
    void resetDefaultTimeBase() { defaultTimeBase = 0; }


#ifdef __SST_DEBUG_EVENT_TRACKING__
    std::string comp;
    std::string ctype;
    std::string port;
#endif
};

/** Self Links are links from a component to itself */
class SelfLink : public Link
{
public:
    SelfLink() :
        Link()
    {
        pair_link = this;
        latency   = 0;
    }
};


} // namespace SST

#endif // SST_CORE_LINK_H
