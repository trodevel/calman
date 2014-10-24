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


// $Id: job.h 1202 2014-10-24 20:00:23Z serge $

#ifndef CALMAN_JOB_H
#define CALMAN_JOB_H

#include <string>                   // std::string
#include <boost/thread.hpp>         // boost::mutex

#include "i_job.h"                  // IJob
#include "../dialer/i_call_callback.h"  // ICallCallback

namespace asyncp
{
class IAsyncProxy;
}

NAMESPACE_CALMAN_START

class CallManager;

class Job: virtual public IJob, virtual public dialer::ICallCallback
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
    Job(
        const std::string       & party,
        asyncp::IAsyncProxy     * proxy );

    ~Job();

    // IJob interface
    virtual std::string get_property( const std::string & name ) const;

    void on_processing_started();
    void on_activate();
    void on_call_obj_available( dialer::CallIPtr call );
    void on_error( uint32 errorcode );
    void on_finished();

    // dialer::ICallCallback
    void on_fatal_error( uint32 errorcode );
    void on_call_end( uint32 errorcode );
    void on_dial();
    void on_ring();
    void on_connect();
    void on_call_duration( uint32 t );

protected:
    // virtual functions for overloading
    virtual void on_custom_processing_started();
    virtual void on_custom_activate();
    virtual void on_custom_finished();

private:
    void on_activate___();

private:
    // IJob interface
    void on_processing_started__();
    void on_activate__();
    void on_call_obj_available__( dialer::CallIPtr call );
    void on_error__( uint32 errorcode );
    void on_finished__();

    // dialer::ICallCallback
    void on_fatal_error__( uint32 errorcode );
    void on_connect__();

protected:

    asyncp::IAsyncProxy     * proxy_;

    dialer::CallIPtr        call_;

    std::string             party_;

private:
    mutable boost::mutex    mutex_;

    status_e                state_;
};

NAMESPACE_CALMAN_END

#endif  // CALMAN_JOB_H
