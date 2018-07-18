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


// $Revision: 9561 $ $Date:: 2018-07-18 #$ $Author: serge $

#include "call_manager.h"               // self

#include <typeindex>                    // std::type_index
#include <typeinfo>
#include <unordered_map>

#include "utils/mutex_helper.h"         // MUTEX_SCOPE_LOCK
#include "utils/dummy_logger.h"         // dummy_log
#include "utils/assert.h"               // ASSERT

#define MODULENAME      "CallManager"

NAMESPACE_CALMAN_START

CallManager::CallManager():
    log_id_( 0 ),
    voips_( nullptr ), callback_( nullptr )
{
}

CallManager::~CallManager()
{
    MUTEX_SCOPE_LOCK( mutex_ );

    request_queue_.clear();
}

bool CallManager::init(
        unsigned int                        log_id,
        simple_voip::ISimpleVoip            * voips,
        simple_voip::ISimpleVoipCallback    * callback,
        const Config                        & cfg,
        std::string                         * error_msg )
{
    if( voips == nullptr )
        return false;

    if( callback == nullptr )
        return false;

    MUTEX_SCOPE_LOCK( mutex_ );

    if( voips_ != nullptr )
        return false;

    if( callback_ != nullptr )
        return false;

    log_id_     = log_id;
    voips_      = voips;
    callback_   = callback;
    cfg_        = cfg;

    if( cfg_.max_active_calls < 1 )
    {
        * error_msg = "max_active_call < 1";
        return false;
    }

    dummy_log_debug( log_id_, "inited, max_active_calls=%u", cfg_.max_active_calls );

    return true;
}

void CallManager::consume( const simple_voip::ForwardObject* obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    typedef CallManager Type;

    typedef void (Type::*PPMF)( const simple_voip::ForwardObject * r );

#define HANDLER_MAP_ENTRY(_v)       { typeid( simple_voip::_v ),            & Type::handle_##_v }

    static const std::unordered_map<std::type_index, PPMF> funcs =
    {
        HANDLER_MAP_ENTRY( InitiateCallRequest ),
        HANDLER_MAP_ENTRY( DropRequest ),
    };

#undef HANDLER_MAP_ENTRY

    auto it = funcs.find( typeid( * obj ) );

    if( it != funcs.end() )
    {
        (this->*it->second)( obj );
    }
    else
    {
        voips_->consume( obj );
    }
}

void CallManager::consume( const simple_voip::CallbackObject* obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    typedef CallManager Type;

    typedef void (Type::*PPMF)( const simple_voip::CallbackObject * r );

#define HANDLER_MAP_ENTRY(_v)       { typeid( simple_voip::_v ),            & Type::handle_##_v }

    static const std::unordered_map<std::type_index, PPMF> funcs =
    {
        HANDLER_MAP_ENTRY( InitiateCallResponse ),
        HANDLER_MAP_ENTRY( ErrorResponse ),
        HANDLER_MAP_ENTRY( RejectResponse ),
        HANDLER_MAP_ENTRY( DropResponse ),
        HANDLER_MAP_ENTRY( ConnectionLost ),
        HANDLER_MAP_ENTRY( Failed ),
    };

#undef HANDLER_MAP_ENTRY

    auto it = funcs.find( typeid( * obj ) );

    if( it != funcs.end() )
    {
        (this->*it->second)( obj );
    }

    callback_->consume( obj );
}

void CallManager::process_jobs()
{
    // private: no MUTEX lock needed

    log_stat();

    while( get_num_of_activities() < cfg_.max_active_calls )
    {
        if( request_queue_.empty() )
        {
            dummy_log_debug( log_id_, "process_jobs: request queue is empty" );
            break;
        }

        auto req = request_queue_.front();

        dummy_log_debug( log_id_, "process_jobs: taking job id %u from queue", req->req_id );

        request_queue_.pop_front();

        process( req );
    }

    log_stat();
}

void CallManager::process( const simple_voip::InitiateCallRequest * req )
{
    // private: no mutex lock

    auto res = active_request_ids_.insert( req->req_id ).second;

    if( res == false )
    {
        dummy_log_error( log_id_, "request %u already exists", req->req_id );

        ASSERT( 0 );

        return;
    }

    voips_->consume( req );
}

bool CallManager::shutdown()
{
    dummy_log_debug( log_id_, "shutdown()" );

    MUTEX_SCOPE_LOCK( mutex_ );

    return true;
}

void CallManager::handle_InitiateCallRequest( const simple_voip::ForwardObject * rreq )
{
    auto * req = dynamic_cast< const simple_voip::InitiateCallRequest *>( rreq );

    // private: no mutex lock

    log_stat();

    if( get_num_of_activities() >= cfg_.max_active_calls )
    {
        request_queue_.push_back( req );

        dummy_log_debug( log_id_, "insert_job: inserted job %u", req->req_id );

        log_stat();

        return;
    }

    process( req );
}

void CallManager::handle_DropRequest( const simple_voip::ForwardObject * rreq )
{
    auto * req = dynamic_cast< const simple_voip::DropRequest *>( rreq );

    if( active_call_ids_.count( req->call_id ) == 0 )
    {
        dummy_log_warn( log_id_, "unknown call id %u", req->call_id );

        voips_->consume( req );

        return;
    }

    auto _b = map_drop_req_id_to_call_id_.insert( std::make_pair( req->req_id, req->call_id ) ).second;

    ASSERT( _b );

    voips_->consume( req );
}

// ISimpleVoipCallback interface
void CallManager::handle_InitiateCallResponse( const simple_voip::CallbackObject * oobj )
{
    auto * obj = dynamic_cast< const simple_voip::InitiateCallResponse *>( oobj );

    dummy_log_trace( log_id_, "initiated call: %u", obj->call_id );

    auto it = active_request_ids_.find( obj->req_id );

    if( it == active_request_ids_.end() )
    {
        dummy_log_error( log_id_, "unknown call id %u", obj->call_id );
        return;
    }

    active_request_ids_.erase( it );

    auto b = active_call_ids_.insert( obj->call_id ).second;

    if( b == false )
    {
        dummy_log_error( log_id_, "cannot insert call id %u - already exists", obj->call_id );

        ASSERT( 0 );

        return;
    }

    dummy_log_debug( log_id_, "call id %u - active", obj->call_id );

    log_stat();
}

void CallManager::handle_RejectResponse( const simple_voip::CallbackObject * oobj )
{
    auto * obj = dynamic_cast< const simple_voip::RejectResponse *>( oobj );

    auto it = active_request_ids_.find( obj->req_id );

    if( it == active_request_ids_.end() )
    {
        erase_failed_drop_request( obj->req_id );
        return;
    }

    active_request_ids_.erase( it );

    process_jobs();
}

void CallManager::handle_ErrorResponse( const simple_voip::CallbackObject * oobj )
{
    auto * obj = dynamic_cast< const simple_voip::ErrorResponse *>( oobj );

    auto it = active_request_ids_.find( obj->req_id );

    if( it == active_request_ids_.end() )
    {
        erase_failed_drop_request( obj->req_id );
        return;
    }

    active_request_ids_.erase( it );

    process_jobs();
}

void CallManager::handle_DropResponse( const simple_voip::CallbackObject * oobj )
{
    auto * obj = dynamic_cast< const simple_voip::DropResponse *>( oobj );

    auto it = map_drop_req_id_to_call_id_.find( obj->req_id );

    if( it == map_drop_req_id_to_call_id_.end() )
        return;

    auto call_id = it->second;

    map_drop_req_id_to_call_id_.erase( it );

    auto it_2 = active_call_ids_.find( call_id );

    if( it_2 == active_call_ids_.end() )
    {
        dummy_log_warn( log_id_, "unknown call id %u", call_id );
        return;
    }

    active_call_ids_.erase( it_2 );

    process_jobs();
}

void CallManager::handle_ConnectionLost( const simple_voip::CallbackObject * oobj )
{
    auto * obj = dynamic_cast< const simple_voip::ConnectionLost *>( oobj );

    handle_failed_call( obj->call_id );
}

void CallManager::handle_Failed( const simple_voip::CallbackObject * oobj )
{
    auto * obj = dynamic_cast< const simple_voip::Failed *>( oobj );

    handle_failed_call( obj->call_id );
}

void CallManager::handle_failed_call( uint32_t call_id )
{
    dummy_log_debug( log_id_, "failed call id %u", call_id );

    auto it = active_call_ids_.find( call_id );

    if( it == active_call_ids_.end() )
    {
        dummy_log_warn( log_id_, "unknown call id %u", call_id );

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

void CallManager::log_stat()
{
    dummy_log_debug( log_id_, "stat: active calls %u, active requests %u, pending requests %u",
            active_call_ids_.size(), active_request_ids_.size(), request_queue_.size() );
}


NAMESPACE_CALMAN_END
