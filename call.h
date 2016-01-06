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


// $Revision: 3108 $ $Date:: 2016-01-06 #$ $Author: serge $

#ifndef CALMAN_CALL_H
#define CALMAN_CALL_H

#include <string>                   // std::string
#include <mutex>                    // std::mutex
#include <memory>                   // std::shared_ptr
#include "../utils/types.h"         // uint32_t
#include "objects.h"                // DropRequest
#include "../voip_io/objects.h"     // voip_service::Dial

#include "namespace_lib.h"          // NAMESPACE_CALMAN_START

namespace voip_service
{
class IVoipService;
}

NAMESPACE_CALMAN_START

class ICallManagerCallback;

class Call
{
public:
    enum status_e
    {
        UNDEF,
        IDLE,
        WAITING_DIALER_FREE,
        WAITING_INITIATE_CALL_RESP,
        WAITING_CONNECTION,
        CONNECTED,
        WAITING_DROP_RESPONSE,
        DONE
    };

public:
    Call(
        uint32_t                    parent_job_id,
        const std::string           & party,
        ICallManagerCallback        * callback,
        voip_service::IVoipService  * voips );

    bool is_completed() const;
    bool remove() const;
    uint32_t get_parent_job_id() const;

    // partly interface ICallManagerCallback
    void initiate();
    void handle( const PlayFileRequest * req );
    void handle( const DropRequest * req );

    // partly interface IVoipServiceCallback
    void handle( const voip_service::InitiateCallResponse * obj );
    void handle( const voip_service::ErrorResponse * obj );
    void handle( const voip_service::RejectResponse * obj );
    void handle( const voip_service::Dial * obj );
    void handle( const voip_service::Ring * obj );
    void handle( const voip_service::Connected * obj );
    void handle( const voip_service::CallDuration * obj );
    void handle( const voip_service::ConnectionLost * obj );
    void handle( const voip_service::DropResponse * obj );
    void handle( const voip_service::Failed * obj );
    void handle( const voip_service::PlayFileResponse * obj );

private:
    void trace_state_switch() const;

    void send_reject_due_to_wrong_state();
    bool send_reject_if_in_request_processing();

    void send_error_response( const std::string & descr );
    void send_reject_response( const std::string & descr );

    void validate_and_reset_response_job_id( const voip_service::ResponseObject * resp );
    void callback_consume( const CallbackObject * req );

    static uint32_t get_next_request_id();
    static uint32_t decode_failure_reason();

private:

    mutable std::mutex      mutex_;

    std::string             party_;

    status_e                state_;

    uint32_t                parent_job_id_;
    uint32_t                current_req_id_;
    uint32_t                call_id_;
    uint32_t                sleep_interval_;

    ICallManagerCallback        * callback_;
    voip_service::IVoipService  * voips_;
};

typedef std::shared_ptr< Call >   CallPtr;

NAMESPACE_CALMAN_END

#endif  // CALMAN_CALL_H
