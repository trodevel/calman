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


// $Revision: 1496 $ $Date:: 2015-02-18 #$ $Author: serge $

#include "call.h"                       // self

#include "i_call_manager_callback.h"    // ICallManagerCallback
#include "object_factory.h"             // create_message_t

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../utils/assert.h"            // ASSERT
#include "../dialer/i_dialer.h"         // IDialer
#include "../dialer/object_factory.h"   // create_play_file

NAMESPACE_CALMAN_START

#define MODULENAME      "Call"

const char* to_c_str( Call::status_e s )
{
    static const char *vals[]=
    {
            "UNDEF",
            "IDLE",
            "DIALLING",
            "ACTIVE",
            "WAITING_DROP_RESPONSE",
            "DONE"
    };

    if( s < Call::UNDEF || s > Call::DONE )
        return "???";

    return vals[ (int) s ];
}

Call::Call(
        uint32                  parent_job_id,
        const std::string       & party,
        ICallManagerCallback    * callback,
        dialer::IDialer         * dialer ):
        jobman::Job( parent_job_id ),
        party_( party ),
        state_( IDLE ),
        callback_( callback ),
        dialer_( dialer )
{
    ASSERT( parent_job_id );
}

const std::string & Call::get_party() const
{
    SCOPE_LOCK( mutex_ );

    return party_;
}

void Call::handle( const CalmanPlayFile * req )
{
    SCOPE_LOCK( mutex_ );

    dialer_->consume( dialer::create_play_file( get_child_job_id(), req->filename ) );
}

void Call::handle( const CalmanDrop * req )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ACTIVE )
    {
        dummy_log_fatal( MODULENAME, "drop: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    dialer_->consume( dialer::create_message_t<dialer::DialerDrop>( get_child_job_id() ) );

    state_  = WAITING_DROP_RESPONSE;
}

void Call::handle( const dialer::DialerErrorResponse * obj )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != IDLE )
    {
        dummy_log_fatal( MODULENAME, "handle DialerErrorResponse: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    state_ = DONE;
    dummy_log_debug( MODULENAME, "switched to %s", to_c_str( state_ ) );

    if( callback_ )
        callback_->consume( create_finished_by_other_party( parent_job_id_, obj->errorcode, obj->descr ) );
}

void Call::handle( const dialer::DialerDial * obj )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != IDLE && state_ != DIALLING )
    {
        dummy_log_fatal( MODULENAME, "on_dial: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    state_ = DIALLING;
    dummy_log_debug( MODULENAME, "switched to %s", to_c_str( state_ ) );

}

void Call::handle( const dialer::DialerRing * obj )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != DIALLING )
    {
        dummy_log_fatal( MODULENAME, "on_ring: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    dummy_log_debug( MODULENAME, "on_ring: ..." );
}

void Call::handle( const dialer::DialerConnect * obj )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != DIALLING )
    {
        dummy_log_fatal( MODULENAME, "on_call_started: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    state_ = ACTIVE;

    dummy_log_debug( MODULENAME, "switched to %s", to_c_str( state_ ) );

    if( callback_ )
        callback_->consume( create_message_t<CalmanCallStarted>( parent_job_id_ ) );
}

void Call::handle( const dialer::DialerCallDuration * obj )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ACTIVE )
    {
        dummy_log_fatal( MODULENAME, "on_call_started: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    dummy_log_debug( MODULENAME, "on_call_duration: ..." );

    if( callback_ )
        callback_->consume( create_call_duration( parent_job_id_, obj->t ) );
}

void Call::handle( const dialer::DialerCallEnd * obj )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ACTIVE && state_ != DIALLING )
    {
        dummy_log_fatal( MODULENAME, "on_call_end: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    dummy_log_debug( MODULENAME, "on_call_end: code %u", obj->errorcode );

    state_ = DONE;

    dummy_log_debug( MODULENAME, "switched to %s", to_c_str( state_ ) );

    if( callback_ )
        callback_->consume( create_finished_by_other_party( parent_job_id_, obj->errorcode, obj->descr ) );
}

void Call::handle( const dialer::DialerDropResponse * obj )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != WAITING_DROP_RESPONSE )
    {
        dummy_log_fatal( MODULENAME, "handle: DialerDropResponse: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
    }

    state_ = DONE;

    dummy_log_debug( MODULENAME, "switched to %s", to_c_str( state_ ) );

    if( callback_ )
        callback_->consume( create_message_t<CalmanDropResponse>( parent_job_id_ ) );
}

void Call::handle( const dialer::DialerPlayStarted * obj )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ACTIVE )
    {
        dummy_log_fatal( MODULENAME, "handle: DialerPlayStarted: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    callback_->consume( create_message_t<CalmanPlayStarted>( parent_job_id_ ) );
}
void Call::handle( const dialer::DialerPlayStopped * obj )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ACTIVE )
    {
        dummy_log_fatal( MODULENAME, "handle: DialerPlayStopped: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    callback_->consume( create_message_t<CalmanPlayStopped>( parent_job_id_ ) );
}
void Call::handle( const dialer::DialerPlayFailed * obj )
{
    SCOPE_LOCK( mutex_ );

    if( state_ != ACTIVE )
    {
        dummy_log_fatal( MODULENAME, "handle: DialerPlayFailed: unexpected in state %s", to_c_str( state_ ) );
        ASSERT( 0 );
        return;
    }

    callback_->consume( create_message_t<CalmanPlayFailed>( parent_job_id_ ) );
}


NAMESPACE_CALMAN_END
