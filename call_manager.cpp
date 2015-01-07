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


// $Id: call_manager.cpp 1326 2015-01-06 18:10:20Z serge $

#include "call_manager.h"               // self

#include <boost/thread.hpp>             // boost::this_thread

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../dialer/i_dialer.h"         // IDialer
#include "../dialer/object_factory.h"   // DialerCallbackCallObject, create_message_t
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
    state_( UNDEF ), dialer_( 0L ), callback_( nullptr ), curr_job_id_( 0L )
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

void CallManager::consume( const CalmanObject* obj )
{
    ServerBase::consume( obj );
}

void CallManager::consume( const dialer::DialerCallbackObject* obj )
{
    ServerBase::consume( obj );
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
    else if( typeid( *req ) == typeid( dialer::DialerInitiateCallResponse ) )
    {
        handle( dynamic_cast< const dialer::DialerInitiateCallResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( dialer::DialerErrorResponse ) )
    {
        handle( dynamic_cast< const dialer::DialerErrorResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( dialer::DialerDropResponse ) )
    {
        handle( dynamic_cast< const dialer::DialerDropResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( dialer::DialerDial ) )
    {
        handle( dynamic_cast< const dialer::DialerDial *>( req ) );
    }
    else if( typeid( *req ) == typeid( dialer::DialerRing ) )
    {
        handle( dynamic_cast< const dialer::DialerRing *>( req ) );
    }
    else if( typeid( *req ) == typeid( dialer::DialerConnect ) )
    {
        handle( dynamic_cast< const dialer::DialerConnect *>( req ) );
    }
    else if( typeid( *req ) == typeid( dialer::DialerCallDuration ) )
    {
        handle( dynamic_cast< const dialer::DialerCallDuration *>( req ) );
    }
    else if( typeid( *req ) == typeid( dialer::DialerCallEnd ) )
    {
        handle( dynamic_cast< const dialer::DialerCallEnd *>( req ) );
    }
    else if( typeid( *req ) == typeid( dialer::DialerPlayStarted ) )
    {
        handle( dynamic_cast< const dialer::DialerPlayStarted *>( req ) );
    }
    else if( typeid( *req ) == typeid( dialer::DialerPlayStopped ) )
    {
        handle( dynamic_cast< const dialer::DialerPlayStopped *>( req ) );
    }
    else if( typeid( *req ) == typeid( dialer::DialerPlayFailed ) )
    {
        handle( dynamic_cast< const dialer::DialerPlayFailed *>( req ) );
    }
    else
    {
        dummy_log_fatal( MODULENAME, "handle: cannot cast request to known type - %p", (void *) req );

        ASSERT( 0 );
    }

    delete req;
}

void CallManager::handle_call_end()
{
    ASSERT( state_ == BUSY || state_ == WAITING_DIALER_FREE );

    switch( state_ )
    {

    case BUSY:
    {
        dummy_log_debug( MODULENAME, "handle_call_end: switching into state %s", "IDLE" );
        state_  = IDLE;

        ASSERT( curr_job_id_ );    // curr job must not be empty

        remove_job__( curr_job_id_ );

        curr_job_id_    = 0;      // as call finished, curr job can be deleted

        process_jobs();
    }
    break;

    case WAITING_DIALER_FREE:
    {
        dummy_log_debug( MODULENAME, "handle_call_end: switching into state %s", "WAITING_DIALER_RESP" );

        ASSERT( curr_job_id_ );    // curr job must not be empty

        const std::string & party = jobman_.get_job_by_parent_job_id( curr_job_id_ )->get_party();

        dialer_->consume( dialer::create_initiate_call_request( party ) );

        state_  = WAITING_DIALER_RESP;
    }
    break;

    default:
        break;
    }
}

void CallManager::process_jobs()
{
    // private: no MUTEX lock needed

    ASSERT( state_ == IDLE ); // just paranoid check

    if( job_id_queue_.empty() )
        return;

    ASSERT( curr_job_id_ == 0 );  // curr_job_id_ must be empty

    curr_job_id_ = job_id_queue_.front();

    job_id_queue_.pop_front();

    if( callback_ )
        callback_->consume( create_message_t<CalmanProcessingStarted>( curr_job_id_ ) );

    const std::string & party = jobman_.get_job_by_parent_job_id( curr_job_id_ )->get_party();

    dialer_->consume( dialer::create_initiate_call_request( party ) );

    state_  = WAITING_DIALER_RESP;
}

void CallManager::handle( const CalmanInsertJob * req )
{
    // private: no mutex lock

    try
    {
        CallPtr call( new Call( req->job_id, req->party, callback_, dialer_ ) );

        job_id_queue_.push_back( req->job_id );

        jobman_.insert_job( req->job_id, call );

        dummy_log_debug( MODULENAME, "insert_job: inserted job %u", req->job_id );

        if( state_ == IDLE )
            process_jobs();
    }
    catch( std::exception & e )
    {
        dummy_log_fatal( MODULENAME, "insert_job: error - %s", e.what() );

        ASSERT( 0 );
    }
}
void CallManager::handle( const CalmanRemoveJob * req )
{
    // private: no mutex lock

    remove_job__( req->job_id );
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

    jobman_.get_job_by_parent_job_id( req->job_id )->handle( req );
}

void CallManager::handle( const CalmanDrop * req )
{
    // private: no mutex lock

    jobman_.get_job_by_parent_job_id( req->job_id )->handle( req );
}

bool CallManager::shutdown()
{
    SCOPE_LOCK( mutex_ );

    ServerBase::shutdown();

    return true;
}

// IDialerCallback interface
void CallManager::handle( const dialer::DialerInitiateCallResponse * obj )
{
    // private: no mutex lock

    if( state_ != WAITING_DIALER_RESP )
    {
        dummy_log_fatal( MODULENAME, "on_call_initiate_response: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    ASSERT( curr_job_id_ );    // curr job must not be empty

    jobman_.assign_child_id( curr_job_id_, obj->call_id );

    state_  = BUSY;
}

void CallManager::handle( const dialer::DialerErrorResponse * obj )
{
    // private: no mutex lock

    if( state_ != WAITING_DIALER_RESP )
    {
        dummy_log_fatal( MODULENAME, "on_error_response: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    ASSERT( curr_job_id_ );    // curr job must not be empty

    dummy_log_error( MODULENAME, "on_error_response: dialer is busy, error %u, %s", obj->errorcode, obj->descr.c_str() );

    state_  = WAITING_DIALER_FREE;
}

void CallManager::handle( const dialer::DialerDropResponse * obj )
{
    forward_to_call( obj );

    handle_call_end();
}

template <class _OBJ>
void CallManager::forward_to_call( const _OBJ * obj )
{
    // private: no mutex lock

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_id_ );     // curr job must not be empty
    ASSERT( curr_job_id_ == jobman_.get_parent_id_by_child_id( obj->call_id ) );

    jobman_.get_job_by_child_job_id( obj->call_id )->handle( obj );
}

void CallManager::handle( const dialer::DialerDial * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const dialer::DialerRing * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const dialer::DialerConnect * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const dialer::DialerCallDuration * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const dialer::DialerCallEnd * obj )
{
    forward_to_call( obj );

    handle_call_end();
}
void CallManager::handle( const dialer::DialerPlayStarted * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const dialer::DialerPlayStopped * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const dialer::DialerPlayFailed * obj )
{
    forward_to_call( obj );
}


NAMESPACE_CALMAN_END
