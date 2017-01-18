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


// $Revision: 5572 $ $Date:: 2017-01-17 #$ $Author: serge $

#ifndef CALL_MANAGER_H
#define CALL_MANAGER_H

#include <list>
#include <mutex>                            // std::mutex

#include "call.h"                           // Call
#include "config.h"                         // Config
#include "i_call_manager.h"                 // ICallManager
#include "objects.h"                        // InitiateCallRequest, ...
#include "../simple_voip/i_simple_voip_callback.h"     // ISimpleVoipCallback
#include "../threcon/i_controllable.h"      // IControllable
#include "../workt/worker_t.h"              // WorkerT

#include "namespace_lib.h"              // NAMESPACE_CALMAN_START

namespace simple_voip
{
class ISimpleVoip;
}

NAMESPACE_CALMAN_START

class SimpleVoipWrap;

class ICallManagerCallback;

class CallManager;

typedef workt::WorkerT< const workt::IObject*, CallManager> WorkerBase;

class CallManager: public WorkerBase,
    virtual public ICallManager,
    virtual public simple_voip::ISimpleVoipCallback,
    virtual public threcon::IControllable
{
    friend WorkerBase;

public:
    enum state_e
    {
        IDLE    = 0,
        BUSY
    };

public:
    CallManager();
    ~CallManager();

    bool init( simple_voip::ISimpleVoip * voips, const Config & cfg );

    bool register_callback( ICallManagerCallback * callback );

    // ICallManager interface
    void consume( const Object* obj );

    // interface ISimpleVoipCallback
    void consume( const simple_voip::CallbackObject * obj );

    void start();

    // interface threcon::IControllable
    bool shutdown();


private:

    typedef std::list<CallPtr>  JobQueue;

private:

    // ServerT interface
    void handle( const workt::IObject* req );

    void handle( const SimpleVoipWrap * req );

    // ICallManager interface
    void handle( const InitiateCallRequest * req );
    void handle( const DropRequest * req );
    void handle( const PlayFileRequest * req );

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
    void forward_to_call( const _OBJ * obj );

    void check_call_end();
    void process_jobs();

    void trace_state_switch() const;

    void send_error_response( uint32_t job_id, const std::string & descr );
    void send_reject_response( uint32_t job_id, const std::string & descr );

    void callback_consume( const CallbackObject * req );

    JobQueue::iterator find( uint32_t job_id );

private:
    mutable std::mutex          mutex_;

    Config                      cfg_;

    state_e                     state_;

    JobQueue                    job_queue_;

    simple_voip::ISimpleVoip  * voips_;
    ICallManagerCallback        * callback_;

    CallPtr                     curr_job_;
};

NAMESPACE_CALMAN_END

#endif  // CALL_MANAGER_H
