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


// $Revision: 5608 $ $Date:: 2017-01-23 #$ $Author: serge $

#include "call.h"                       // self

#include "object_factory.h"             // create_message_t

#include "../utils/mutex_helper.h"      // MUTEX_SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../utils/assert.h"            // ASSERT
#include "../simple_voip/i_simple_voip.h"  // ISimpleVoip
#include "../simple_voip/i_simple_voip_callback.h"  // ISimpleVoipCallback
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
        const std::string           & party,
        simple_voip::ISimpleVoipCallback        * callback,
        simple_voip::ISimpleVoip    * voips ):
        party_( party ),
        state_( IDLE ),
        current_req_id_( 0 ),
        call_id_( 0 ),
        sleep_interval_( 1 ),
        callback_( callback ),
        voips_( voips )
{
}

bool Call::is_completed() const
{
    MUTEX_SCOPE_LOCK( mutex_ );

    return state_ == DONE;
}

void Call::handle( const simple_voip::InitiateCallRequest * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != IDLE )
    {
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", "initiate()", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    set_current_job_id( obj->job_id );

    voips_->consume( obj );

    next_state( WAITING_INITIATE_CALL_RESP );
}

void Call::handle( const simple_voip::PlayFileRequest * obj )
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
        callback_consume( simple_voip::create_reject_response( obj->job_id, 0,
                "cannot process request, busy with processing request " + std::to_string( current_req_id_ ) ) );

        delete obj;
        return;
    }

    set_current_job_id( obj->job_id );

    voips_->consume( obj );

    next_state( CONNECTED_BUSY );
}

void Call::handle( const simple_voip::DropRequest * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case IDLE:
    case WAITING_DIALER_FREE:

        callback_consume( simple_voip::create_drop_response( obj->job_id ) );
        delete obj;
        next_state( DONE );
        break;

    case WAITING_INITIATE_CALL_RESP:
        set_current_job_id( obj->job_id );
        delete obj;
        next_state( CANCELLED_IN_WICR );
        break;

    case WAITING_CONNECTED:
        set_current_job_id( obj->job_id );
        voips_->consume( obj );
        next_state( CANCELLED_IN_WC );
        break;

    case CONNECTED:
        set_current_job_id( obj->job_id );
        voips_->consume( obj );
        next_state( CANCELLED_IN_C );
        break;

    case CONNECTED_BUSY:
        set_current_job_id( obj->job_id );
        voips_->consume( obj );
        next_state( CANCELLED_IN_CB );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const simple_voip::InitiateCallResponse * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case WAITING_INITIATE_CALL_RESP:
        validate_and_reset_response_job_id( obj );
        callback_consume( obj );
        call_id_    = obj->call_id;
        next_state( WAITING_CONNECTED );
        break;

    case CANCELLED_IN_WICR:
        call_id_        = obj->call_id;
        voips_->consume( simple_voip::create_drop_request( get_current_job_id_and_invalidate(), call_id_ ) );
        next_state( CANCELLED_IN_WC );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const simple_voip::ErrorResponse * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case WAITING_INITIATE_CALL_RESP:
        callback_consume( obj );
        next_state( DONE );
        break;

    case CANCELLED_IN_WICR:
        callback_consume( obj );
        next_state( DONE );
        break;

    case CANCELLED_IN_CB:
        delete obj;
        next_state( CANCELLED_IN_C );
        break;

    case CONNECTED_BUSY:
        validate_and_reset_response_job_id( obj );
        callback_consume( obj );
        next_state( CONNECTED );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const simple_voip::RejectResponse * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case WAITING_INITIATE_CALL_RESP:
        validate_and_reset_response_job_id( obj );
        callback_consume( obj );
        next_state( DONE );
        break;

    case CANCELLED_IN_WICR:
        callback_consume( obj );
        next_state( DONE );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const simple_voip::Dialing * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_CONNECTED && state_ != CANCELLED_IN_WC )
    {
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        delete obj;
        return;
    }

    dummy_log_debug( CLASS_ID, "dialing ..." );

    delete obj;
}

void Call::handle( const simple_voip::Ringing * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_CONNECTED  && state_ != CANCELLED_IN_WC )
    {
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        delete obj;
        return;
    }

    dummy_log_debug( CLASS_ID, "ringing ..." );

    delete obj;
}

void Call::handle( const simple_voip::Connected * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case WAITING_CONNECTED:
        callback_consume( obj );
        next_state( CONNECTED );
        break;

    case CANCELLED_IN_WC:
        delete obj;
        next_state( WRONG_CONNECTED );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const simple_voip::Failed * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case WAITING_CONNECTED:
        dummy_log_debug( CLASS_ID, "Failed: %s", obj->descr.c_str() );

        callback_consume( obj );

        next_state( DONE );
        break;

    case CANCELLED_IN_WC:
        delete obj;
        callback_consume( simple_voip::create_drop_response( get_current_job_id_and_invalidate() ) );
        next_state( DONE );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const simple_voip::ConnectionLost * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case CONNECTED:
    case CONNECTED_BUSY:
        dummy_log_debug( CLASS_ID, "connection lost: %s", obj->descr.c_str() );
        callback_consume( obj );
        next_state( DONE );
        break;

    case CANCELLED_IN_C:
    case CANCELLED_IN_CB:
    case WRONG_CONNECTED:
        delete obj;
        callback_consume( simple_voip::create_drop_response( get_current_job_id_and_invalidate() ) );
        next_state( DONE );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const simple_voip::DropResponse * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case CANCELLED_IN_WICR:
        callback_consume( obj );
        next_state( DONE );
        break;

    case WRONG_CONNECTED:
    case CANCELLED_IN_WC:
    case CANCELLED_IN_C:
    case CANCELLED_IN_CB:
        validate_and_reset_response_job_id( obj );
        callback_consume( obj );
        next_state( DONE );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const simple_voip::PlayFileResponse * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case CONNECTED_BUSY:
        validate_and_reset_response_job_id( obj );
        callback_consume( obj );
        next_state( CONNECTED );
        break;

    case CANCELLED_IN_CB:
        delete obj;
        next_state( CANCELLED_IN_C );
        break;

    default:
        dummy_log_fatal( CLASS_ID, "%s is unexpected in state %s", typeid( *obj ).name(), to_c_str( state_ ) );
        ASSERT( 0 );
        break;
    }
}

void Call::handle( const simple_voip::DtmfTone * obj )
{
    dummy_log_trace( CLASS_ID, "handle(): %s", typeid( *obj ).name() );

    MUTEX_SCOPE_LOCK( mutex_ );

    switch( state_ )
    {
    case CONNECTED:
    case CONNECTED_BUSY:
    {
        dummy_log_debug( CLASS_ID, "handle DtmfTone: %u", obj->tone );
        callback_consume( obj );
    }
        break;

    case CANCELLED_IN_C:
    case CANCELLED_IN_CB:
    case WRONG_CONNECTED:
        delete obj;
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

void Call::set_current_job_id( uint32_t job_id )
{
    ASSERT( current_req_id_ == 0 );

    current_req_id_ = job_id;
}

uint32_t Call::get_current_job_id_and_invalidate()
{
    ASSERT( current_req_id_ != 0 );

    auto res = current_req_id_;

    current_req_id_ = 0;

    return res;
}

void Call::validate_and_reset_response_job_id( const simple_voip::ResponseObject * resp )
{
    ASSERT( current_req_id_ != 0 );

    ASSERT( current_req_id_ == resp->job_id );

    current_req_id_ = 0;
}

void Call::callback_consume( const simple_voip::CallbackObject * req )
{
    if( callback_ )
        callback_->consume( req );
}

dtmf_tools::tone_e Call::decode_tone( simple_voip::DtmfTone::tone_e tone )
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
            tone >= simple_voip::DtmfTone::tone_e::TONE_0 &&
            tone <= simple_voip::DtmfTone::tone_e::TONE_HASH )
    {
        return table[ static_cast<uint16_t>( tone ) ];
    }

    return dtmf_tools::tone_e::TONE_A;
}

NAMESPACE_CALMAN_END
