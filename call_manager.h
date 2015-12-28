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


// $Revision: 3071 $ $Date:: 2015-12-28 #$ $Author: serge $

#ifndef CALL_MANAGER_H
#define CALL_MANAGER_H

#include <list>
#include <mutex>                            // std::mutex

#include "call.h"                           // Call
#include "config.h"                         // Config
#include "i_call_manager.h"                 // ICallManager
#include "objects.h"                        // CalmanInsertJob, ...
#include "../voip_io/i_voip_service_callback.h"     // IVoipServiceCallback
#include "../threcon/i_controllable.h"      // IControllable
#include "../jobman/job_man_t.h"            // JobManT
#include "../servt/server_t.h"              // ServerT

#include "namespace_lib.h"              // NAMESPACE_CALMAN_START

namespace voip_service
{
class IVoipService;
}

NAMESPACE_CALMAN_START

class ICallManagerCallback;

class CallManager;

typedef servt::ServerT< const servt::IObject*, CallManager> ServerBase;

class CallManager: public ServerBase,
    virtual public ICallManager,
    virtual public voip_service::IVoipServiceCallback,
    virtual public threcon::IControllable
{
    friend ServerBase;

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
    void handle( const servt::IObject* req );

    // ICallManager interface
    void handle( const InitiateCall * req );
    void handle( const CancelCall * req );
    void handle( const PlayFileRequest * req );
    void handle( const DropRequest * req );

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

    template <class _OBJ>
    void forward_to_call( const _OBJ * obj );

    void check_call_end();
    void process_jobs();
    bool remove_job__( uint32_t job_id );

    void trace_state_switch() const;

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
