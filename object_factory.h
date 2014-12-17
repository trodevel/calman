/*

Call manager objects.

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


// $Id: object_factory.h 1271 2014-12-16 19:48:03Z serge $

#ifndef CALMAN_OBJECT_FACTORY_H
#define CALMAN_OBJECT_FACTORY_H

#include "objects.h"    // CalmanObject...

NAMESPACE_CALMAN_START

inline void init_job_id( CalmanObject * obj, uint32 job_id )
{
    obj->job_id = job_id;
}

template <class _T>
_T *create_message_t( uint32 job_id )
{
    _T *res = new _T;

    init_job_id( res, job_id );

    return res;
}

inline CalmanCallDuration *create_call_duration( uint32 job_id, uint32 t )
{
    CalmanCallDuration *res = create_message_t<CalmanCallDuration>( job_id );

    res->t  = t;

    return res;
}

inline CalmanError *create_error( uint32 job_id, const std::string & error )
{
    CalmanError *res = create_message_t<CalmanError>( job_id );

    res->error = error;

    return res;
}

NAMESPACE_CALMAN_END

#endif  // CALMAN_OBJECT_FACTORY_H
