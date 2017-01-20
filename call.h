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


// $Revision: 5578 $ $Date:: 2017-01-19 #$ $Author: serge $

#ifndef CALMAN_CALL_H
#define CALMAN_CALL_H

#include <string>                   // std::string
#include <mutex>                    // std::mutex
#include <memory>                   // std::shared_ptr
#include <cstdint>                  // uint32_t
#include "../simple_voip/objects.h"     // simple_voip::Dialing

#include "namespace_lib.h"          // NAMESPACE_CALMAN_START

namespace simple_voip
{
class ISimpleVoip;
class ISimpleVoipCallback;
}

NAMESPACE_CALMAN_START

class Call
{
public:
    static unsigned int CLASS_ID;

public:
    enum state_e
    {
        IDLE,
        WAITING_DIALER_FREE,
        WAITING_INITIATE_CALL_RESP,
        WAITING_CONNECTED,
        CONNECTED,
        CONNECTED_BUSY,
        WRONG_CONNECTED,
        CANCELLED_IN_WICR,
        CANCELLED_IN_WC,
        CANCELLED_IN_C,
        CANCELLED_IN_CB,
        DONE
    };

public:
    Call(
        uint32_t                    parent_job_id,
        const std::string           & party,
        simple_voip::ISimpleVoipCallback        * callback,
        simple_voip::ISimpleVoip  * voips );

    bool is_completed() const;
    uint32_t get_parent_job_id() const;

    // partly interface simple_voip::ISimpleVoipCallback
    void initiate();
    void handle( const simple_voip::PlayFileRequest * obj );
    void handle( const simple_voip::DropRequest * obj );

    // partly interface ISimpleVoipCallback
    void handle( const simple_voip::InitiateCallResponse * obj );
    void handle( const simple_voip::ErrorResponse * obj );
    void handle( const simple_voip::RejectResponse * obj );
    void handle( const simple_voip::Dialing * obj );
    void handle( const simple_voip::Ringing * obj );
    void handle( const simple_voip::Connected * obj );
    void handle( const simple_voip::ConnectionLost * obj );
    void handle( const simple_voip::DropResponse * obj );
    void handle( const simple_voip::Failed * obj );
    void handle( const simple_voip::PlayFileResponse * obj );
    void handle( const simple_voip::DtmfTone * obj );

private:
    void next_state( state_e state );
    void trace_state_switch() const;

    void send_error_response( const std::string & descr );

    void validate_and_reset_response_job_id( const simple_voip::ResponseObject * resp );
    void callback_consume( const simple_voip::CallbackObject * req );

    static uint32_t get_next_request_id();
    static simple_voip::Failed::type_e decode_failure_reason( simple_voip::Failed::type_e type );

    static dtmf_tools::tone_e decode_tone( simple_voip::DtmfTone::tone_e tone );

private:

    mutable std::mutex      mutex_;

    std::string             party_;

    state_e                 state_;

    uint32_t                parent_job_id_;
    uint32_t                current_req_id_;
    uint32_t                call_id_;
    uint32_t                sleep_interval_;

    simple_voip::ISimpleVoipCallback        * callback_;
    simple_voip::ISimpleVoip    * voips_;
};

typedef std::shared_ptr< Call >   CallPtr;

NAMESPACE_CALMAN_END

#endif  // CALMAN_CALL_H
