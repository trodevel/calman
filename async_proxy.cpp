/*

Async Proxy.

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

// $Id: async_proxy.cpp 544 2014-05-15 17:26:37Z serge $

#include "async_proxy.h"                // self

#include <algorithm>                    // std::find
#include <boost/bind.hpp>               // boost::bind

#include "../utils/assert.h"            // ASSERT
#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log

#define MODULENAME      "AsyncProxy"

NAMESPACE_CALMAN_START

AsyncProxy::AsyncProxy()
{
}

AsyncProxy::~AsyncProxy()
{
}

bool AsyncProxy::init( const Config & cfg )
{
    SCOPE_LOCK( mutex_ );

    dummy_log( 0, MODULENAME, "init()" );

    cfg_    = cfg;

    return true;
}

void AsyncProxy::thread_func()
{
    dummy_log( 0, MODULENAME, "thread_func: started" );

    bool should_run    = true;
    while( should_run )
    {
        {
            SCOPE_LOCK( mutex_ );

            if( has_events() )
            {
                check_queue();
            }
        }

        THREAD_SLEEP_MS( cfg_.sleep_time_ms );
    }

    dummy_log( 0, MODULENAME, "thread_func: ended" );
}

bool AsyncProxy::has_events() const
{
    // private: no MUTEX lock
    if( events_.empty() )
        return false;

    return true;
}

void AsyncProxy::check_queue()
{
    // private: no MUTEX lock
}

bool AsyncProxy::add_event( IEventPtr event )
{
    SCOPE_LOCK( mutex_ );

    if( std::find( events_.begin(), events_.end(), event ) != events_.end() )
    {
        dummy_log( 0, MODULENAME, "add_event: event %p already exists", event.get() );

        return false;
    }

    events_.push_back( event );

    dummy_log( 0, MODULENAME, "add_event: added event %p", event.get() );

    return true;
}

bool AsyncProxy::remove_event( IEventPtr event )
{
    SCOPE_LOCK( mutex_ );

    EventList::iterator it = std::find( events_.begin(), events_.end(), event );

    if( it == events_.end() )
    {
        dummy_log( 0, MODULENAME, "remove_event: cannot remove - event %p not found", event.get() );

        return false;
    }

    events_.erase( it );

    dummy_log( 0, MODULENAME, "remove_event: removed event %p", event.get() );

    return true;
}

NAMESPACE_CALMAN_END
