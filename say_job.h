/*

SayJob.

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


// $Id: say_job.h 522 2014-05-08 17:02:03Z serge $

#ifndef CALMAN_SAY_JOB_H
#define CALMAN_SAY_JOB_H

#include "job.h"                    // Job

namespace gspeak
{
class GSpeak;
}

NAMESPACE_CALMAN_START

class CallManager;

class SayJob: virtual public Job
{
public:
    SayJob( const std::string & party, gspeak::GSpeak * gs, const std::string & text, uint32 start_delay = 0 );
    ~SayJob();

protected:
    // virtual functions for overloading
    void on_custom_activate();
    void on_custom_finished();

private:
    mutable boost::mutex    mutex_;

    gspeak::GSpeak          * gs_;

    std::string             text_;

    uint32                  start_delay_;
};

NAMESPACE_CALMAN_END

#endif  // CALMAN_SAY_JOB_H
