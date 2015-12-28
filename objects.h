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


// $Revision: 3071 $ $Date:: 2015-12-28 #$ $Author: serge $

#ifndef CALMAN_OBJECTS_H
#define CALMAN_OBJECTS_H

#include <string>                   // std::string
#include <cstdint>                  // uint32_t

#include "../servt/i_object.h"      // IObject

#include "namespace_lib.h"          // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

struct Object: public servt::IObject
{
    uint32_t        job_id;
};

struct InitiateCall: public Object
{
    std::string     party;
};

struct CancelCall: public Object
{
};

struct PlayFileRequest: public Object
{
    std::string     filename;
};

struct DropRequest: public Object
{
};

struct CallbackObject: public Object
{
};

struct ProcessingStarted: public CallbackObject
{
};

struct CallStarted: public CallbackObject
{
};

struct CallDuration: public CallbackObject
{
    uint32_t t;
};

struct PlayFileResponse: public CallbackObject
{
};

struct ConnectionLost: public CallbackObject
{
    uint32_t        errorcode;
    std::string     descr;
};

struct DropResponse: public CallbackObject
{
};

NAMESPACE_CALMAN_END

#endif  // CALMAN_OBJECTS_H
