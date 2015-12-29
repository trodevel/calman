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


// $Revision: 3074 $ $Date:: 2015-12-29 #$ $Author: serge $

#include "call.h"                       // self

#include "i_call_manager_callback.h"    // ICallManagerCallback
#include "object_factory.h"             // create_message_t

#include "../utils/mutex_helper.h"      // MUTEX_SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../utils/assert.h"            // ASSERT
#include "../voip_io/i_voip_service.h"  // IVoipService
#include "../voip_io/object_factory.h"  // create_play_file_requiest

NAMESPACE_CALMAN_START

#define MODULENAME      "Call"

const char* to_c_str( Call::status_e s )
{
    static const char *vals[]=
    {
            "UNDEF",
            "IDLE",
            "WAITING_DIALER_FREE",
            "WAITING_INITIATE_CALL_RESP",
            "WAITING_CONNECTION",
            "CONNECTED",
            "WAITING_DROP_RESPONSE",
            "DONE"
    };

    if( s < Call::UNDEF || s > Call::DONE )
        return "???";

    return vals[ (int) s ];
}

Call::Call(
        uint32_t                    parent_job_id,
        const std::string           & party,
        ICallManagerCallback        * callback,
        voip_service::IVoipService  * voips ):
        jobman::Job( parent_job_id ),
        party_( party ),
        state_( IDLE ),
        parent_job_id_( parent_job_id ),
        current_req_id_( 0 ),
        call_id_( 0 ),
        callback_( callback ),
        voips_( voips )
{
    ASSERT( parent_job_id );
}

const std::string & Call::get_party() const
{
    MUTEX_SCOPE_LOCK( mutex_ );

    return party_;
}

bool Call::is_completed() const
{
    MUTEX_SCOPE_LOCK( mutex_ );

    return state_ == DONE;
}

void Call::initiate()
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != IDLE )
    {
        dummy_log_fatal( MODULENAME, "initiate: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    callback_consume( create_message_t<InitiateCallResponse>( get_parent_job_id() ) );

    voips_->consume( voip_service::create_initiate_call_request( party_ ) );

    state_  = WAITING_INITIATE_CALL_RESP;

    trace_state_switch();
}

void Call::handle( const PlayFileRequest * req )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != CONNECTED )
    {
        // TODO send reject response, fc29
        dummy_log_fatal( MODULENAME, "handle PlayFileRequest: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    voips_->consume( voip_service::create_play_file_request( get_child_job_id(), req->filename ) );
}

void Call::handle( const DropRequest * req )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != CONNECTED && state_ != WAITING_CONNECTION )
    {
        // TODO send reject response, fc29
        dummy_log_fatal( MODULENAME, "drop: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    voips_->consume( voip_service::create_message_t<voip_service::DropRequest>( get_child_job_id() ) );

    state_  = WAITING_DROP_RESPONSE;
}

void Call::handle( const voip_service::InitiateCallResponse * obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_INITIATE_CALL_RESP )
    {
        dummy_log_fatal( MODULENAME, "handle InitiateCallResponse: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    call_id_    = obj->call_id;
    state_      = WAITING_CONNECTION;

    trace_state_switch();
}

void Call::handle( const voip_service::ErrorResponse * obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ == WAITING_INITIATE_CALL_RESP )
    {
        send_error_response( "failed to initiate call" );
        state_ = DONE;
        trace_state_switch();
    }
    else if( state_ == CONNECTED )
    {

    }
    else if( state_ == WAITING_DROP_RESPONSE )
    {
        send_error_response( "failed to drop call" );
        state_ = DONE;
        trace_state_switch();
    }
    else
    {
        dummy_log_fatal( MODULENAME, "handle ErrorResponse: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }
}

void Call::handle( const voip_service::Dial * obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_CONNECTION )
    {
        dummy_log_fatal( MODULENAME, "handle Dial: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    dummy_log_debug( MODULENAME, "dialing ..." );
}

void Call::handle( const voip_service::Ring * obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_CONNECTION )
    {
        dummy_log_fatal( MODULENAME, "handle Ring: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    dummy_log_debug( MODULENAME, "ringing ..." );
}

void Call::handle( const voip_service::Connected * obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_CONNECTION )
    {
        dummy_log_fatal( MODULENAME, "handle Connected: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    state_ = CONNECTED;

    trace_state_switch();

    callback_consume( create_message_t<Connected>( parent_job_id_ ) );
}

void Call::handle( const voip_service::CallDuration * obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != CONNECTED )
    {
        dummy_log_fatal( MODULENAME, "handle CallDuration: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    dummy_log_debug( MODULENAME, "handle CallDuration: ..." );

    callback_consume( create_call_duration( parent_job_id_, obj->t ) );
}

void Call::handle( const voip_service::Failed * obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_CONNECTION )
    {
        dummy_log_fatal( MODULENAME, "handle Failed: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    dummy_log_debug( MODULENAME, "handle Failed: code %u, %s", obj->errorcode, obj->descr.c_str() );

    state_ = DONE;

    trace_state_switch();

    callback_consume( create_failed( parent_job_id_, obj->errorcode, obj->descr ) );
}

void Call::handle( const voip_service::ConnectionLost * obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != CONNECTED )
    {
        dummy_log_fatal( MODULENAME, "handle ConnectionLost: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    dummy_log_debug( MODULENAME, "handle ConnectionLost: code %u, %s", obj->errorcode, obj->descr.c_str() );

    state_ = DONE;

    trace_state_switch();

    callback_consume( create_connection_lost( parent_job_id_, obj->errorcode, obj->descr ) );
}

void Call::handle( const voip_service::DropResponse * obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_DROP_RESPONSE )
    {
        dummy_log_fatal( MODULENAME, "handle: DialerDropResponse: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    state_ = DONE;

    trace_state_switch();

    callback_consume( create_message_t<DropResponse>( parent_job_id_ ) );
}

void Call::handle( const voip_service::PlayFileResponse * obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != CONNECTED )
    {
        dummy_log_fatal( MODULENAME, "handle: DialerPlayStarted: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    callback_consume( create_message_t<PlayStarted>( parent_job_id_ ) );
}
void Call::handle( const voip_service::DialerPlayStopped * obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != CONNECTED && state_ != WAITING_DROP_RESPONSE )
    {
        dummy_log_fatal( MODULENAME, "handle: DialerPlayStopped: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    if( state_ == WAITING_DROP_RESPONSE )
    {
        dummy_log_warn( MODULENAME, "handle: DialerPlayStopped: arrived too late, ignored in state %s", to_c_str( state_ ) );
    }

    callback_consume( create_message_t<PlayStopped>( parent_job_id_ ) );
}
void Call::handle( const voip_service::DialerPlayFailed * obj )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( state_ != CONNECTED )
    {
        dummy_log_fatal( MODULENAME, "handle: DialerPlayFailed: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    callback_consume( create_message_t<PlayFailed>( parent_job_id_ ) );
}

void Call::trace_state_switch() const
{
    dummy_log_debug( MODULENAME, "switched to %s", to_c_str( state_ ) );
}

void Call::send_error_response( const std::string & descr )
{
    callback_consume( create_error_response( parent_job_id_, descr ) );
}

void Call::callback_consume( const CallbackObject * req )
{
    if( callback_ )
        callback_->consume( req );
}



NAMESPACE_CALMAN_END
