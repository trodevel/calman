/*

Job.

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


// $Id: job.cpp 1217 2014-10-28 18:06:24Z serge $

#include "job.h"                    // self

#include "call_manager.h"           // CallManager

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../asyncp/async_proxy.h"      // AsyncProxy
#include "../asyncp/event.h"            // new_event
#include "../utils/assert.h"            // ASSERT

NAMESPACE_CALMAN_START

#define MODULENAME      "Job"

Job::Job(
        const std::string       & party,
        asyncp::IAsyncProxy     * proxy ):
        proxy_( proxy ), call_( 0L ), party_( party ), state_( IDLE )
{
}
Job::~Job()
{
}

// IJob interface
std::string Job::get_property( const std::string & name ) const
{
    SCOPE_LOCK( mutex_ );

    static const std::string empty;

    if( name == "party" )
        return party_;

    return empty;
}

void Job::on_processing_started()
{
    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &Job::on_processing_started__, this ) ) ) );
}
void Job::on_processing_started__()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == IDLE )
    {
        dummy_log_debug( MODULENAME, "on_processing_started: switched to PREPARING" );
        state_ = PREPARING;

        on_custom_processing_started();
    }
    else
    {
        dummy_log_fatal( MODULENAME, "on_processing_started: unexpected in state %u", state_ );

        ASSERT( 0 );
    }
}

/**
 * @brief on_call_started() is called when the connection is established
 */

void Job::on_call_started()
{
    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &Job::on_call_started__, this ) ) ) );
}

void Job::on_call_started__()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == PREPARING )
    {
        state_ = ACTIVE;
        dummy_log_debug( MODULENAME, "on_call_started: switched to ACTIVE" );

        on_custom_activate();
    }
    else
    {
        dummy_log_fatal( MODULENAME, "on_call_started: unexpected in state %u", state_ );

        ASSERT( 0 );
    }
}
void Job::on_call_obj_available( dialer::CallIPtr call )
{
    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &Job::on_call_obj_available__, this, call ) ) ) );
}
void Job::on_call_obj_available__( dialer::CallIPtr call )
{
    SCOPE_LOCK( mutex_ );

    if( state_ == PREPARING )
    {
        call_ = call;
        dummy_log_debug( MODULENAME, "on_call_obj_available: got CallI" );
    }
    else
    {
        dummy_log_fatal( MODULENAME, "on_call_obj_available: unexpected in state %u", state_ );

        ASSERT( 0 );
    }
}
void Job::on_error( uint32 errorcode )
{
    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &Job::on_error__, this, errorcode ) ) ) );
}
void Job::on_error__( uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    if( state_ == PREPARING || state_ == ACTIVE || state_ == IDLE )
    {
        state_ = DONE;

        dummy_log_debug( MODULENAME, "on_error: switched to DONE" );
    }
    else
    {
        dummy_log_fatal( MODULENAME, "on_error: unexpected in state %u", state_ );

        ASSERT( 0 );
    }
}
void Job::on_finished()
{
    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &Job::on_finished__, this ) ) ) );
}
void Job::on_finished__()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == ACTIVE )
    {
        state_ = DONE;

        dummy_log_debug( MODULENAME, "on_finished: switched to DONE" );

        on_custom_finished();
    }
    else
    {
        dummy_log_fatal( MODULENAME, "on_finished: unexpected in state %u", state_ );

        ASSERT( 0 );
    }
}


// virtual functions for overloading
void Job::on_custom_processing_started()
{
}

void Job::on_custom_activate()
{
}

void Job::on_custom_finished()
{
}

NAMESPACE_CALMAN_END
