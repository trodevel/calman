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


// $Id: call_manager_impl.cpp 1236 2014-11-26 19:15:35Z serge $

#include "call_manager_impl.h"          // self

#include <boost/thread.hpp>             // boost::this_thread

#include "../utils/assert.h"            // ASSERT
#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../dialer/i_dialer.h"         // IDialer

#include "job.h"                        // Job

#define MODULENAME      "CallManager"

NAMESPACE_CALMAN_START

const char* to_cstr( ICallManager::state_e s )
{
    static const char *vals[]=
    {
            "UNDEF", "IDLE", "WAITING_DIALER", "BUSY"
    };

    if( s < ICallManager::UNDEF || s > ICallManager::BUSY )
        return vals[0];

    return vals[ (int) s ];
}

CallManagerImpl::CallManagerImpl():
    must_stop_( false ), state_( ICallManager::UNDEF ), dialer_( 0L ), curr_job_( 0L ), last_id_( 0 )
{
}
CallManagerImpl::~CallManagerImpl()
{
    SCOPE_LOCK( mutex_ );


    job_queue_.clear();

    if( curr_job_ )
    {
        curr_job_   = 0L;
    }
}

bool CallManagerImpl::init( dialer::IDialer * dialer, const Config & cfg )
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

void CallManagerImpl::wakeup()
{
    dummy_log_trace( MODULENAME, "wakeup" );

    SCOPE_LOCK( mutex_ );

    if( must_stop_ )
        return;

    switch( state_ )
    {
    case ICallManager::IDLE:
        process_jobs();
        break;

    case ICallManager::BUSY:
    case ICallManager::WAITING_DIALER:
        break;

    default:
        break;
    }

}

void CallManagerImpl::process_jobs()
{
    // private: no MUTEX lock needed

    if( job_queue_.empty() )
        return;

    ASSERT( curr_job_ == 0L );  // curr_job_ must be empty

    curr_job_ = job_queue_.front();

    job_queue_.pop_front();

    process_current_job();
}

void CallManagerImpl::process_current_job()
{
    // private: no MUTEX lock needed

    ASSERT( state_ == ICallManager::IDLE ); // just paranoid check

    ASSERT( curr_job_ );  // job must not be empty

    curr_job_->on_processing_started();

    std::string party = curr_job_->get_property( "party" );

    dialer_->initiate_call( party );

    state_  = ICallManager::WAITING_DIALER;
}

bool CallManagerImpl::insert_job( uint32 job_id, const std::string & party )
{
    SCOPE_LOCK( mutex_ );

    if( map_job_id_to_call_.count( job_id ) > 0 )
    {
        dummy_log_error( MODULENAME, "insert_job: job %u already exists", job_id );

        return false;
    }

    CallPtr call    = new Call( job_id, party, nullptr );

    job_queue_.push_back( job_id );

    ASSERT( map_job_id_to_call_.insert( MapIdToCall::value_type( job_id, call ) ).second );

    dummy_log_debug( MODULENAME, "insert_job: inserted job %u", job_id );

    return true;
}
bool CallManagerImpl::remove_job( uint32 job_id )
{
    SCOPE_LOCK( mutex_ );

    uint32 call_id  = 0;

    {
        MapIdToCall::iterator it = map_job_id_to_call_.find( job_id );
        if( it == map_job_id_to_call_.end() )
        {
            dummy_log_fatal( MODULENAME, "cannot remove job %u - it doesn't exist", job_id );
            ASSERT( 0 );
            return false;
        }

        dummy_log_debug( MODULENAME, "removed job %u from map", job_id );

        CallPtr call = (*it).second;

        call_id  = call->get_call_id();
    }

    {
        JobQueue::iterator it = std::find( job_queue_.begin(), job_queue_.end(), job_id );
        if( it != job_queue_.end() )
        {
            dummy_log_debug( MODULENAME, "removed job %u from pending queue", job_id );

            job_queue_.erase( it );
        }
    }

    if( call_id != 0 )
    {
        MapIdToCall::iterator it = map_id_to_call_.find( call_id );
        if( it == map_id_to_call_.end() )
        {
            dummy_log_fatal( MODULENAME, "cannot remove call %u for job %u - it doesn't exist", call_id, job_id );
            ASSERT( 0 );
            return false;
        }

        dummy_log_debug( MODULENAME, "removed call %u for job %u from map", call_id, job_id );
    }

    return true;
}

bool CallManagerImpl::shutdown()
{
    SCOPE_LOCK( mutex_ );

    must_stop_  = true;

    return true;
}

// IDialerCallback interface
void CallManagerImpl::on_registered( bool b )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ICallManager::UNDEF )
    {
        dummy_log_debug( MODULENAME, "on_register: ignored in state %d", to_cstr( state_ ) );
        return;
    }

    if( b == false )
    {
        dummy_log_debug( MODULENAME, "on_register: ERROR: cannot register" );
        return;
    }

    state_  = ICallManager::IDLE;
}

void CallManagerImpl::on_call_initiate_response( bool is_initiated, uint32 status, CallPtr call )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ICallManager::WAITING_DIALER )
    {
        dummy_log_warn( MODULENAME, "on_call_initiate_response: ignored in state %s", to_cstr( state_ ) );
        return;
    }

    ASSERT( curr_job_ );    // curr job must not be empty

    if( is_initiated == false )
    {
        dummy_log_error( MODULENAME, "failed to initiate call: job %p, party %s", curr_job_.get(), curr_job_->get_property( "party" ).c_str() );

        state_  = ICallManager::IDLE;

        curr_job_.reset();  // as call failed, curr job can be deleted

        return;
    }

    curr_call_  = call;

    state_  = ICallManager::BUSY;
}

void CallManagerImpl::on_call_started()
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ICallManager::BUSY )
    {
        dummy_log_debug( MODULENAME, "on_call_started: ignored in state %s", to_cstr( state_ ) );
        return;
    }

    if( state_ == ICallManager::BUSY )
    {
        dummy_log_debug( MODULENAME, "on_call_started: call started" );

        ASSERT( curr_job_ );    // curr job must not be empty

        ASSERT( curr_call_ );   // curr call must not be empty

        curr_job_->on_call_started( curr_call_ );
    }
}

void CallManagerImpl::on_ready()
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ICallManager::BUSY )
    {
        dummy_log_debug( MODULENAME, "on_ready: ignored in state %s", to_cstr( state_ ) );
        return;
    }

    if( state_ == ICallManager::BUSY )
    {
        dummy_log_debug( MODULENAME, "on_ready: switching into state %s", "IDLE" );
        state_  = ICallManager::IDLE;

        ASSERT( curr_job_ );    // curr job must not be empty

        curr_job_->on_finished();

        curr_job_.reset();      // as call finished, curr job can be deleted

        curr_call_.reset();     // clear current call
    }

    if( job_queue_.empty() )
        return; // no jobs to process, exit
}
void CallManagerImpl::on_error( uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ICallManager::BUSY )
    {
        dummy_log_debug( MODULENAME, "on_error: ignored in state %s", to_cstr( state_ ) );

        ASSERT( curr_job_ == nullptr );    // curr job must be empty

        ASSERT( curr_call_ == nullptr );   // curr call must be empty

        return;
    }

    if( state_ == ICallManager::BUSY )
    {
        dummy_log_debug( MODULENAME, "on_error: call failed, switching into state %s", "IDLE" );

        ASSERT( curr_job_ );    // curr job must not be empty

        curr_job_->on_error( errorcode );

        curr_job_.reset();      // as call finished, curr job can be deleted

        state_  = ICallManager::IDLE;
    }
}

NAMESPACE_CALMAN_END
