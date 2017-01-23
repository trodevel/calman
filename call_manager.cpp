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


// $Revision: 5603 $ $Date:: 2017-01-23 #$ $Author: serge $

#include "call_manager.h"               // self

#include "../utils/mutex_helper.h"      // MUTEX_SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../simple_voip/i_simple_voip.h"  // ISimpleVoip
#include "../simple_voip/object_factory.h"  // create_message_t
#include "../utils/assert.h"            // ASSERT

#include "object_factory.h"             // create_message_t

#define MODULENAME      "CallManager"

NAMESPACE_CALMAN_START

CallManager::CallManager():
    WorkerBase( this ),
    num_active_jobs_( 0 ), voips_( nullptr ), callback_( nullptr )
{
}
CallManager::~CallManager()
{
    MUTEX_SCOPE_LOCK( mutex_ );

    job_queue_.clear();
}

bool CallManager::init( simple_voip::ISimpleVoip * voips, const Config & cfg )
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

bool CallManager::register_callback( simple_voip::ISimpleVoipCallback * callback )
{
    if( callback == 0L )
        return false;

    MUTEX_SCOPE_LOCK( mutex_ );

    if( callback_ != 0L )
        return false;

    callback_ = callback;

    return true;
}

void CallManager::consume( const simple_voip::ForwardObject* obj )
{
    WorkerBase::consume( obj );
}

void CallManager::consume( const simple_voip::CallbackObject* obj )
{
    WorkerBase::consume( obj );
}

void CallManager::handle( const workt::IObject* req )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( typeid( *req ) == typeid( simple_voip::InitiateCallRequest ) )
    {
        handle( dynamic_cast< const simple_voip::InitiateCallRequest *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::DropRequest ) )
    {
        handle( dynamic_cast< const simple_voip::DropRequest *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::PlayFileRequest ) )
    {
        handle( dynamic_cast< const simple_voip::PlayFileRequest *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::InitiateCallResponse ) )
    {
        handle( dynamic_cast< const simple_voip::InitiateCallResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::ErrorResponse ) )
    {
        handle( dynamic_cast< const simple_voip::ErrorResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::RejectResponse ) )
    {
        handle( dynamic_cast< const simple_voip::RejectResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::DropResponse ) )
    {
        handle( dynamic_cast< const simple_voip::DropResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::Dialing ) )
    {
        handle( dynamic_cast< const simple_voip::Dialing *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::Ringing ) )
    {
        handle( dynamic_cast< const simple_voip::Ringing *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::Connected ) )
    {
        handle( dynamic_cast< const simple_voip::Connected *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::CallDuration ) )
    {
        handle( dynamic_cast< const simple_voip::CallDuration *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::ConnectionLost ) )
    {
        handle( dynamic_cast< const simple_voip::ConnectionLost *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::Failed ) )
    {
        handle( dynamic_cast< const simple_voip::Failed *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::PlayFileResponse ) )
    {
        handle( dynamic_cast< const simple_voip::PlayFileResponse *>( req ) );
    }
    else if( typeid( *req ) == typeid( simple_voip::DtmfTone ) )
    {
        handle( dynamic_cast< const simple_voip::DtmfTone *>( req ) );
    }
    else
    {
        dummy_log_fatal( MODULENAME, "handle: cannot cast request to known type - %s", typeid( *req ).name() );

        ASSERT( 0 );

        delete req;
    }

    //delete req; // no need to delete request, because it will be forwarded
}

void CallManager::check_call_end( CallPtr call )
{
    if( call->is_completed() )
    {
        ASSERT( num_active_jobs_ > 0 );

        num_active_jobs_--;

        dummy_log_debug( MODULENAME, "removing call id - %u", call->get_parent_job_id() );

        process_jobs();
    }
}

void CallManager::process_jobs()
{
    // private: no MUTEX lock needed

    dummy_log_trace( MODULENAME, "process_jobs: number of active jobs = %u, maximal number of active jobs", num_active_jobs_, cfg_.max_active_jobs );

    while( job_queue_.empty() == false )
    {
        if( num_active_jobs_ == cfg_.max_active_jobs )
        {
            dummy_log_debug( MODULENAME, "process_jobs: maximal number of active jobs is reached %u", cfg_.max_active_jobs );

            break;
        }

        auto job_pair = job_queue_.front();

        auto job_id = job_pair.first;

        dummy_log_debug( MODULENAME, "process_jobs: taking job id %u from queue", job_id );

        job_queue_.pop_front();

        auto it = map_job_id_to_call_.find( job_id );

        if( it == map_job_id_to_call_.end() )
        {
            dummy_log_debug( MODULENAME, "process_jobs: cannot find job id %u", job_id );

            continue;
        }

        auto job = it->second;

        job->handle( job_pair.second );

        num_active_jobs_++;
    }
}

void CallManager::handle( const simple_voip::InitiateCallRequest * req )
{
    // private: no mutex lock

    try
    {
        auto it = map_job_id_to_call_.find( req->job_id );

        if( it != map_job_id_to_call_.end() )
        {
            dummy_log_error( MODULENAME, "job %u already exists", req->job_id );

            send_error_response( req->job_id, "job already exists" );
            return;
        }

        CallPtr call( new Call( req->party, callback_, voips_ ) );

        job_queue_.push_back( std::make_pair( req->job_id, req ) );

        map_job_id_to_call_.insert( MapJobIdToCall::value_type( req->job_id, CallPtr ) );

        dummy_log_debug( MODULENAME, "insert_job: inserted job %u", req->job_id );

        process_jobs();
    }
    catch( std::exception & e )
    {
        dummy_log_fatal( MODULENAME, "insert_job: error - %s", e.what() );

        ASSERT( 0 );
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
void CallManager::forward_request_to_call( const _OBJ * obj )
{
    auto it = map_call_id_to_call_.find( obj->call_id );

    if( it == map_call_id_to_call_.end() )
    {
        dummy_log_info( MODULENAME, "cannot forward to call, call id %u not found", obj->call_id );
        return;
    }

    auto call = it->second;

    auto _b = map_job_id_to_call_.insert( MapJobIdToCall::value_type( obj->job_id, call ) ).second;

    ASSERT( _b );

    call->handle( obj );

    check_call_end( call );
}

template <class _OBJ>
void CallManager::forward_response_to_call( const _OBJ * obj )
{
    auto it = map_job_id_to_call_.find( obj->job_id );

    if( it == map_job_id_to_call_.end() )
    {
        dummy_log_info( MODULENAME, "cannot forward to call, job id %u not found", obj->call_id );
        return;
    }

    auto call = it->second;

    map_job_id_to_call_.erase( it );    // erase request after getting response

    call->handle( obj );

    check_call_end( call );
}

template <class _OBJ>
void CallManager::forward_event_to_call( const _OBJ * obj )
{
    auto it = map_call_id_to_call_.find( obj->call_id );

    if( it == map_call_id_to_call_.end() )
    {
        dummy_log_info( MODULENAME, "cannot forward to call, call id %u not found", obj->call_id );
        return;
    }

    auto call = it->second;

    call->handle( obj );

    check_call_end( call );
}

void CallManager::handle( const simple_voip::DropRequest * req )
{
    forward_request_to_call( req );
}

void CallManager::handle( const simple_voip::PlayFileRequest * req )
{
    forward_request_to_call( req );
}

// IDialerCallback interface
void CallManager::handle( const simple_voip::InitiateCallResponse * obj )
{
    forward_response_to_call( obj );
}
void CallManager::handle( const simple_voip::RejectResponse * obj )
{
    forward_response_to_call( obj );
}
void CallManager::handle( const simple_voip::ErrorResponse * obj )
{
    forward_response_to_call( obj );
}
void CallManager::handle( const simple_voip::DropResponse * obj )
{
    forward_response_to_call( obj );
}
void CallManager::handle( const simple_voip::Dialing * obj )
{
    forward_event_to_call( obj );
}
void CallManager::handle( const simple_voip::Ringing * obj )
{
    forward_event_to_call( obj );
}
void CallManager::handle( const simple_voip::Connected * obj )
{
    forward_event_to_call( obj );
}
void CallManager::handle( const simple_voip::CallDuration * obj )
{
    // ignored
}
void CallManager::handle( const simple_voip::ConnectionLost * obj )
{
    forward_event_to_call( obj );
}

void CallManager::handle( const simple_voip::Failed * obj )
{
    forward_event_to_call( obj );
}

void CallManager::handle( const simple_voip::PlayFileResponse * obj )
{
    forward_response_to_call( obj );
}

void CallManager::handle( const simple_voip::DtmfTone * obj )
{
    forward_event_to_call( obj );
}

void CallManager::send_error_response( uint32_t job_id, const std::string & descr )
{
    callback_consume( simple_voip::create_error_response( job_id, descr ) );
}

void CallManager::send_reject_response( uint32_t job_id, const std::string & descr )
{
    callback_consume( simple_voip::create_reject_response( job_id, descr ) );
}

void CallManager::callback_consume( const simple_voip::CallbackObject * req )
{
    if( callback_ )
        callback_->consume( req );
}


NAMESPACE_CALMAN_END
