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


// $Id: job.cpp 448 2014-04-25 17:26:21Z serge $

#include "job.h"                    // self

#include "call_manager.h"           // CallManager

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK

NAMESPACE_CALMAN_START

Job::Job( uint32 id, CallManager * parent ):
        status_( UNDEF ), id_( id ), call_( 0L ), parent_( parent ), cb_( 0L )
{
}
Job::~Job()
{
}

uint32 Job::get_id() const
{
    SCOPE_LOCK( mutex_ );
    return id_;
}
IJob::status_e Job::get_status() const
{
    SCOPE_LOCK( mutex_ );
    return status_;
}
bool Job::cancel()
{
    SCOPE_LOCK( mutex_ );
    return parent_->cancel_job( id_ );
}
bool Job::is_alive() const
{
    SCOPE_LOCK( mutex_ );
    return ( status_ == WAITING ) || ( status_ == ACTIVE );
}
dialer::CallI* Job::get_call()
{
    SCOPE_LOCK( mutex_ );
    return call_;
}
bool Job::register_callback( IJobCallback * cb )
{
    if( cb == 0L )
        return false;

    SCOPE_LOCK( mutex_ );

    if( cb_ != 0L )
        return false;

    cb_ = cb;

    return true;
}

NAMESPACE_CALMAN_END
