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


// $Id: call.cpp 1250 2014-12-04 18:48:26Z serge $

#include "call.h"                       // self

#include "i_call_manager_callback.h"    // ICallManagerCallback

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../utils/assert.h"            // ASSERT

NAMESPACE_CALMAN_START

#define MODULENAME      "Call"

Call::Call(
        uint32                  parent_job_id,
        const std::string       & party,
        ICallManagerCallback    * callback ):
        parent_job_id_( parent_job_id ),
        call_id_( 0 ),
        party_( party ),
        state_( IDLE ),
        callback_( callback )
{
    ASSERT( parent_job_id );
}

const std::string & Call::get_party() const
{
    SCOPE_LOCK( mutex_ );

    return party_;
}

uint32 Call::get_parent_job_id() const
{
    SCOPE_LOCK( mutex_ );

    return parent_job_id_;
}

uint32 Call::get_call_id() const
{
    SCOPE_LOCK( mutex_ );

    return call_id_;
}

void Call::set_call_id( uint32 call_id )
{
    SCOPE_LOCK( mutex_ );

    call_id_    = call_id;
}

void Call::on_dial()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == IDLE )
    {
        dummy_log_debug( MODULENAME, "on_dial: switched to PREPARING" );
        state_ = PREPARING;
    }
    else
    {
        dummy_log_fatal( MODULENAME, "on_dial: unexpected in state %u", state_ );
        ASSERT( 0 );
    }
}
void Call::on_ring()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == PREPARING )
    {
        dummy_log_debug( MODULENAME, "on_ring: ..." );
    }
    else
    {
        dummy_log_fatal( MODULENAME, "on_ring: unexpected in state %u", state_ );
        ASSERT( 0 );
    }
}
void Call::on_call_started()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == PREPARING )
    {
        dummy_log_debug( MODULENAME, "on_call_started: switched to ACTIVE" );

        state_ = ACTIVE;

        if( callback_ )
            callback_->on_call_started( parent_job_id_ );
    }
    else
    {
        dummy_log_fatal( MODULENAME, "on_call_started: unexpected in state %u", state_ );
        ASSERT( 0 );
    }
}
void Call::on_call_duration( uint32 t )
{
    SCOPE_LOCK( mutex_ );

    if( state_ == ACTIVE )
    {
        dummy_log_debug( MODULENAME, "on_call_duration: ..." );

        if( callback_ )
            callback_->on_call_duration( parent_job_id_, t );
    }
    else
    {
        dummy_log_fatal( MODULENAME, "on_call_started: unexpected in state %u", state_ );
        ASSERT( 0 );
    }
}
void Call::on_call_end( uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    if( state_ == ACTIVE )
    {
        dummy_log_debug( MODULENAME, "on_call_end: code %u, switched to DONE", errorcode );

        state_ = DONE;

        if( callback_ )
            callback_->on_finished( parent_job_id_ );
    }
    else
    {
        dummy_log_fatal( MODULENAME, "on_call_end: unexpected in state %u", state_ );
        ASSERT( 0 );
    }
}

void Call::on_error( uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    if( state_ == PREPARING || state_ == ACTIVE )
    {
        dummy_log_debug( MODULENAME, "on_error: code %u, switched to DONE", errorcode );

        state_ = DONE;

        if( callback_ )
            callback_->on_error( parent_job_id_, errorcode );
    }
    else
    {
        dummy_log_fatal( MODULENAME, "on_error: unexpected in state %u", state_ );
        ASSERT( 0 );
    }
}
void Call::on_fatal_error( uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    if( state_ == PREPARING || state_ == ACTIVE )
    {
        dummy_log_debug( MODULENAME, "on_fatal_error: code %u, switched to DONE", errorcode );

        state_ = DONE;

        if( callback_ )
            callback_->on_error( parent_job_id_, errorcode );
    }
    else
    {
        dummy_log_fatal( MODULENAME, "on_fatal_error: unexpected in state %u", state_ );
        ASSERT( 0 );
    }
}

NAMESPACE_CALMAN_END
