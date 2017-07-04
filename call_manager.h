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


// $Revision: 5739 $ $Date:: 2017-02-09 #$ $Author: serge $

#ifndef CALL_MANAGER_H
#define CALL_MANAGER_H

#include <list>                             // std::list
#include <mutex>                            // std::mutex
#include <map>                              // std::map

#include "call.h"                           // Call
#include "config.h"                         // Config
#include "../simple_voip/i_simple_voip.h"   // simple_voip::ISimpleVoip
#include "../simple_voip/i_simple_voip_callback.h"     // ISimpleVoipCallback
#include "../threcon/i_controllable.h"      // IControllable
#include "../workt/worker_t.h"              // WorkerT

#include "namespace_lib.h"              // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

class CallManager;

typedef workt::WorkerT< const simple_voip::IObject*, CallManager> WorkerBase;

class CallManager: public WorkerBase,
    virtual public simple_voip::ISimpleVoip,
    virtual public simple_voip::ISimpleVoipCallback,
    virtual public threcon::IControllable
{
    friend WorkerBase;

public:
    CallManager();
    ~CallManager();

    bool init( simple_voip::ISimpleVoip * voips, const Config & cfg );

    bool register_callback( simple_voip::ISimpleVoipCallback * callback );

    // interface ISimpleVoip
    void consume( const simple_voip::ForwardObject* obj );

    // interface ISimpleVoipCallback
    void consume( const simple_voip::CallbackObject * obj );

    void start();

    // interface threcon::IControllable
    bool shutdown();

    void map_call_id_to_call( uint32_t call_id, Call * call );

private:

    typedef std::list<std::pair<uint32_t,const simple_voip::InitiateCallRequest*>>  JobQueue;

    typedef std::map<uint32_t, Call*>         MapIdToCall;
    typedef std::map<uint32_t, Call*>         MapJobIdToCall;

private:

    // ServerT interface
    void handle( const simple_voip::IObject* req );

    // simple_voip::ISimpleVoip interface
    void handle( const simple_voip::InitiateCallRequest * req );
    void handle( const simple_voip::DropRequest * req );
    void handle( const simple_voip::PlayFileRequest * req );

    // interface ISimpleVoipCallback
    void handle( const simple_voip::InitiateCallResponse * obj );
    void handle( const simple_voip::RejectResponse * obj );
    void handle( const simple_voip::ErrorResponse * obj );
    void handle( const simple_voip::DropResponse * obj );
    void handle( const simple_voip::Dialing * obj );
    void handle( const simple_voip::Ringing * obj );
    void handle( const simple_voip::Connected * obj );
    void handle( const simple_voip::CallDuration * obj );
    void handle( const simple_voip::ConnectionLost * obj );
    void handle( const simple_voip::Failed * obj );
    void handle( const simple_voip::PlayFileResponse * obj );
    void handle( const simple_voip::DtmfTone * obj );

    template <class _OBJ>
    void forward_request_to_call( const _OBJ * obj );

    template <class _OBJ>
    void forward_response_to_call( const _OBJ * obj );

    template <class OBJ>
    void forward_event_to_call( const OBJ * obj );

    void check_call_end( Call* call, uint32_t call_id, uint32_t job_id );
    void process_jobs();

    void send_error_response( uint32_t job_id, const std::string & descr );

    void callback_consume( const simple_voip::CallbackObject * req );

private:
    mutable std::mutex          mutex_;

    Config                      cfg_;

    uint32_t                    num_active_jobs_;

    JobQueue                    job_queue_;

    simple_voip::ISimpleVoip  * voips_;
    simple_voip::ISimpleVoipCallback        * callback_;

    MapIdToCall                 map_call_id_to_call_;
    MapJobIdToCall              map_job_id_to_call_;
};

NAMESPACE_CALMAN_END

#endif  // CALL_MANAGER_H
