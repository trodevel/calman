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


// $Id: call_manager.h 1299 2014-12-30 19:33:19Z serge $

#ifndef CALL_MANAGER_H
#define CALL_MANAGER_H

#include <list>
#include <boost/thread.hpp>             // boost::mutex
#include <boost/thread/condition.hpp>   // boost::condition

#include "call.h"                           // Call
#include "config.h"                         // Config
#include "i_call_manager.h"                 // ICallManager
#include "objects.h"                        // CalmanInsertJob, ...
#include "../dialer/i_dialer_callback.h"    // IDialerCallback
#include "../threcon/i_controllable.h"      // IControllable
#include "../jobman/job_man_t.h"            // JobManT
#include "../servt/server_t.h"              // ServerT

#include "namespace_lib.h"              // NAMESPACE_CALMAN_START

namespace dialer
{
class IDialer;
}

NAMESPACE_CALMAN_START

class ICallManagerCallback;

class CallManager;

typedef servt::ServerT< const servt::IObject*, CallManager> ServerBase;

class CallManager: public ServerBase,
    virtual public ICallManager,
    virtual public dialer::IDialerCallback,
    virtual public threcon::IControllable
{
    friend ServerBase;

public:
    enum state_e
    {
        UNDEF   = 0,
        IDLE,
        WAITING_DIALER_RESP,
        WAITING_DIALER_FREE,
        BUSY
    };

public:
    CallManager();
    ~CallManager();

    bool init( dialer::IDialer * dialer, const Config & cfg );

    bool register_callback( ICallManagerCallback * callback );

    // ICallManager interface
    void consume( const CalmanObject* obj );

    // interface IDialerCallback
    void consume( const dialer::DialerCallbackObject * obj );

    void wakeup();

    // interface threcon::IControllable
    bool shutdown();

private:

    // ICallManager interface
    void handle( const servt::IObject* req );

    bool handle( const CalmanInsertJob * req );
    bool handle( const CalmanRemoveJob * req );
    void handle( const CalmanPlayFile * req );
    void handle( const CalmanDrop * req );

    // interface IDialerCallback
    void handle( const dialer::DialerInitiateCallResponse * obj );
    void handle( const dialer::DialerErrorResponse * obj );
    void handle( const dialer::DialerDial * obj );
    void handle( const dialer::DialerRing * obj );
    void handle( const dialer::DialerConnect * obj );
    void handle( const dialer::DialerCallDuration * obj );
    void handle( const dialer::DialerCallEnd * obj );
    void handle( const dialer::DialerDropResponse * obj );
    void handle( const dialer::DialerError * obj );
    void handle( const dialer::DialerFatalError * obj );

    template <class _OBJ>
    void forward_to_call( const _OBJ * obj );

private:
    void process_jobs();
    void process_current_job();
    bool remove_job__( uint32 job_id );

private:

    typedef std::list<uint32>           JobIdQueue;

private:
    mutable boost::mutex        mutex_;

    bool                        must_stop_;

    Config                      cfg_;

    state_e                     state_;

    JobIdQueue                  job_id_queue_;

    jobman::JobManT<CallPtr>    jobman_;

    dialer::IDialer             * dialer_;
    ICallManagerCallback        * callback_;

    uint32                      curr_job_id_;
};

NAMESPACE_CALMAN_END

#endif  // CALL_MANAGER_H
