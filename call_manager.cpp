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


// $Id: call_manager.cpp 457 2014-04-28 16:34:59Z serge $

#include "call_manager.h"                 // self

#include <boost/thread.hpp>             // boost::this_thread

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../dialer/i_dialer.h"         // IDialer

#include "job.h"                        // Job

#define MODULENAME      "CallManager"

NAMESPACE_CALMAN_START

CallManager::CallManager():
    state_( UNDEF ), dialer_( 0L ), curr_job_( 0L ), last_id_( 0 )
{
}
CallManager::~CallManager()
{
    SCOPE_LOCK( mutex_ );

    for( auto s : jobs_ )
    {
        delete s;
    }

    jobs_.clear();

    if( curr_job_ )
    {
        delete curr_job_;
        curr_job_   = 0L;
    }
}

bool CallManager::init( dialer::IDialer * dialer, const Config & cfg )
{
    if( dialer == 0L )
        return false;

    SCOPE_LOCK( mutex_ );

    if( dialer_ != 0L )
        return false;

    dialer_ = dialer;
    cfg_    = cfg;

    return true;
}

void CallManager::thread_func()
{
    dummy_log( 0, MODULENAME, "thread_func: started" );

    bool should_run    = true;
    while( should_run )
    {
        SCOPE_LOCK( mutex_ );

        switch( state_ )
        {
        case IDLE:
            process_jobs();
            break;

        case BUSY:
            break;

        default:
            break;
        }

        THREAD_SLEEP_MS( cfg_.sleep_time_ms );
    }

    dummy_log( 0, MODULENAME, "thread_func: ended" );
}

void CallManager::process_jobs()
{
    if( jobs_.empty() )
        return;

    Job * job = jobs_.pop_front();
}

// ICallManager interface
calman::IJob* CallManager::create_call_job( const std::string & party )
{
    SCOPE_LOCK( mutex_ );

    last_id_++;

    Job * job   = new Job( last_id_, this );

    jobs_.push_back( job );

    return job;
}
bool CallManager::cancel_job( uint32 id )
{
    SCOPE_LOCK( mutex_ );

    return false;
}
bool CallManager::cancel_job( const IJob * job )
{
    SCOPE_LOCK( mutex_ );

    return false;
}
calman::IJob* CallManager::get_job( uint32 id )
{
    SCOPE_LOCK( mutex_ );

    return 0L;  // TODO implement it e425
}
bool CallManager::shutdown()
{
    SCOPE_LOCK( mutex_ );

    return true;
}

// IDialerCallback interface
void CallManager::on_ready()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == IDLE )
    {
        dummy_log( 0, MODULENAME, "on_ready: ignored in state %s", "IDLE" );
        return;
    }

    if( state_ == BUSY )
    {
        dummy_log( 0, MODULENAME, "on_ready: switching into state %s", "IDLE" );
        state_  = IDLE;
    }

    if( jobs_.empty() )
        return; // no jobs to process, exit
}
void CallManager::on_busy()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == BUSY )
    {
        dummy_log( 0, MODULENAME, "on_busy: ignored in state %s", "BUSY" );
        return;
    }

    if( state_ == IDLE )
    {
        dummy_log( 0, MODULENAME, "on_busy: switching into state %s", "BUSY" );
        state_  = BUSY;
    }
}
void CallManager::on_error( uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );
}

NAMESPACE_CALMAN_END
