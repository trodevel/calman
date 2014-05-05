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


// $Id: i_job.h 506 2014-05-05 17:19:48Z serge $

#ifndef CALMAN_I_JOB_H
#define CALMAN_I_JOB_H

#include <string>                   // std::string
#include "../dialer/call_i.h"       // CallIPtr
#include "../utils/types.h"         // uint32

#include "namespace_calman.h"       // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

class IJob
{
public:

    virtual ~IJob() {};

    virtual std::string get_property( const std::string & name ) const  = 0;

    virtual void on_preparing()                                 = 0;
    virtual void on_activate()                                  = 0;
    virtual void on_call_ready( dialer::CallIPtr call )         = 0;
    virtual void on_error( uint32 errorcode )                   = 0;
    virtual void on_finished()                                  = 0;
};

typedef boost::shared_ptr< IJob >   IJobPtr;

NAMESPACE_CALMAN_END

#endif  // CALMAN_I_JOB_H
