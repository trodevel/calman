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


// $Id: call_manager_impl.cpp 1242 2014-12-02 19:16:35Z serge $

#include "call_manager_impl.h"          // self

#include <boost/thread.hpp>             // boost::this_thread

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../dialer/i_dialer.h"         // IDialer
#include "../utils/assert.h"            // ASSERT

#include "i_call_manager_callback.h"    // ICallManagerCallback

#define MODULENAME      "CallManager"

NAMESPACE_CALMAN_START

const char* to_c_str( CallManagerImpl::state_e s )
{
    static const char *vals[]=
    {
            "UNDEF", "IDLE", "WAITING_DIALER_RESP", "WAITING_DIALER_FREE", "BUSY"
    };

    if( s < CallManagerImpl::UNDEF || s > CallManagerImpl::BUSY )
        return vals[0];

    return vals[ (int) s ];
}

CallManagerImpl::CallManagerImpl():
    must_stop_( false ), state_( UNDEF ), dialer_( 0L ), callback_( nullptr ), curr_job_id_( 0L )
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
    case IDLE:
        process_jobs();
        break;

    case BUSY:
    case WAITING_DIALER_RESP:
    case WAITING_DIALER_FREE:
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

    ASSERT( state_ == IDLE ); // just paranoid check

    ASSERT( curr_job_id_ );  // job must not be empty

    if( callback_ )
        callback_->on_processing_started( curr_job_id_ );

    const std::string & party = get_call_by_job_id( curr_job_id_ )->get_party();

    dialer_->initiate_call( party );

    state_  = WAITING_DIALER_RESP;
}

CallPtr CallManagerImpl::get_call_by_job_id( uint32 id )
{
    MapIdToCall::iterator it = map_job_id_to_call_.find( id );

    ASSERT( it != map_job_id_to_call_.end() );

    return (*it).second;
}

CallPtr CallManagerImpl::get_call_by_call_id( uint32 id )
{
    uint32 job_id   = get_job_id_by_call_id( id );

    return get_call_by_job_id( job_id );
}

uint32 CallManagerImpl::get_call_id_by_job_id( uint32 id )
{
    return get_call_by_job_id( id )->get_call_id();
}

uint32 CallManagerImpl::get_job_id_by_call_id( uint32 id )
{
    MapIdToId::iterator it = map_call_id_to_job_id_.find( id );

    ASSERT( it != map_call_id_to_job_id_.end() );

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

    CallPtr call( new Call( job_id, party, callback_ ) );

    job_id_queue_.push_back( job_id );

    ASSERT( map_job_id_to_call_.insert( MapIdToCall::value_type( job_id, call ) ).second );

    dummy_log_debug( MODULENAME, "insert_job: inserted job %u", job_id );

    return true;
}
bool CallManagerImpl::remove_job( uint32 job_id )
{
    SCOPE_LOCK( mutex_ );

    return remove_job__( job_id );
}
bool CallManagerImpl::remove_job__( uint32 job_id )
{
    // private: no mutex lock

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
        MapIdToId::iterator it = map_call_id_to_job_id_.find( call_id );
        if( it == map_call_id_to_job_id_.end() )
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

    if( state_ != WAITING_DIALER_RESP )
    {
        dummy_log_fatal( MODULENAME, "on_call_initiate_response: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    ASSERT( curr_job_id_ );    // curr job must not be empty

    ASSERT( map_call_id_to_job_id_.insert( MapIdToId::value_type( call_id, curr_job_id_) ).second );

    state_  = BUSY;
}

void CallManagerImpl::on_error_response( uint32 error, const std::string & descr )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_DIALER_RESP )
    {
        dummy_log_fatal( MODULENAME, "on_error_response: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    ASSERT( curr_job_id_ );    // curr job must not be empty

    dummy_log_error( MODULENAME, "on_error_response: dialer is busy, error %u, %s", error, descr.c_str() );

    state_  = WAITING_DIALER_FREE;
}

void CallManagerImpl::on_dial( uint32 call_id )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );     // curr job must not be empty
    ASSERT( curr_job_id_ == get_job_id_by_call_id( call_id ) );

    get_call_by_call_id( call_id )->on_dial();
}
void CallManagerImpl::on_ring( uint32 call_id )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );     // curr job must not be empty
    ASSERT( curr_job_id_ == get_job_id_by_call_id( call_id ) );

    get_call_by_call_id( call_id )->on_ring();
}

void CallManagerImpl::on_call_started( uint32 call_id )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );     // curr job must not be empty
    ASSERT( curr_job_id_ == get_job_id_by_call_id( call_id ) );

    get_call_by_call_id( call_id )->on_call_started();
}

void CallManagerImpl::on_call_duration( uint32 call_id, uint32 t )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );     // curr job must not be empty
    ASSERT( curr_job_id_ == get_job_id_by_call_id( call_id ) );

    get_call_by_call_id( call_id )->on_call_duration( t );
}

void CallManagerImpl::on_call_end( uint32 call_id, uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );    // curr job must not be empty
    ASSERT( curr_job_id_ == get_job_id_by_call_id( call_id ) );

    get_call_by_call_id( call_id )->on_call_end( errorcode );
}

void CallManagerImpl::on_ready()
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY || state_ == WAITING_DIALER_FREE );

    switch( state_ )
    {

    case BUSY:
    {
        dummy_log_debug( MODULENAME, "on_ready: switching into state %s", "IDLE" );
        state_  = IDLE;

        ASSERT( curr_job_id_ );    // curr job must not be empty

        remove_job__( curr_job_id_ );

        curr_job_id_    = 0;      // as call finished, curr job can be deleted

        // TODO add queue check, eb02
    }
    break;

    case WAITING_DIALER_FREE:
    {
        dummy_log_debug( MODULENAME, "on_ready: switching into state %s", "WAITING_DIALER_RESP" );

        ASSERT( curr_job_id_ );    // curr job must not be empty

        const std::string & party = get_call_by_job_id( curr_job_id_ )->get_party();

        dialer_->initiate_call( party );

        state_  = WAITING_DIALER_RESP;
    }
    break;

    default:
        break;
    }
}
void CallManagerImpl::on_error( uint32 call_id, uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );    // curr job must not be empty
    ASSERT( curr_job_id_ == get_job_id_by_call_id( call_id ) );

    get_call_by_call_id( call_id )->on_error( errorcode );
}

void CallManagerImpl::on_fatal_error( uint32 call_id, uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );    // curr job must not be empty
    ASSERT( curr_job_id_ == get_job_id_by_call_id( call_id ) );

    get_call_by_call_id( call_id )->on_fatal_error( errorcode );
}

NAMESPACE_CALMAN_END
