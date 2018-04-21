/*

Call manager.

Copyright (C) 2014 Sergey Kolevatov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

*/


// $Revision: 8945 $ $Date:: 2018-04-20 #$ $Author: serge $

#ifndef CALL_MANAGER_H
#define CALL_MANAGER_H

#include <list>                             // std::list
#include <mutex>                            // std::mutex
#include <map>                              // std::map
#include <set>                              // std::set

#include "config.h"                         // Config
#include "simple_voip/objects.h"            // simple_voip::InitiateCallRequest
#include "../simple_voip/i_simple_voip.h"   // simple_voip::ISimpleVoip
#include "../simple_voip/i_simple_voip_callback.h"     // ISimpleVoipCallback

#include "namespace_lib.h"              // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

class CallManager;

class CallManager:
    virtual public simple_voip::ISimpleVoip,
    virtual public simple_voip::ISimpleVoipCallback
{
public:
    CallManager();
    ~CallManager();

    bool init( simple_voip::ISimpleVoip * voips, const Config & cfg );

    bool register_callback( simple_voip::ISimpleVoipCallback * callback );

    // interface ISimpleVoip
    void consume( const simple_voip::ForwardObject* obj );

    // interface ISimpleVoipCallback
    void consume( const simple_voip::CallbackObject * obj );

    // interface threcon::IControllable
    bool shutdown();

private:

    typedef std::list<const simple_voip::InitiateCallRequest*>  RequestQueue;

    typedef std::set<uint32_t>              SetReqIds;
    typedef std::set<uint32_t>              SetCallIds;
    typedef std::map<uint32_t, uint32_t>    MapReqIdToCallId;

private:

    void process( const simple_voip::InitiateCallRequest * req );

    // simple_voip::ISimpleVoip interface
    void handle( const simple_voip::InitiateCallRequest * req );
    void handle( const simple_voip::DropRequest * req );

    // interface ISimpleVoipCallback
    void handle( const simple_voip::InitiateCallResponse * obj );
    void handle( const simple_voip::RejectResponse * obj );
    void handle( const simple_voip::ErrorResponse * obj );
    void handle( const simple_voip::DropResponse * obj );
    void handle( const simple_voip::ConnectionLost * obj );
    void handle( const simple_voip::Failed * obj );

    void erase_failed_drop_request( uint32_t req_id );
    void handle_failed_call( uint32_t call_id );

    uint32_t get_num_of_activities() const;

    void process_jobs();

private:
    mutable std::mutex          mutex_;

    Config                      cfg_;

    RequestQueue                request_queue_;

    simple_voip::ISimpleVoip  * voips_;
    simple_voip::ISimpleVoipCallback        * callback_;

    SetReqIds                   active_request_ids_;
    SetCallIds                  active_call_ids_;
    MapReqIdToCallId            map_drop_req_id_to_call_id_;
};

NAMESPACE_CALMAN_END

#endif  // CALL_MANAGER_H
