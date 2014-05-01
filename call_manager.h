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


// $Id: call_manager.h 485 2014-04-30 17:06:58Z serge $

#ifndef CALL_MANAGER_H
#define CALL_MANAGER_H

#include <list>
#include <boost/thread.hpp>         // boost::mutex

#include "i_call_manager.h"                 // IJob
#include "../dialer/i_dialer_callback.h"    // IDialerCallback

#include "namespace_calman.h"       // NAMESPACE_CALMAN_START

namespace dialer
{
class IDialer;
}

NAMESPACE_CALMAN_START

class Job;

class CallManager: public virtual ICallManager, public virtual dialer::IDialerCallback
{
public:
    struct Config
    {
        uint32  sleep_time_ms;  // try 3 ms
    };

public:
    CallManager();
    ~CallManager();

    bool init( dialer::IDialer * dialer, const Config & cfg );
    void thread_func();

    // ICallManager interface
    bool insert_job( IJob * job );
    bool remove_job( IJob * job );
    bool shutdown();

    // IDialerCallback interface
    void on_ready();
    void on_busy();
    void on_error( uint32 errorcode );

private:
    void process_jobs();
    bool process_job( IJob* job );

private:

    typedef std::list<IJob*>    JobList;

private:
    mutable boost::mutex        mutex_;

    Config                      cfg_;

    state_e                     state_;

    JobList                     jobs_;

    dialer::IDialer             * dialer_;

    IJob                        * curr_job_;

    uint32                      last_id_;
};

NAMESPACE_CALMAN_END

#endif  // CALL_MANAGER_H
