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

// $Id: async_proxy.h 544 2014-05-15 17:26:37Z serge $

#ifndef CALMAN_ASYNC_PROXY_H
#define CALMAN_ASYNC_PROXY_H

#include <string>                   // std::string
#include <list>
#include <boost/thread.hpp>         // boost::mutex

#include "../utils/types.h"         // uint32

#include "namespace_calman.h"       // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

class IEvent
{
public:
    virtual ~IEvent() {}

    virtual void invoke()   = 0;
};

typedef boost::shared_ptr< IEvent > IEventPtr;

template< class CLOSURE >
class Event: public virtual IEvent
{
public:

    Event( const CLOSURE & closure ):
            closure( closure )
    {
    }

    void invoke()
    {
        closure();
    }

public:
    CLOSURE     closure;
};

template< class CLOSURE >
inline Event<CLOSURE> *new_event( const CLOSURE &closure )
{
    return new Event<CLOSURE>( closure );
}


class AsyncProxy
{
public:
    struct Config
    {
        uint32  sleep_time_ms;  // try 1 ms
    };

public:
    AsyncProxy();
    ~AsyncProxy();

    bool init( const Config & cfg );
    void thread_func();

    bool add_event( IEventPtr event );
    bool remove_event( IEventPtr event );

private:

    bool has_events() const;
    void check_queue();

private:

    typedef std::list< IEventPtr > EventList;

private:
    mutable boost::mutex    mutex_;

    Config                  cfg_;

    EventList               events_;
};

NAMESPACE_CALMAN_END

#endif  // CALMAN_ASYNC_PROXY_H
