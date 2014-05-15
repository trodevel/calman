/*

Job Proxy.

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


// $Id: job_proxy.h 538 2014-05-14 17:14:38Z serge $

#ifndef CALMAN_JOB_PROXY_H
#define CALMAN_JOB_PROXY_H

#include <string>                   // std::string
#include <list>
#include <boost/thread.hpp>         // boost::mutex

#include "i_job.h"                  // IJob
#include "../dialer/i_call_callback.h"  // ICallCallback

NAMESPACE_CALMAN_START

class CallManager;

class JobProxy: virtual public IJob, virtual public dialer::ICallCallback
{
public:
    struct Config
    {
        uint32  sleep_time_ms;  // try 1 ms
    };

public:
    JobProxy();
    ~JobProxy();

    bool init( const Config & cfg );
    void thread_func();

    bool register_job( IJobPtr job );
    bool unregister_job( IJobPtr job );

    // IJob interface - synchronous implementation
    virtual std::string get_property( const std::string & name ) const;

    // IJob interface - asynchronous implementation
    void on_preparing();
    void on_activate();
    void on_call_ready( dialer::CallIPtr call );
    void on_error( uint32 errorcode );
    void on_finished();

    // dialer::ICallCallback - asynchronous implementation
    void on_call_end( uint32 errorcode );
    void on_dial();
    void on_ring();
    void on_connect();

private:

    bool has_job() const;
    void check_queue();

private:
    class IEvent
    {
    public:
        virtual ~IEvent() {}

        virtual void invoke()   = 0;
    };

    typedef boost::shared_ptr< IEvent > IEventPtr;

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


    typedef std::list< IEventPtr > EventList;

private:
    mutable boost::mutex    mutex_;

    Config                  cfg_;

    IJobPtr                 job_;

    EventList               events_;
};

NAMESPACE_CALMAN_END

#endif  // CALMAN_JOB_PROXY_H
