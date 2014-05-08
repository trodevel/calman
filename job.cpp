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


// $Id: job.cpp 516 2014-05-08 06:15:46Z serge $

#include "job.h"                    // self

#include "call_manager.h"           // CallManager

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log

NAMESPACE_CALMAN_START

#define MODULENAME      "Job"

Job::Job( const std::string & party, const std::string & scen ):
        state_( IDLE ), call_( 0L ), party_( party ), scen_( scen )
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

    if( name == "scen" )
        return scen_;

    return empty;
}

void Job::on_preparing()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == IDLE )
    {
        dummy_log( 0, MODULENAME, "on_preparing: switched to PREPARING" );
        state_ = PREPARING;
    }
    else
    {
        dummy_log( 0, MODULENAME, "on_preparing: ignored in state %u", state_ );
    }
}

/**
 * @brief on_activate() is called when the connection is established
 */
void Job::on_activate()
{
    SCOPE_LOCK( mutex_ );

    on_activate__();
}

void Job::on_activate__()
{
    if( state_ == PREPARING )
    {
        state_ = ACTIVE;
        dummy_log( 0, MODULENAME, "on_activate: switched to ACTIVE" );
    }
    else
    {
        dummy_log( 0, MODULENAME, "on_activate: ignored in state %u", state_ );
    }
}
void Job::on_call_ready( dialer::CallIPtr call )
{
    SCOPE_LOCK( mutex_ );

    if( state_ == ACTIVE )
    {
        call_ = call;
        dummy_log( 0, MODULENAME, "on_call_ready: switched to ACTIVE" );
    }
    else
    {
        dummy_log( 0, MODULENAME, "on_call_ready: ignored in state %u", state_ );
    }
}
void Job::on_error( uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    if( state_ == PREPARING || state_ == ACTIVE || state_ == IDLE )
    {
        state_ = DONE;

        dummy_log( 0, MODULENAME, "on_error: switched to DONE" );
    }
    else
    {
        dummy_log( 0, MODULENAME, "on_error: ignored in state %u", state_ );
    }
}
void Job::on_finished()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == ACTIVE )
    {
        state_ = DONE;

        dummy_log( 0, MODULENAME, "on_finished: switched to DONE" );
    }
    else
    {
        dummy_log( 0, MODULENAME, "on_finished: ignored in state %u", state_ );
    }
}


// dialer::ICallCallback
void Job::on_call_end( uint32 errorcode )
{
}
void Job::on_dial()
{
}
void Job::on_ring()
{
}
void Job::on_connect()
{
    SCOPE_LOCK( mutex_ );

    on_activate__();
}

NAMESPACE_CALMAN_END
