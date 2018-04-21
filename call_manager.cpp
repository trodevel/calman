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


// $Revision: 8944 $ $Date:: 2018-04-20 #$ $Author: serge $

#include "call_manager.h"               // self

#include "../utils/mutex_helper.h"      // MUTEX_SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../utils/assert.h"            // ASSERT

#define MODULENAME      "CallManager"

NAMESPACE_CALMAN_START

CallManager::CallManager():
    voips_( nullptr ), callback_( nullptr )
{
}

CallManager::~CallManager()
{
    MUTEX_SCOPE_LOCK( mutex_ );

//    for( auto c : map_call_id_to_call_ )
//    {
//        delete c.second;
//    }

    request_queue_.clear();
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
    MUTEX_SCOPE_LOCK( mutex_ );

    if( typeid( *obj ) == typeid( simple_voip::InitiateCallRequest ) )
    {
        handle( dynamic_cast< const simple_voip::InitiateCallRequest *>( obj ) );
    }
    else
    {
        voips_->consume( obj );
    }
}

void CallManager::consume( const simple_voip::CallbackObject* obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( typeid( *obj ) == typeid( simple_voip::InitiateCallResponse ) )
    {
        handle( dynamic_cast< const simple_voip::InitiateCallResponse *>( obj ) );
    }
    else if( typeid( *obj ) == typeid( simple_voip::ErrorResponse ) )
    {
        handle( dynamic_cast< const simple_voip::ErrorResponse *>( obj ) );
    }
    else if( typeid( *obj ) == typeid( simple_voip::RejectResponse ) )
    {
        handle( dynamic_cast< const simple_voip::RejectResponse *>( obj ) );
    }
    else if( typeid( *obj ) == typeid( simple_voip::DropResponse ) )
    {
        handle( dynamic_cast< const simple_voip::DropResponse *>( obj ) );
    }
    else if( typeid( *obj ) == typeid( simple_voip::ConnectionLost ) )
    {
        handle( dynamic_cast< const simple_voip::ConnectionLost *>( obj ) );
    }
    else if( typeid( *obj ) == typeid( simple_voip::Failed ) )
    {
        handle( dynamic_cast< const simple_voip::Failed *>( obj ) );
    }

    callback_->consume( obj );
}

void CallManager::process_jobs()
{
    // private: no MUTEX lock needed

    dummy_log_trace( MODULENAME, "process_jobs: active calls %u, active requests %u, maximal number of active calls %u",
                active_call_ids_.size(), active_request_ids_.size(), cfg_.max_active_calls );

    while( get_num_of_activities() < cfg_.max_active_calls )
    {
        if( request_queue_.empty() )
        {
            dummy_log_debug( MODULENAME, "process_jobs: request queue is empty" );
            break;
        }

        auto req = request_queue_.front();

        dummy_log_debug( MODULENAME, "process_jobs: taking job id %u from queue", req->req_id );

        request_queue_.pop_front();

        process( req );
    }
}

void CallManager::handle( const simple_voip::InitiateCallRequest * req )
{
    // private: no mutex lock

    dummy_log_debug( MODULENAME, "insert_job: active calls %u, active requests %u, pending requests %u",
            active_call_ids_.size(), active_request_ids_.size(), request_queue_.size() );

    if( get_num_of_activities() >= cfg_.max_active_calls )
    {
        request_queue_.push_back( req );

        dummy_log_debug( MODULENAME, "insert_job: inserted job %u", req->req_id );

        return;
    }

    process( req );
}

void CallManager::process( const simple_voip::InitiateCallRequest * req )
{
    // private: no mutex lock

    auto res = active_request_ids_.insert( req->req_id ).second;

    if( res == false )
    {
        dummy_log_error( MODULENAME, "request %u already exists", req->req_id );

        ASSERT( 0 );

        return;
    }

    voips_->consume( req );
}

bool CallManager::shutdown()
{
    dummy_log_debug( MODULENAME, "shutdown()" );

    MUTEX_SCOPE_LOCK( mutex_ );

    WorkerBase::shutdown();

    return true;
}

void CallManager::handle( const simple_voip::DropRequest * req )
{
    if( active_call_ids_.count( req->call_id ) == 0 )
    {
        dummy_log_warn( MODULENAME, "unknown call id %u", req->call_id );

        voips_->consume( req );

        return;
    }

    auto _b = map_drop_req_id_to_call_id_.insert( std::make_pair( req->req_id, req->call_id ) ).second;

    ASSERT( _b );

    voips_->consume( req );
}

// ISimpleVoipCallback interface
void CallManager::handle( const simple_voip::InitiateCallResponse * obj )
{
    auto it = active_request_ids_.find( obj->req_id );

    if( it == active_request_ids_.end() )
        return;

    active_request_ids_.erase( it );

    auto b = active_call_ids_.insert( obj->call_id ).second;

    if( b == false )
    {
        dummy_log_error( MODULENAME, "cannot insert call id %u - already exists", obj->call_id );

        ASSERT( 0 );

        return;
    }
}

void CallManager::handle( const simple_voip::RejectResponse * obj )
{
    auto it = active_request_ids_.find( obj->req_id );

    if( it == active_request_ids_.end() )
    {
        erase_failed_drop_request( obj->req_id );
        return;
    }

    active_request_ids_.erase( it );

    process_jobs();
}

void CallManager::handle( const simple_voip::ErrorResponse * obj )
{
    auto it = active_request_ids_.find( obj->req_id );

    if( it == active_request_ids_.end() )
    {
        erase_failed_drop_request( obj->req_id );
        return;
    }

    active_request_ids_.erase( it );

    process_jobs();
}

void CallManager::handle( const simple_voip::DropResponse * obj )
{
    auto it = map_drop_req_id_to_call_id_.find( obj->req_id );

    if( it == map_drop_req_id_to_call_id_.end() )
        return;

    auto call_id = it->second;

    map_drop_req_id_to_call_id_.erase( it );

    auto it_2 = active_call_ids_.find( call_id );

    if( it_2 == active_call_ids_.end() )
    {
        dummy_log_warn( MODULENAME, "unknown call id %u", call_id );
        return;
    }

    active_call_ids_.erase( it_2 );

    process_jobs();
}

void CallManager::handle( const simple_voip::ConnectionLost * obj )
{
    handle_failed_call( obj->call_id );
}

void CallManager::handle( const simple_voip::Failed * obj )
{
    handle_failed_call( obj->call_id );
}

void CallManager::handle_failed_call( uint32_t call_id )
{
    auto it = active_call_ids_.find( call_id );

    if( it == active_call_ids_.end() )
    {
        dummy_log_warn( MODULENAME, "unknown call id %u", call_id );

        return;
    }

    active_call_ids_.erase( it );

    process_jobs();
}

void CallManager::erase_failed_drop_request( uint32_t req_id )
{
    auto it = map_drop_req_id_to_call_id_.find( req_id );

    if( it == map_drop_req_id_to_call_id_.end() )
        return;

    map_drop_req_id_to_call_id_.erase( it );
}

uint32_t CallManager::get_num_of_activities() const
{
    return active_call_ids_.size() + active_request_ids_.size();
}


NAMESPACE_CALMAN_END
