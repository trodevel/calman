/*

Event.

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

// $Id: event.h 551 2014-05-16 17:18:34Z serge $

#ifndef CALMAN_EVENT_H
#define CALMAN_EVENT_H

#include "i_async_proxy.h"          // IAsyncProxy, IEvent

#include "namespace_calman.h"       // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

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


NAMESPACE_CALMAN_END

#endif  // CALMAN_EVENT_H
