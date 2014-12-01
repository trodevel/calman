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


// $Id: call_manager_impl.cpp 1237 2014-11-28 18:10:22Z serge $

#include "call_manager_impl.h"          // self

#include <boost/thread.hpp>             // boost::this_thread

#include "../utils/assert.h"            // ASSERT
#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../dialer/i_dialer.h"         // IDialer

#include "i_call_manager_callback.h"    // ICallManagerCallback
#include "job.h"                        // Job

#define MODULENAME      "CallManager"

NAMESPACE_CALMAN_START

const char* to_c_str( ICallManager::state_e s )
{
    static const char *vals[]=
    {
            "UNDEF", "IDLE", "WAITING_DIALER_RESP", "WAITING_DIALER_FREE", "BUSY"
    };

    if( s < ICallManager::UNDEF || s > ICallManager::BUSY )
        return vals[0];

    return vals[ (int) s ];
}

CallManagerImpl::CallManagerImpl():
    must_stop_( false ), state_( ICallManager::UNDEF ), dialer_( 0L ), callback_( nullptr ), curr_job_id_( 0L ), last_id_( 0 )
{
}
CallManagerImpl::~CallManagerImpl()
{
    SCOPE_LOCK( mutex_ );

    job_id_queue_.clear();

    if( curr_job_id_ )
    {
        curr_job_id_   = 0L;
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

bool CallManagerImpl::register_callback( ICallManagerCallback * callback )
{
    if( callback == 0L )
        return false;

    SCOPE_LOCK( mutex_ );

    if( callback_ != 0L )
        return false;

    callback_ = callback;

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
    case ICallManager::WAITING_DIALER_RESP:
        break;

    default:
        break;
    }

}

void CallManagerImpl::process_jobs()
{
    // private: no MUTEX lock needed

    if( job_id_queue_.empty() )
        return;

    ASSERT( curr_job_id_ == 0 );  // curr_job_id_ must be empty

    curr_job_id_ = job_id_queue_.front();

    job_id_queue_.pop_front();

    process_current_job();
}

void CallManagerImpl::process_current_job()
{
    // private: no MUTEX lock needed

    ASSERT( state_ == ICallManager::IDLE ); // just paranoid check

    ASSERT( curr_job_id_ );  // job must not be empty

    if( callback_ )
        callback_->on_processing_started( curr_job_id_ );

    const std::string & party = get_call_by_job_id( curr_job_id_ )->get_party();

    dialer_->initiate_call( party );

    state_  = ICallManager::WAITING_DIALER_RESP;
}

CallPtr CallManagerImpl::get_call_by_job_id( uint32 id )
{
    MapIdToCall::iterator it = map_job_id_to_call_.find( id );

    ASSERT( it != map_job_id_to_call_.end() );

    return (*it).second;
}

CallPtr CallManagerImpl::get_call_by_call_id( uint32 id )
{
    MapIdToCall::iterator it = map_id_to_call_.find( id );

    ASSERT( it != map_id_to_call_.end() );

    return (*it).second;
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

    job_id_queue_.push_back( job_id );

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
        JobIdQueue::iterator it = std::find( job_id_queue_.begin(), job_id_queue_.end(), job_id );
        if( it != job_id_queue_.end() )
        {
            dummy_log_debug( MODULENAME, "removed job %u from pending queue", job_id );

            job_id_queue_.erase( it );
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
void CallManagerImpl::on_call_initiate_response( uint32 call_id, uint32 status )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ICallManager::WAITING_DIALER_RESP )
    {
        dummy_log_fatal( MODULENAME, "on_call_initiate_response: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    ASSERT( curr_job_id_ );    // curr job must not be empty

    curr_call_  = call;

    state_  = ICallManager::BUSY;
}

void CallManagerImpl::on_error_response( uint32 error, const std::string & descr )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ICallManager::WAITING_DIALER_RESP )
    {
        dummy_log_fatal( MODULENAME, "on_error_response: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    ASSERT( curr_job_id_ );    // curr job must not be empty

    dummy_log_error( MODULENAME, "on_error_response: dialer is busy, error %u, %s", error, descr.c_str() );

    state_  = ICallManager::WAITING_DIALER_FREE;
}


void CallManagerImpl::on_call_started()
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ICallManager::BUSY )
    {
        dummy_log_debug( MODULENAME, "on_call_started: ignored in state %s", to_c_str( state_ ) );
        return;
    }

    if( state_ == ICallManager::BUSY )
    {
        dummy_log_debug( MODULENAME, "on_call_started: call started" );

        ASSERT( curr_job_id_ );    // curr job must not be empty

        ASSERT( curr_call_ );   // curr call must not be empty

        curr_job_id_->on_call_started( curr_call_ );
    }
}

void CallManagerImpl::on_ready()
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ICallManager::BUSY )
    {
        dummy_log_debug( MODULENAME, "on_ready: ignored in state %s", to_c_str( state_ ) );
        return;
    }

    if( state_ == ICallManager::BUSY )
    {
        dummy_log_debug( MODULENAME, "on_ready: switching into state %s", "IDLE" );
        state_  = ICallManager::IDLE;

        ASSERT( curr_job_id_ );    // curr job must not be empty

        curr_job_id_->on_finished();

        curr_job_id_.reset();      // as call finished, curr job can be deleted

        curr_call_.reset();     // clear current call
    }

    if( job_id_queue_.empty() )
        return; // no jobs to process, exit
}
void CallManagerImpl::on_error( uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ICallManager::BUSY )
    {
        dummy_log_debug( MODULENAME, "on_error: ignored in state %s", to_c_str( state_ ) );

        ASSERT( curr_job_id_ == nullptr );    // curr job must be empty

        ASSERT( curr_call_ == nullptr );   // curr call must be empty

        return;
    }

    if( state_ == ICallManager::BUSY )
    {
        dummy_log_debug( MODULENAME, "on_error: call failed, switching into state %s", "IDLE" );

        ASSERT( curr_job_id_ );    // curr job must not be empty

        curr_job_id_->on_error( errorcode );

        curr_job_id_.reset();      // as call finished, curr job can be deleted

        state_  = ICallManager::IDLE;
    }
}

NAMESPACE_CALMAN_END
