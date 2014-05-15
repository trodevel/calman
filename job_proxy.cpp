/*

Job Proxy.

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


// $Id: job_proxy.cpp 539 2014-05-14 17:15:36Z serge $

#include "job_proxy.h"                  // self

#include <boost/bind.hpp>               // boost::bind

#include "../utils/assert.h"            // ASSERT
#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log

#define MODULENAME      "JobProxy"

NAMESPACE_CALMAN_START

JobProxy::JobProxy()
{
}

JobProxy::~JobProxy()
{
}

bool JobProxy::init( const Config & cfg )
{
    SCOPE_LOCK( mutex_ );

    dummy_log( 0, MODULENAME, "init()" );

    cfg_    = cfg;

    return true;
}

void JobProxy::thread_func()
{
    dummy_log( 0, MODULENAME, "thread_func: started" );

    bool should_run    = true;
    while( should_run )
    {
        {
            SCOPE_LOCK( mutex_ );

            if( has_job() )
            {
                check_queue();
            }
        }

        THREAD_SLEEP_MS( cfg_.sleep_time_ms );
    }

    dummy_log( 0, MODULENAME, "thread_func: ended" );
}

bool JobProxy::has_job() const
{
    // private: no MUTEX lock
    if( job_ == 0L )
        return false;

    return true;
}

void JobProxy::check_queue()
{
    // private: no MUTEX lock
}

bool JobProxy::register_job( IJobPtr job )
{
    SCOPE_LOCK( mutex_ );

    if( job == job_ )
    {
        dummy_log( 0, MODULENAME, "register_job: job %p already exists", job.get() );

        return false;
    }

    if( job_ != 0L )
    {
        dummy_log( 0, MODULENAME, "register_job: another job %p is already registered", job_.get() );
        return false;
    }

    job_ = job;

    dummy_log( 0, MODULENAME, "register_job: registered job %p", job.get() );

    return true;
}

bool JobProxy::unregister_job( IJobPtr job )
{
    SCOPE_LOCK( mutex_ );

    if( job_ == 0L )
    {
        dummy_log( 0, MODULENAME, "ubregister_job: cannot unregister - no job is registered" );
        return false;
    }

    if( job != job_ )
    {
        dummy_log( 0, MODULENAME, "unregister_job: cannot unregister - another job %p is registered", job.get() );

        return false;
    }

    job_.reset();

    events_.clear();

    dummy_log( 0, MODULENAME, "unregister_job: unregistered job %p", job.get() );

    return true;
}

// IJob interface
std::string JobProxy::get_property( const std::string & name ) const
{
    SCOPE_LOCK( mutex_ );

    static const std::string error( "no job exists" );

    if( has_job() == false )
    {
        return error;
    }

    return job_->get_property( name );
}

void JobProxy::on_preparing()
{
    SCOPE_LOCK( mutex_ );

    if( has_job() == false )
    {
        return;
    }

    IEventPtr ev( new_event( boost::bind( &IJob::on_preparing, job_ ) ) );

    events_.push_back( ev );
}

void JobProxy::on_activate()
{
}

void JobProxy::on_call_ready( dialer::CallIPtr call )
{
}

void JobProxy::on_error( uint32 errorcode )
{
}

void JobProxy::on_finished()
{
}


// dialer::ICallCallback
void JobProxy::on_call_end( uint32 errorcode )
{
}

void JobProxy::on_dial()
{
}

void JobProxy::on_ring()
{
}

void JobProxy::on_connect()
{
}

NAMESPACE_CALMAN_END
