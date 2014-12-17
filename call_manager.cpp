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


// $Id: call_manager.cpp 1270 2014-12-16 19:41:37Z serge $

#include "call_manager.h"               // self

#include <boost/thread.hpp>             // boost::this_thread

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../dialer/i_dialer.h"         // IDialer
#include "../utils/assert.h"            // ASSERT

#include "object_factory.h"             // create_message_t
#include "i_call_manager_callback.h"    // ICallManagerCallback

#define MODULENAME      "CallManager"

NAMESPACE_CALMAN_START

const char* to_c_str( CallManager::state_e s )
{
    static const char *vals[]=
    {
            "UNDEF", "IDLE", "WAITING_DIALER_RESP", "WAITING_DIALER_FREE", "BUSY"
    };

    if( s < CallManager::UNDEF || s > CallManager::BUSY )
        return vals[0];

    return vals[ (int) s ];
}

CallManager::CallManager():
    ServerBase( this ),
    must_stop_( false ), state_( UNDEF ), dialer_( 0L ), callback_( nullptr ), curr_job_id_( 0L )
{
}
CallManager::~CallManager()
{
    SCOPE_LOCK( mutex_ );

    job_id_queue_.clear();

    if( curr_job_id_ )
    {
        curr_job_id_   = 0L;
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

bool CallManager::register_callback( ICallManagerCallback * callback )
{
    if( callback == 0L )
        return false;

    SCOPE_LOCK( mutex_ );

    if( callback_ != 0L )
        return false;

    callback_ = callback;

    return true;
}

bool CallManager::consume( const CalmanObject* obj )
{
    return ServerBase::consume( obj );
}

void CallManager::handle( const servt::IObject* req )
{
    SCOPE_LOCK( mutex_ );

    if( typeid( *req ) == typeid( CalmanInsertJob ) )
    {
        handle( dynamic_cast< const CalmanInsertJob *>( req ) );
    }
    else if( typeid( *req ) == typeid( CalmanRemoveJob ) )
    {
        handle( dynamic_cast< const CalmanRemoveJob *>( req ) );
    }
    else if( typeid( *req ) == typeid( CalmanPlayFile ) )
    {
        handle( dynamic_cast< const CalmanPlayFile *>( req ) );
    }
    else if( typeid( *req ) == typeid( CalmanDrop ) )
    {
        handle( dynamic_cast< const CalmanDrop *>( req ) );
    }
    else
    {
        dummy_log_fatal( MODULENAME, "handle: cannot cast request to known type - %p", (void *) req );

        ASSERT( 0 );
    }

    delete req;
}

void CallManager::wakeup()
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

void CallManager::process_jobs()
{
    // private: no MUTEX lock needed

    if( job_id_queue_.empty() )
        return;

    ASSERT( curr_job_id_ == 0 );  // curr_job_id_ must be empty

    curr_job_id_ = job_id_queue_.front();

    job_id_queue_.pop_front();

    process_current_job();
}

void CallManager::process_current_job()
{
    // private: no MUTEX lock needed

    ASSERT( state_ == IDLE ); // just paranoid check

    ASSERT( curr_job_id_ );  // job must not be empty

    if( callback_ )
        callback_->consume( create_message_t<CalmanProcessingStarted>( curr_job_id_ ) );

    const std::string & party = jobman_.get_job_by_parent_job_id( curr_job_id_ )->get_party();

    dialer_->initiate_call( party );

    state_  = WAITING_DIALER_RESP;
}

bool CallManager::handle( const CalmanInsertJob * req )
{
    // private: no mutex lock

    try
    {
        CallPtr call( new Call( req->job_id, req->party, callback_, dialer_ ) );

        job_id_queue_.push_back( req->job_id );

        jobman_.insert_job( req->job_id, call );

        dummy_log_debug( MODULENAME, "insert_job: inserted job %u", req->job_id );

        return true;
    }
    catch( std::exception & e )
    {
        dummy_log_fatal( MODULENAME, "insert_job: error - %s", e.what() );

        ASSERT( 0 );

        return false;
    }
}
bool CallManager::handle( const CalmanRemoveJob * req )
{
    // private: no mutex lock

    return remove_job__( req->job_id );
}
bool CallManager::remove_job__( uint32 job_id )
{
    // private: no mutex lock

    try
    {
        JobIdQueue::iterator it = std::find( job_id_queue_.begin(), job_id_queue_.end(), job_id );
        if( it != job_id_queue_.end() )
        {
            dummy_log_debug( MODULENAME, "removed job %u from pending queue", job_id );

            job_id_queue_.erase( it );
        }

        uint32 call_id  = jobman_.get_child_id_by_parent_id( job_id );

        jobman_.remove_job( job_id );

        dummy_log_debug( MODULENAME, "removed job %u from map (call %u)", job_id, call_id );

        return true;
    }
    catch( std::exception & e )
    {
        dummy_log_fatal( MODULENAME, "remove_job: error - %s", e.what() );

        ASSERT( 0 );

        return false;
    }
}

void CallManager::handle( const CalmanPlayFile * req )
{
    // private: no mutex lock

    jobman_.get_job_by_parent_job_id( req->job_id )->play_file( req->filename );
}

void CallManager::handle( const CalmanDrop * req )
{
    // private: no mutex lock

    jobman_.get_job_by_parent_job_id( req->job_id )->drop();
}

bool CallManager::shutdown()
{
    SCOPE_LOCK( mutex_ );

    must_stop_  = true;

    ServerBase::shutdown();

    return true;
}

// IDialerCallback interface
void CallManager::on_call_initiate_response( uint32 call_id, uint32 status )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_DIALER_RESP )
    {
        dummy_log_fatal( MODULENAME, "on_call_initiate_response: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    ASSERT( curr_job_id_ );    // curr job must not be empty

    jobman_.assign_child_id( curr_job_id_, call_id );

    state_  = BUSY;
}

void CallManager::on_error_response( uint32 error, const std::string & descr )
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

void CallManager::on_dial( uint32 call_id )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );     // curr job must not be empty
    ASSERT( curr_job_id_ == jobman_.get_parent_id_by_child_id( call_id ) );

    jobman_.get_job_by_child_job_id( call_id )->on_dial();
}
void CallManager::on_ring( uint32 call_id )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );     // curr job must not be empty
    ASSERT( curr_job_id_ == jobman_.get_parent_id_by_child_id( call_id ) );

    jobman_.get_job_by_child_job_id( call_id )->on_ring();
}

void CallManager::on_call_started( uint32 call_id )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );     // curr job must not be empty
    ASSERT( curr_job_id_ == jobman_.get_parent_id_by_child_id( call_id ) );

    jobman_.get_job_by_child_job_id( call_id )->on_call_started();
}

void CallManager::on_call_duration( uint32 call_id, uint32 t )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );     // curr job must not be empty
    ASSERT( curr_job_id_ == jobman_.get_parent_id_by_child_id( call_id ) );

    jobman_.get_job_by_child_job_id( call_id )->on_call_duration( t );
}

void CallManager::on_call_end( uint32 call_id, uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );    // curr job must not be empty
    ASSERT( curr_job_id_ == jobman_.get_parent_id_by_child_id( call_id ) );

    jobman_.get_job_by_child_job_id( call_id )->on_call_end( errorcode );
}

void CallManager::on_ready()
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

        const std::string & party = jobman_.get_job_by_parent_job_id( curr_job_id_ )->get_party();

        dialer_->initiate_call( party );

        state_  = WAITING_DIALER_RESP;
    }
    break;

    default:
        break;
    }
}
void CallManager::on_error( uint32 call_id, uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );    // curr job must not be empty
    ASSERT( curr_job_id_ == jobman_.get_parent_id_by_child_id( call_id ) );

    jobman_.get_job_by_child_job_id( call_id )->on_error( errorcode );
}

void CallManager::on_fatal_error( uint32 call_id, uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );    // curr job must not be empty
    ASSERT( curr_job_id_ == jobman_.get_parent_id_by_child_id( call_id ) );

    jobman_.get_job_by_child_job_id( call_id )->on_fatal_error( errorcode );
}

NAMESPACE_CALMAN_END
