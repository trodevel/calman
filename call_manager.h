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


// $Revision: 3315 $ $Date:: 2016-01-29 #$ $Author: serge $

#ifndef CALL_MANAGER_H
#define CALL_MANAGER_H

#include <list>
#include <mutex>                            // std::mutex

#include "call.h"                           // Call
#include "config.h"                         // Config
#include "i_call_manager.h"                 // ICallManager
#include "objects.h"                        // InitiateCallRequest, ...
#include "../voip_io/i_voip_service_callback.h"     // IVoipServiceCallback
#include "../threcon/i_controllable.h"      // IControllable
#include "../jobman/job_man_t.h"            // JobManT
#include "../workt/worker_t.h"              // WorkerT

#include "namespace_lib.h"              // NAMESPACE_CALMAN_START

namespace voip_service
{
class IVoipService;
}

NAMESPACE_CALMAN_START

class ICallManagerCallback;

class CallManager;

typedef workt::WorkerT< const workt::IObject*, CallManager> WorkerBase;

class CallManager: public WorkerBase,
    virtual public ICallManager,
    virtual public voip_service::IVoipServiceCallback,
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

    bool init( voip_service::IVoipService * voips, const Config & cfg );

    bool register_callback( ICallManagerCallback * callback );

    // ICallManager interface
    void consume( const Object* obj );

    // interface IVoipServiceCallback
    void consume( const voip_service::CallbackObject * obj );

    // interface threcon::IControllable
    bool shutdown();


private:

    typedef std::list<CallPtr>  JobQueue;

private:

    // ServerT interface
    void handle( const workt::IObject* req );

    // ICallManager interface
    void handle( const InitiateCallRequest * req );
    void handle( const DropRequest * req );
    void handle( const PlayFileRequest * req );

    // interface IVoipServiceCallback
    void handle( const voip_service::InitiateCallResponse * obj );
    void handle( const voip_service::RejectResponse * obj );
    void handle( const voip_service::ErrorResponse * obj );
    void handle( const voip_service::DropResponse * obj );
    void handle( const voip_service::Dial * obj );
    void handle( const voip_service::Ring * obj );
    void handle( const voip_service::Connected * obj );
    void handle( const voip_service::CallDuration * obj );
    void handle( const voip_service::ConnectionLost * obj );
    void handle( const voip_service::Failed * obj );
    void handle( const voip_service::PlayFileResponse * obj );
    void handle( const voip_service::DtmfTone * obj );

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

    voip_service::IVoipService  * voips_;
    ICallManagerCallback        * callback_;

    CallPtr                     curr_job_;
};

NAMESPACE_CALMAN_END

#endif  // CALL_MANAGER_H
