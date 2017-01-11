/*

Call.

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


// $Revision: 5545 $ $Date:: 2017-01-10 #$ $Author: serge $

#include "call.h"                       // self

#include "i_call_manager_callback.h"    // ICallManagerCallback
#include "object_factory.h"             // create_message_t

#include "../utils/mutex_helper.h"      // MUTEX_SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../utils/assert.h"            // ASSERT
#include "../simple_voip/i_simple_voip.h"  // IVoipService
#include "../simple_voip/object_factory.h"  // create_play_file_requiest

NAMESPACE_CALMAN_START

#define MODULENAME      "Call"

unsigned int Call::CLASS_ID = 0;

const char* to_c_str( Call::state_e s )
{
    static const char *vals[]=
    {
            "IDLE",
            "WAITING_DIALER_FREE",
            "WAITING_INITIATE_CALL_RESP",
            "WAITING_CONNECTED",
            "CONNECTED",
            "CONNECTED_BUSY",
            "WRONG_CONNECTED",
            "CANCELLED_IN_WICR",
            "CANCELLED_IN_WC",
            "CANCELLED_IN_C",
            "CANCELLED_IN_CB",
            "DONE",
    };

    if( s < Call::IDLE || s > Call::DONE )
        return "???";

    return vals[ (int) s ];
}

Call::Call(
        uint32_t                    parent_job_id,
        const std::string           & party,
        ICallManagerCallback        * callback,
        voip_service::IVoipService  * voips ):
        party_( party ),
        state_( IDLE ),
        parent_job_id_( parent_job_id ),
        current_req_id_( 0 ),
        call_id_( 0 ),
        sleep_interval_( 1 ),
        callback_( callback ),
        voips_( voips )
{
    ASSERT( parent_job_id );
}

bool Call::is_completed() const
{
    MUTEX_SCOPE_LOCK( mutex_ );

    return state_ == DONE;
}

uint32_t Call::get_parent_job_id() const
{
    MUTEX_SCOPE_LOCK( mutex_ );

    return parent_job_id_;
}

void Call::initiate()
{
    dummy_log_trace( CLASS_ID, "handle(): %s", "initiate" );

    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != IDLE )
    {
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", "initiate()", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    ASSERT( current_req_id_ == 0 );

    current_req_id_ = get_next_request_id();

    voips_->consume( voip_service::create_initiate_call_request( current_req_id_, party_ ) );

    next_state( WAITING_INITIATE_CALL_RESP );
}

void Call::handle( const PlayFileRequest * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != CONNECTED && state_ != CONNECTED_BUSY )
    {
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    if( state_ == CONNECTED_BUSY )
    {
        callback_consume( create_reject_response( parent_job_id_,
                "cannot process request, busy with processing request " + std::to_string( current_req_id_ ) ) );
        return;
    }

    ASSERT( current_req_id_ == 0 );

    current_req_id_ = get_next_request_id();

    voips_->consume( voip_service::create_play_file_request( current_req_id_, call_id_, obj->filename ) );

    next_state( CONNECTED_BUSY );
}

void Call::handle( const DropRequest * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case IDLE:
    case WAITING_DIALER_FREE:

        callback_consume( create_message_t<DropResponse>( parent_job_id_ ) );
        next_state( DONE );
        break;

    case WAITING_INITIATE_CALL_RESP:
        next_state( CANCELLED_IN_WICR );
        break;

    case WAITING_CONNECTED:
        current_req_id_ = get_next_request_id();
        voips_->consume( voip_service::create_drop_request( current_req_id_, call_id_ ) );
        next_state( CANCELLED_IN_WC );
        break;

    case CONNECTED:
        current_req_id_ = get_next_request_id();
        voips_->consume( voip_service::create_drop_request( current_req_id_, call_id_ ) );
        next_state( CANCELLED_IN_C );
        break;

    case CONNECTED_BUSY:
        current_req_id_ = get_next_request_id();
        voips_->consume( voip_service::create_drop_request( current_req_id_, call_id_ ) );
        next_state( CANCELLED_IN_CB );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const voip_service::InitiateCallResponse * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case WAITING_INITIATE_CALL_RESP:
        validate_and_reset_response_job_id( obj );
        callback_consume( create_message_t<InitiateCallResponse>( parent_job_id_ ) );
        call_id_    = obj->call_id;
        next_state( WAITING_CONNECTED );
        break;

    case CANCELLED_IN_WICR:
        call_id_        = obj->call_id;
        current_req_id_ = get_next_request_id();
        voips_->consume( voip_service::create_drop_request( current_req_id_, call_id_ ) );
        next_state( CANCELLED_IN_WC );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const voip_service::ErrorResponse * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case WAITING_INITIATE_CALL_RESP:
        send_error_response( "failed to initiate call: " + obj->descr );
        next_state( DONE );
        break;

    case CANCELLED_IN_WICR:
        callback_consume( create_message_t<DropResponse>( parent_job_id_ ) );
        next_state( DONE );
        break;

    case CANCELLED_IN_CB:
        next_state( CANCELLED_IN_C );
        break;

    case CONNECTED_BUSY:
        validate_and_reset_response_job_id( obj );
        send_error_response( "failed to to process request: " + obj->descr );
        next_state( CONNECTED );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const voip_service::RejectResponse * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case WAITING_INITIATE_CALL_RESP:
        validate_and_reset_response_job_id( obj );
        sleep_interval_ *= 2;
        dummy_log_info( CLASS_ID, "handle: RejectResponse: dialer is busy, sleeping %u sec ...", sleep_interval_ );
//        THIS_THREAD_SLEEP_SEC( sleep_interval_ );
//        dummy_log_info( CLASS_ID, "handle: RejectResponse: active again, retrying ..." );
        next_state( WAITING_DIALER_FREE );
        break;

    case CANCELLED_IN_WICR:
        callback_consume( create_message_t<DropResponse>( parent_job_id_ ) );
        next_state( DONE );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const voip_service::Dial * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_CONNECTED && state_ != CANCELLED_IN_WC )
    {
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    dummy_log_debug( CLASS_ID, "dialing ..." );
}

void Call::handle( const voip_service::Ring * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_CONNECTED  && state_ != CANCELLED_IN_WC )
    {
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
    }

    dummy_log_debug( CLASS_ID, "ringing ..." );
}

void Call::handle( const voip_service::Connected * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case WAITING_CONNECTED:
        callback_consume( create_message_t<Connected>( parent_job_id_ ) );
        next_state( CONNECTED );
        break;

    case CANCELLED_IN_WC:
        next_state( WRONG_CONNECTED );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const voip_service::Failed * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case WAITING_CONNECTED:
        dummy_log_debug( CLASS_ID, "Failed: code %u, %s", obj->errorcode, obj->descr.c_str() );

        callback_consume( create_failed( parent_job_id_,
                decode_failure_reason( obj->type ), obj->errorcode, obj->descr ) );

        next_state( DONE );
        break;

    case CANCELLED_IN_WC:
        callback_consume( create_message_t<DropResponse>( parent_job_id_ ) );
        next_state( DONE );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const voip_service::ConnectionLost * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case CONNECTED:
    case CONNECTED_BUSY:
        dummy_log_debug( CLASS_ID, "connection lost: code %u, %s", obj->errorcode, obj->descr.c_str() );
        callback_consume( create_connection_lost( parent_job_id_, obj->errorcode, obj->descr ) );
        next_state( DONE );
        break;

    case CANCELLED_IN_C:
    case CANCELLED_IN_CB:
    case WRONG_CONNECTED:
        callback_consume( create_message_t<DropResponse>( parent_job_id_ ) );
        next_state( DONE );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const voip_service::DropResponse * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case CANCELLED_IN_WICR:
        callback_consume( create_message_t<DropResponse>( parent_job_id_ ) );
        next_state( DONE );
        break;

    case WRONG_CONNECTED:
    case CANCELLED_IN_WC:
    case CANCELLED_IN_C:
    case CANCELLED_IN_CB:
        validate_and_reset_response_job_id( obj );
        callback_consume( create_message_t<DropResponse>( parent_job_id_ ) );
        next_state( DONE );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const voip_service::PlayFileResponse * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case CONNECTED_BUSY:
        validate_and_reset_response_job_id( obj );
        callback_consume( create_message_t<PlayFileResponse>( parent_job_id_ ) );
        next_state( CONNECTED );
        break;

    case CANCELLED_IN_CB:
        next_state( CANCELLED_IN_C );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const voip_service::DtmfTone * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case CONNECTED:
    case CONNECTED_BUSY:
    {
        dummy_log_debug( CLASS_ID, "handle DtmfTone: %u", obj->tone );
        auto tone = decode_tone( obj->tone );
        callback_consume( create_dtmf_tone( parent_job_id_, tone ) );
    }
        break;

    case CANCELLED_IN_C:
    case CANCELLED_IN_CB:
    case WRONG_CONNECTED:
        dummy_log_warn( CLASS_ID, "DTMF tone is ignored", to_c_str( state_ ) );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::next_state( state_e state )
{
    state_  = state;

    trace_state_switch();
}

void Call::trace_state_switch() const
{
    dummy_log_debug( CLASS_ID, "switched to %s", to_c_str( state_ ) );
}

void Call::send_error_response( const std::string & descr )
{
    callback_consume( create_error_response( parent_job_id_, descr ) );
}

void Call::validate_and_reset_response_job_id( const voip_service::ResponseObject * resp )
{
    ASSERT( current_req_id_ != 0 );

    ASSERT( current_req_id_ == resp->job_id );

    current_req_id_ = 0;
}

void Call::callback_consume( const CallbackObject * req )
{
    if( callback_ )
        callback_->consume( req );
}

uint32_t Call::get_next_request_id()
{
    static uint32_t id = 0;

    return ++id;
}

Failed::type_e Call::decode_failure_reason( voip_service::Failed::type_e type )
{
    if( type == voip_service::Failed::REFUSED )
        return Failed::REFUSED;
    else if( type == voip_service::Failed::BUSY )
        return Failed::BUSY;

    return Failed::FAILED;
}

dtmf_tools::tone_e Call::decode_tone( voip_service::DtmfTone::tone_e tone )
{
    static const dtmf_tools::tone_e table[] =
    {
        dtmf_tools::tone_e::TONE_0,
        dtmf_tools::tone_e::TONE_1,
        dtmf_tools::tone_e::TONE_2,
        dtmf_tools::tone_e::TONE_3,
        dtmf_tools::tone_e::TONE_4,
        dtmf_tools::tone_e::TONE_5,
        dtmf_tools::tone_e::TONE_6,
        dtmf_tools::tone_e::TONE_7,
        dtmf_tools::tone_e::TONE_8,
        dtmf_tools::tone_e::TONE_9,
        dtmf_tools::tone_e::TONE_A,
        dtmf_tools::tone_e::TONE_B,
        dtmf_tools::tone_e::TONE_C,
        dtmf_tools::tone_e::TONE_D,
        dtmf_tools::tone_e::TONE_STAR,
        dtmf_tools::tone_e::TONE_HASH
    };

    if(
            tone >= voip_service::DtmfTone::tone_e::TONE_0 &&
            tone <= voip_service::DtmfTone::tone_e::TONE_HASH )
    {
        return table[ static_cast<uint16_t>( tone ) ];
    }

    return dtmf_tools::tone_e::TONE_A;
}

NAMESPACE_CALMAN_END
