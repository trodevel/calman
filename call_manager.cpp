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


// $Id: call_manager.cpp 1044 2014-09-22 17:40:31Z serge $

#include "call_manager.h"                 // self

#include <boost/thread.hpp>             // boost::this_thread

#include "../utils/assert.h"            // ASSERT
#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../dialer/i_dialer.h"         // IDialer

#include "job.h"                        // Job

#define MODULENAME      "CallManager"

NAMESPACE_CALMAN_START

CallManager::CallManager():
    must_stop_( false ), state_( UNDEF ), dialer_( 0L ), curr_job_( 0L ), last_id_( 0 )
{
}
CallManager::~CallManager()
{
    SCOPE_LOCK( mutex_ );


    jobs_.clear();

    if( curr_job_ )
    {
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
    dummy_log_debug( MODULENAME, "thread_func: started" );

    while( true )
    {

        {
            cond_.wait( mutex_cond_ );
        }

        {
            SCOPE_LOCK( mutex_ );

            if( must_stop_ )
                break;

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
        }
    }

    dummy_log_debug( MODULENAME, "thread_func: ended" );
}

void CallManager::process_jobs()
{
    // private: no MUTEX lock needed

    if( jobs_.empty() )
        return;

    ASSERT( curr_job_ == 0L );  // curr_job_ must be empty

    curr_job_ = jobs_.front();

    jobs_.pop_front();

    process_job( curr_job_ );

    curr_job_.reset();
}

bool CallManager::process_job( IJobPtr job )
{
    // private: no MUTEX lock needed

    ASSERT( job );  // job must be empty

    job->on_processing_started();

    std::string party = job->get_property( "party" );

    uint32 status = 0;

    bool b = dialer_->initiate_call( party, status );

    if( b == false)
    {
        dummy_log_error( MODULENAME, "failed to initiate call: job %p, party %s", job.get(), party.c_str() );

        return false;
    }

    boost::shared_ptr< dialer::CallI > call = dialer_->get_call();

    job->on_call_obj_available( call );

    call->register_callback( boost::dynamic_pointer_cast< dialer::ICallCallback, IJob>( job ) );

    return true;
}

void CallManager::wakeup()
{
    // PRIVATE:

    cond_.notify_one();     // wake-up the thread
}

bool CallManager::insert_job( IJobPtr job )
{
    SCOPE_LOCK( mutex_ );

    if( std::find( jobs_.begin(), jobs_.end(), job ) != jobs_.end() )
    {
        dummy_log_error( MODULENAME, "insert_job: job %p already exists", job.get() );

        return false;
    }

    jobs_.push_back( job );

    dummy_log_debug( MODULENAME, "insert_job: inserted job %p", job.get() );

    wakeup();

    return true;
}
bool CallManager::remove_job( IJobPtr job )
{
    SCOPE_LOCK( mutex_ );

    JobList::iterator it = std::find( jobs_.begin(), jobs_.end(), job );
    if( it == jobs_.end() )
    {
        dummy_log_error( MODULENAME, "ERROR: cannot remove job %p - it doesn't exist", job.get() );
        return false;
    }

    dummy_log_debug( MODULENAME, "remove_job: removed job %p", job.get() );

    jobs_.erase( it );

    return true;
}

bool CallManager::shutdown()
{
    SCOPE_LOCK( mutex_ );

    must_stop_  = true;

    wakeup();

    return true;
}

// IDialerCallback interface
void CallManager::on_registered( bool b )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != UNDEF )
    {
        dummy_log_debug( MODULENAME, "on_register: ignored in state %d", state_ );
        return;
    }

    if( b == false )
    {
        dummy_log_debug( MODULENAME, "on_register: ERROR: cannot register" );
        return;
    }

    state_  = IDLE;
}

void CallManager::on_ready()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == IDLE )
    {
        dummy_log_debug( MODULENAME, "on_ready: ignored in state %s", "IDLE" );
        return;
    }

    if( state_ == BUSY )
    {
        dummy_log_debug( MODULENAME, "on_ready: switching into state %s", "IDLE" );
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
        dummy_log_debug( MODULENAME, "on_busy: ignored in state %s", "BUSY" );
        return;
    }

    if( state_ == IDLE )
    {
        dummy_log_debug( MODULENAME, "on_busy: switching into state %s", "BUSY" );
        state_  = BUSY;
    }
}
void CallManager::on_error( uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );
}

NAMESPACE_CALMAN_END
