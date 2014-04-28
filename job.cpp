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


// $Id: job.cpp 459 2014-04-28 16:59:39Z serge $

#include "job.h"                    // self

#include "call_manager.h"           // CallManager

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log

NAMESPACE_CALMAN_START

#define MODULENAME      "Job"

Job::Job( const std::string & party, const std::string & scen ):
        state_( UNDEF ), call_( 0L ), party_( party ), scen_( scen )
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

void Job::on_activate()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == IDLE )
    {
        state_ = ACTIVE;
    }
    else
    {
        dummy_log( 0, MODULENAME, "on_activate: ignored in state %u", state_ );
    }
}
void Job::on_call_ready( dialer::CallI* call )
{
    SCOPE_LOCK( mutex_ );

    if( state_ == ACTIVE )
    {
        call_ = call;
    }
    else
    {
        dummy_log( 0, MODULENAME, "on_call_ready: ignored in state %u", state_ );
    }
}
void Job::on_error( uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    if( state_ == ACTIVE || state_ == IDLE )
    {
        state_ = DONE;
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
    }
    else
    {
        dummy_log( 0, MODULENAME, "on_error: ignored in state %u", state_ );
    }
}

NAMESPACE_CALMAN_END
