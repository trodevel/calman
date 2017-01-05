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


// $Revision: 5459 $ $Date:: 2017-01-04 #$ $Author: serge $

#include "call_manager.h"               // self

#include "../utils/mutex_helper.h"      // MUTEX_SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../voip_io/i_voip_service.h"  // IVoipService
#include "../voip_io/object_factory.h"  // create_message_t
#include "../utils/assert.h"            // ASSERT

#include "object_factory.h"             // create_message_t
#include "i_call_manager_callback.h"    // ICallManagerCallback

#define MODULENAME      "CallManager"

NAMESPACE_CALMAN_START

const char* to_c_str( CallManager::state_e s )
{
    static const char *vals[]=
    {
            "IDLE", "BUSY"
    };

    if( s < CallManager::IDLE || s > CallManager::BUSY )
        return "???";

    return vals[ (int) s ];
}

CallManager::CallManager():
    WorkerBase( this ),
    state_( IDLE ), voips_( nullptr ), callback_( nullptr ), curr_job_( 0L )
{
}
CallManager::~CallManager()
{
    MUTEX_SCOPE_LOCK( mutex_ );

    job_queue_.clear();

    if( curr_job_ )
    {
        curr_job_   = 0L;
    }
}

bool CallManager::init( voip_service::IVoipService * voips, const Config & cfg )
{
    if( voips == 0L )
        return false;

    MUTEX_SCOPE_LOCK( mutex_ );

    if( voips_ != 0L )
        return false;

    voips_  = voips;
    cfg_    = cfg;

    dummy_log_debug( MODULENAME, "inited" );

    return true;
}

bool CallManager::register_callback( ICallManagerCallback * callback )
{
    if( callback == 0L )
        return false;

    MUTEX_SCOPE_LOCK( mutex_ );

    if( callback_ != 0L )
        return false;

    callback_ = callback;

    return true;
}

void CallManager::consume( const Object* obj )
{
    WorkerBase::consume( obj );
}

void CallManager::consume( const voip_service::CallbackObject* obj )
{
    WorkerBase::consume( obj );
}

void CallManager::handle( const workt::IObject* req )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( typeid( *req ) == typeid( InitiateCallRequest ) )
    {
        handle( dynamic_cast< const InitiateCallRequest *>( req ) );
    }
    else if( typeid( *req ) == typeid( DropRequest ) )
    {
        handle( dynamic_cast< const DropRequest *>( req ) );
    }
    else if( typeid( *req ) == typeid( PlayFileRequest ) )
    {
        handle( dynamic_cast< const PlayFileRequest *>( req ) );
    }
    else if( typeid( *req ) == typeid( voip_service::InitiateCallResponse ) )
    {
        handle( dynamic_cast< const voip_service::InitiateCallResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( voip_service::ErrorResponse ) )
    {
        handle( dynamic_cast< const voip_service::ErrorResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( voip_service::RejectResponse ) )
    {
        handle( dynamic_cast< const voip_service::RejectResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( voip_service::DropResponse ) )
    {
        handle( dynamic_cast< const voip_service::DropResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( voip_service::Dial ) )
    {
        handle( dynamic_cast< const voip_service::Dial *>( req ) );
    }
    else if( typeid( *req ) == typeid( voip_service::Ring ) )
    {
        handle( dynamic_cast< const voip_service::Ring *>( req ) );
    }
    else if( typeid( *req ) == typeid( voip_service::Connected ) )
    {
        handle( dynamic_cast< const voip_service::Connected *>( req ) );
    }
    else if( typeid( *req ) == typeid( voip_service::CallDuration ) )
    {
        handle( dynamic_cast< const voip_service::CallDuration *>( req ) );
    }
    else if( typeid( *req ) == typeid( voip_service::ConnectionLost ) )
    {
        handle( dynamic_cast< const voip_service::ConnectionLost *>( req ) );
    }
    else if( typeid( *req ) == typeid( voip_service::Failed ) )
    {
        handle( dynamic_cast< const voip_service::Failed *>( req ) );
    }
    else if( typeid( *req ) == typeid( voip_service::PlayFileResponse ) )
    {
        handle( dynamic_cast< const voip_service::PlayFileResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( voip_service::DtmfTone ) )
    {
        handle( dynamic_cast< const voip_service::DtmfTone *>( req ) );
    }
    else
    {
        dummy_log_fatal( MODULENAME, "handle: cannot cast request to known type - %p", (void *) req );

        ASSERT( 0 );
    }

    delete req;
}

void CallManager::check_call_end()
{
    ASSERT( state_ == BUSY );

    ASSERT( curr_job_ );    // curr job must not be empty

    if( curr_job_->is_completed() == false )
        return;

    state_  = IDLE;
    trace_state_switch();

    dummy_log_debug( MODULENAME, "removing call id - %u", curr_job_->get_parent_job_id() );

    curr_job_.reset();      // as call finished, curr job can be deleted

    process_jobs();
}

void CallManager::process_jobs()
{
    // private: no MUTEX lock needed

    ASSERT( state_ == IDLE ); // just paranoid check

    if( job_queue_.empty() )
        return;

    ASSERT( curr_job_ == nullptr );  // curr_job_ must be empty

    curr_job_ = job_queue_.front();

    job_queue_.pop_front();

    curr_job_->initiate();

    state_  = BUSY;

    trace_state_switch();
}

void CallManager::handle( const InitiateCallRequest * req )
{
    // private: no mutex lock

    try
    {
        if( curr_job_ && curr_job_->get_parent_job_id() == req->job_id )
        {
            dummy_log_error( MODULENAME, "job %u already exists and is active", req->job_id );

            send_error_response( req->job_id, "job already exists and is active" );
            return;
        }

        auto it = find( req->job_id );

        if( it != job_queue_.end() )
        {
            dummy_log_error( MODULENAME, "job %u already exists in the queue", req->job_id );

            send_error_response( req->job_id, "job already exists in the queue" );

            return;
        }

        CallPtr call( new Call( req->job_id, req->party, callback_, voips_ ) );

        job_queue_.push_back( call );

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

void CallManager::handle( const DropRequest * req )
{
    // private: no mutex lock

    if( curr_job_ && curr_job_->get_parent_job_id() == req->job_id )
    {
        curr_job_->handle( req );
    }
    else
    {
        auto it = find( req->job_id );

        if( it == job_queue_.end() )
        {
            dummy_log_error( MODULENAME, "job %u not found", req->job_id );

            send_error_response( req->job_id, "job not found" );

            return;
        }

        dummy_log_info( MODULENAME, "removed job %u from queue", req->job_id );

        job_queue_.erase( it );

        callback_consume( create_message_t<DropResponse>( req->job_id ) );
    }
}

CallManager::JobQueue::iterator CallManager::find( uint32_t job_id )
{
    auto it_end = job_queue_.end();

    for( auto it = job_queue_.begin(); it != it_end; ++it )
    {
        if( (*it)->get_parent_job_id() == job_id )
            return it;
    }

    return it_end;
}

void CallManager::handle( const PlayFileRequest * req )
{
    // private: no mutex lock

    if( curr_job_ && curr_job_->get_parent_job_id() == req->job_id )
    {
        curr_job_->handle( req );
    }
    else
    {
        auto it = find( req->job_id );

        if( it == job_queue_.end() )
        {
            dummy_log_error( MODULENAME, "job %u not found", req->job_id );

            send_error_response( req->job_id, "job not found" );

            return;
        }

        dummy_log_error( MODULENAME, "job %u is not active", req->job_id );

        send_reject_response( req->job_id, "job is not active" );
    }
}

void CallManager::start()
{
    dummy_log_debug( MODULENAME, "start()" );

    WorkerBase::start();
}

bool CallManager::shutdown()
{
    dummy_log_debug( MODULENAME, "shutdown()" );

    MUTEX_SCOPE_LOCK( mutex_ );

    WorkerBase::shutdown();

    return true;
}


template <class _OBJ>
void CallManager::forward_to_call( const _OBJ * obj )
{
    // private: no mutex lock

    ASSERT( state_ == BUSY );
    ASSERT( curr_job_ );

    curr_job_->handle( obj );

    check_call_end();
}

// IDialerCallback interface
void CallManager::handle( const voip_service::InitiateCallResponse * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const voip_service::RejectResponse * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const voip_service::ErrorResponse * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const voip_service::DropResponse * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const voip_service::Dial * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const voip_service::Ring * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const voip_service::Connected * obj )
{
    forward_to_call( obj );
}
void CallManager::handle( const voip_service::CallDuration * obj )
{
    // ignored
}
void CallManager::handle( const voip_service::ConnectionLost * obj )
{
    forward_to_call( obj );
}

void CallManager::handle( const voip_service::Failed * obj )
{
    forward_to_call( obj );
}

void CallManager::handle( const voip_service::PlayFileResponse * obj )
{
    forward_to_call( obj );
}

void CallManager::handle( const voip_service::DtmfTone * obj )
{
    forward_to_call( obj );
}

void CallManager::trace_state_switch() const
{
    dummy_log_debug( MODULENAME, "switched to %s", to_c_str( state_ ) );
}

void CallManager::send_error_response( uint32_t job_id, const std::string & descr )
{
    callback_consume( create_error_response( job_id, descr ) );
}

void CallManager::send_reject_response( uint32_t job_id, const std::string & descr )
{
    callback_consume( create_reject_response( job_id, descr ) );
}

void CallManager::callback_consume( const CallbackObject * req )
{
    if( callback_ )
        callback_->consume( req );
}


NAMESPACE_CALMAN_END
