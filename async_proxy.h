/*

Dialer's async proxy.

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

// $Id: async_proxy.h 530 2014-05-12 17:07:29Z serge $

#ifndef CALMAN_ASYNC_PROXY_H
#define CALMAN_ASYNC_PROXY_H

#include <list>                     // std::list
#include <boost/thread.hpp>         // boost::mutex

#include "../dialer/i_dialer.h"             // IDialer
#include "../dialer/i_dialer_callback.h"    // IDialerCallback

#include "namespace_calman.h"       // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

class AsyncProxy: virtual public dialer::IDialer, virtual public dialer::IDialerCallback
{
public:
    AsyncProxy();
    ~AsyncProxy();

    bool init( dialer::IDialer  * dialer );

    bool is_inited() const;

    // interface IDialer
    bool register_callback( dialer::IDialerCallback * callback );
    bool initiate_call( const std::string & party, uint32 & status );
    bool drop_all_calls();
    boost::shared_ptr< dialer::CallI > get_call();
    bool shutdown();

    // interface IDialerCallback
    void on_registered( bool b );
    void on_ready();
    void on_busy();
    void on_error( uint32 errorcode );

private:
    bool is_inited__() const;
    bool is_call_id_valid( uint32 call_id ) const;

    void check_call_end( const char * event_name );

private:

    typedef std::list<IJobPtr>  EventQueue;

private:
    mutable boost::mutex        mutex_;

    dialer::IDialer             * dialer_;

    dialer::IDialerCallback     * callback_;
};

NAMESPACE_CALMAN_END

#endif  // CALMAN_ASYNC_PROXY_H
