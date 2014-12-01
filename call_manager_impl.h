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


// $Id: call_manager_impl.h 1237 2014-11-28 18:10:22Z serge $

#ifndef CALL_MANAGER_IMPL_H
#define CALL_MANAGER_IMPL_H

#include <list>
#include <boost/thread.hpp>             // boost::mutex
#include <boost/thread/condition.hpp>   // boost::condition

#include "call.h"                           // Call
#include "config.h"                         // Config
#include "../dialer/i_dialer_callback.h"    // IDialerCallback
#include "../threcon/i_controllable.h"      // IControllable

#include "namespace_lib.h"              // NAMESPACE_CALMAN_START

namespace dialer
{
class IDialer;
}

NAMESPACE_CALMAN_START

class Job;
class ICallManagerCallback;

class CallManagerImpl
{
public:
    CallManagerImpl();
    ~CallManagerImpl();

    bool init( dialer::IDialer * dialer, const Config & cfg );

    bool register_callback( ICallManagerCallback * callback );

    // ICallManager interface
    bool insert_job( uint32 job_id, const std::string & party );
    bool remove_job( uint32 job_id );

    void wakeup();

    // interface IDialerCallback
    void on_call_initiate_response( uint32 call_id, uint32 status );
    void on_error_response( uint32 error, const std::string & descr );
    void on_dial( uint32 call_id );
    void on_ring( uint32 call_id );
    void on_call_started( uint32 call_id );
    void on_call_duration( uint32 call_id, uint32 t );
    void on_call_end( uint32 call_id, uint32 errorcode );
    void on_ready();
    void on_error( uint32 call_id, uint32 errorcode );
    void on_fatal_error( uint32 call_id, uint32 errorcode );

    // interface threcon::IControllable
    bool shutdown();

private:
    void process_jobs();
    void process_current_job();

    CallPtr get_call_by_job_id( uint32 id );
    CallPtr get_call_by_call_id( uint32 id );

private:

    typedef std::list<uint32>           JobIdQueue;
    typedef std::map<uint32, CallPtr>   MapIdToCall;

private:
    mutable boost::mutex        mutex_;

    bool                        must_stop_;

    Config                      cfg_;

    ICallManager::state_e       state_;

    JobIdQueue                  job_id_queue_;
    MapIdToCall                 map_job_id_to_call_;
    MapIdToCall                 map_id_to_call_;

    dialer::IDialer             * dialer_;
    ICallManagerCallback        * callback_;

    uint32                      curr_job_id_;

    uint32                      last_id_;

    CallPtr                     curr_call_;
};

NAMESPACE_CALMAN_END

#endif  // CALL_MANAGER_IMPL_H
