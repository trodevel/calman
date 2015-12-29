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


// $Revision: 3074 $ $Date:: 2015-12-29 #$ $Author: serge $

#ifndef CALMAN_OBJECT_FACTORY_H
#define CALMAN_OBJECT_FACTORY_H

#include "objects.h"    // Object...

NAMESPACE_CALMAN_START

inline void init_job_id( Object * obj, uint32_t job_id )
{
    obj->job_id = job_id;
}

template <class _T>
_T *create_message_t( uint32_t job_id )
{
    _T *res = new _T;

    init_job_id( res, job_id );

    return res;
}

inline InitiateCallRequest *create_insert_job( uint32_t job_id, const std::string & party )
{
    InitiateCallRequest *res = create_message_t<InitiateCallRequest>( job_id );

    res->party  = party;

    return res;
}


inline ErrorResponse *create_error_response( uint32_t job_id, const std::string & descr )
{
    ErrorResponse *res = new ErrorResponse;

    init_job_id( res, job_id );

    res->descr      = descr;

    return res;
}


inline PlayFile *create_play_file( uint32_t job_id, const std::string & filename )
{
    PlayFile *res = create_message_t<PlayFile>( job_id );

    res->filename   = filename;

    return res;
}

inline CallDuration *create_call_duration( uint32_t job_id, uint32_t t )
{
    CallDuration *res = create_message_t<CallDuration>( job_id );

    res->t  = t;

    return res;
}

inline Failed *create_failed( uint32_t job_id, uint32_t errorcode, const std::string & descr )
{
    auto res = create_message_t<Failed>( job_id );

    res->errorcode  = errorcode;
    res->descr      = descr;

    return res;
}

inline ConnectionLost *create_connection_lost( uint32_t job_id, uint32_t errorcode, const std::string & descr )
{
    auto res = create_message_t<ConnectionLost>( job_id );

    res->errorcode  = errorcode;
    res->descr      = descr;

    return res;
}

NAMESPACE_CALMAN_END

#endif  // CALMAN_OBJECT_FACTORY_H
