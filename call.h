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


// $Id: call.h 1312 2015-01-05 17:31:50Z serge $

#ifndef CALMAN_CALL_H
#define CALMAN_CALL_H

#include <string>                   // std::string
#include <boost/thread.hpp>         // boost::mutex
#include "../utils/types.h"         // uint32
#include "../jobman/job.h"          // Job
#include "objects.h"                // CalmanDrop
#include "../dialer/objects.h"      // DialerDial

#include "namespace_lib.h"          // NAMESPACE_CALMAN_START

namespace dialer
{
class IDialer;
}

NAMESPACE_CALMAN_START

class ICallManagerCallback;

class Call: public jobman::Job
{
public:
    enum status_e
    {
        UNDEF,
        IDLE,
        PREPARING,
        ACTIVE,
        WAITING_DROP_RESPONSE,
        DONE
    };

public:
    Call(
        uint32                  parent_job_id,
        const std::string       & party,
        ICallManagerCallback    * callback,
        dialer::IDialer         * dialer );

    const std::string & get_party() const;

    // partly interface ICallManagerCallback
    void handle( const CalmanPlayFile * req );
    void handle( const CalmanDrop * req );

    // partly interface IDialerCallback
    // not needed: void handle( const dialer::DialerInitiateCallResponse * obj );
    // not needed: void handle( const dialer::DialerErrorResponse * obj );
    void handle( const dialer::DialerDial * obj );
    void handle( const dialer::DialerRing * obj );
    void handle( const dialer::DialerConnect * obj );
    void handle( const dialer::DialerCallDuration * obj );
    void handle( const dialer::DialerCallEnd * obj );
    void handle( const dialer::DialerDropResponse * obj );

private:

    mutable boost::mutex    mutex_;

    std::string             party_;

    status_e                state_;

    ICallManagerCallback    * callback_;
    dialer::IDialer         * dialer_;
};

typedef boost::shared_ptr< Call >   CallPtr;

NAMESPACE_CALMAN_END

#endif  // CALMAN_CALL_H
