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


// $Id: job.h 458 2014-04-28 16:56:18Z serge $

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
    enum status_e
    {
        UNDEF,
        IDLE,
        ACTIVE,
        DONE
    };

public:
    Job( const std::string & party, const std::string & scen );
    ~Job();

    // IJob interface
    std::string get_property( const std::string & name ) const  = 0;

    virtual void on_activate()                                  = 0;
    virtual void on_call_ready( dialer::CallI* call )           = 0;
    virtual void on_error( uint32 errorcode )                   = 0;
    virtual void on_finished()                                  = 0;

private:
    mutable boost::mutex    mutex_;

    status_e                state_;

    dialer::CallI           * call_;

    std::string             party_;
    std::string             scen_;
};

NAMESPACE_CALMAN_END

#endif  // CALMAN_JOB_H
