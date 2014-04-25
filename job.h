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


// $Id: job.h 448 2014-04-25 17:26:21Z serge $

#ifndef CALMAN_JOB_H
#define CALMAN_JOB_H

#include <string>                   // std::string
#include <boost/thread.hpp>         // boost::mutex


#include "i_job.h"                  // Job

NAMESPACE_CALMAN_START

class CallManager;

class Job: public IJob
{
public:

public:
    Job( uint32 id, CallManager * parent );
    ~Job();

    // IJob interface
    uint32 get_id() const;
    IJob::status_e get_status() const;
    bool cancel();
    bool is_alive() const;
    dialer::CallI* get_call();
    bool register_callback( IJobCallback * cb );

private:
    mutable boost::mutex    mutex_;

    status_e                status_;

    uint32                  id_;

    dialer::CallI           * call_;

    CallManager             * parent_;

    IJobCallback            * cb_;
};

NAMESPACE_CALMAN_END

#endif  // CALMAN_JOB_H
