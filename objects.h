/*

Call manager objects.

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


// $Revision: 5526 $ $Date:: 2017-01-10 #$ $Author: serge $

#ifndef CALMAN_OBJECTS_H
#define CALMAN_OBJECTS_H

#include <string>                   // std::string
#include <cstdint>                  // uint32_t

#include "../workt/i_object.h"      // IObject
#include "../dtmf_tools/dtmf_enum.h"        // dtmf_tools::tone_e

#include "namespace_lib.h"          // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

struct Object: public workt::IObject
{
    uint32_t        job_id;
};

struct CallbackObject: public Object
{
};

struct ErrorResponse: public CallbackObject
{
    std::string     descr;
};

struct RejectResponse: public CallbackObject
{
    std::string     descr;
};

struct InitiateCallRequest: public Object
{
    std::string     party;
};

struct InitiateCallResponse: public CallbackObject
{
};

struct DropRequest: public Object
{
};

struct DropResponse: public CallbackObject
{
};

struct PlayFileRequest: public Object
{
    std::string     filename;
};

struct PlayFileResponse: public CallbackObject
{
};

struct Connected: public CallbackObject
{
};

struct DtmfTone: public CallbackObject
{
    dtmf_tools::tone_e  tone;
};

struct Failed: public CallbackObject
{
    enum type_e
    {
        FAILED,
        REFUSED,
        BUSY,
        NOANSWER
    };

    type_e          type;
    uint32_t        errorcode;
    std::string     descr;
};

struct ConnectionLost: public CallbackObject
{
    uint32_t        errorcode;
    std::string     descr;
};

NAMESPACE_CALMAN_END

#endif  // CALMAN_OBJECTS_H
