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


// $Id: call_manager.cpp 453 2014-04-25 18:03:03Z serge $

#include "call_manager.h"                 // self

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../dialer/i_dialer.h"         // IDialer

#include "job.h"                        // Job


NAMESPACE_CALMAN_START

CallManager::CallManager():
    state_( UNDEF ), dialer_( 0L ), last_id_( 0 )
{
}
CallManager::~CallManager()
{
}

bool CallManager::init( dialer::IDialer * dialer )
{
    if( dialer == 0L )
        return false;

    SCOPE_LOCK( mutex_ );

    if( dialer_ != 0L )
        return false;

    dialer_ = dialer;

    return true;
}

// ICallManager interface
calman::IJob* CallManager::create_call_job( const std::string & party )
{
    SCOPE_LOCK( mutex_ );

    last_id_++;

    Job * job   = new Job( last_id_, this );

    jobs_.push_back( job );

    return job;
}
bool CallManager::cancel_job( uint32 id )
{
    SCOPE_LOCK( mutex_ );

    return false;
}
bool CallManager::cancel_job( const IJob * job )
{
    SCOPE_LOCK( mutex_ );

    return false;
}
calman::IJob* CallManager::get_job( uint32 id )
{
    SCOPE_LOCK( mutex_ );

    return 0L;  // TODO implement it e425
}
bool CallManager::shutdown()
{
    SCOPE_LOCK( mutex_ );

    return true;
}

// IDialerCallback interface
void CallManager::on_ready()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == IDLE )
    {
        // TODO error
        return;
    }

    if( state_ == BUSY )
    {
        state_  = IDLE;
    }

    if( jobs_.empty() )
        return; // no jobs to process, exit
}
void CallManager::on_busy()
{
    SCOPE_LOCK( mutex_ );

    if( state_ == BUSY )
    {
        // TODO error
        return;
    }

    if( state_ == IDLE )
    {
        state_  = BUSY;
    }
}
void CallManager::on_error( uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );
}

NAMESPACE_CALMAN_END
