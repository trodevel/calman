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


// $Id: call_manager.h 1241 2014-12-02 19:14:46Z serge $

#ifndef CALL_MANAGER_H
#define CALL_MANAGER_H

#include <boost/thread.hpp>             // boost::mutex
#include <boost/thread/condition.hpp>   // boost::condition

#include "config.h"                         // Config
#include "i_call_manager.h"                 // IJob
#include "../dialer/i_dialer_callback.h"    // IDialerCallback
#include "../threcon/i_controllable.h"      // IControllable

#include "namespace_lib.h"              // NAMESPACE_CALMAN_START

namespace dialer
{
class IDialer;
}

namespace asyncp
{
class AsyncProxy;
}

NAMESPACE_CALMAN_START

class ICallManagerCallback;
class CallManagerImpl;

class CallManager: public virtual ICallManager, public virtual dialer::IDialerCallback, public virtual threcon::IControllable
{
public:
    CallManager();
    ~CallManager();

    bool init( dialer::IDialer * dialer, const Config & cfg );
    void thread_func();

    bool register_callback( ICallManagerCallback * callback );

    // ICallManager interface
    bool insert_job( uint32 job_id, const std::string & party );
    bool remove_job( uint32 job_id );

    // IDialerCallback interface
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
    virtual bool shutdown();

private:
    mutable boost::mutex        mutex_;

    asyncp::AsyncProxy          * proxy_;
    CallManagerImpl             * impl_;
};

NAMESPACE_CALMAN_END

#endif  // CALL_MANAGER_H
