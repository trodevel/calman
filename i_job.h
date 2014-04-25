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


// $Id: i_job.h 448 2014-04-25 17:26:21Z serge $

#ifndef CALMAN_I_JOB_H
#define CALMAN_I_JOB_H

#include <string>                   // std::string
#include "../utils/Types.h"         // uint32

#include "namespace_calman.h"       // NAMESPACE_CALMAN_START

namespace dialer
{
class CallI;
}

NAMESPACE_CALMAN_START

class IJobCallback;

class IJob
{
public:

    enum status_e
    {
        UNDEF   = 0,
        WAITING,
        ACTIVE,
        DONE
    };

public:
    virtual ~IJob() {};

    virtual uint32 get_id() const                   = 0;
    virtual status_e get_status() const             = 0;
    virtual bool cancel()                           = 0;
    virtual bool is_alive() const                   = 0;
    virtual dialer::CallI* get_call()               = 0;
    virtual bool register_callback( IJobCallback * cb ) = 0;
};

NAMESPACE_CALMAN_END

#endif  // CALMAN_I_JOB_H
