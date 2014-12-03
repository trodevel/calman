/*

Call.

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


// $Id: call.h 1241 2014-12-02 19:14:46Z serge $

#ifndef CALMAN_CALL_H
#define CALMAN_CALL_H

#include <string>                   // std::string
#include <boost/thread.hpp>         // boost::mutex
#include "../utils/types.h"         // uint32

#include "namespace_lib.h"          // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

class ICallManagerCallback;

class Call
{
public:
    enum status_e
    {
        UNDEF,
        IDLE,
        PREPARING,
        ACTIVE,
        DONE
    };

public:
    Call(
        uint32                  parent_job_id,
        const std::string       & party,
        ICallManagerCallback    * callback );

    const std::string & get_party() const;
    uint32 get_parent_job_id() const;
    uint32 get_call_id() const;

    void set_call_id( uint32 call_id );

    // partly interface IDialerCallback
    // not needed: void on_call_initiate_response( uint32 call_id, uint32 status );
    // not needed: void on_error_response( uint32 error, const std::string & descr );
    void on_dial();
    void on_ring();
    void on_call_started();
    void on_call_duration( uint32 t );
    void on_call_end( uint32 errorcode );
    // not needed: void on_ready();
    void on_error( uint32 errorcode );
    void on_fatal_error( uint32 errorcode );

private:

    mutable boost::mutex    mutex_;

    uint32                  parent_job_id_;

    uint32                  call_id_;

    std::string             party_;

    status_e                state_;

    ICallManagerCallback    * callback_;
};

typedef boost::shared_ptr< Call >   CallPtr;

NAMESPACE_CALMAN_END

#endif  // CALMAN_CALL_H
