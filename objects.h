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


// $Id: objects.h 1325 2015-01-06 18:09:17Z serge $

#ifndef CALMAN_OBJECTS_H
#define CALMAN_OBJECTS_H

#include <string>                   // std::string
#include "../utils/types.h"         // uint32

#include "../servt/i_object.h"      // IObject

#include "namespace_lib.h"          // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

struct CalmanObject: public servt::IObject
{
    uint32          job_id;
};

struct CalmanInsertJob: public CalmanObject
{
    std::string     party;
};

struct CalmanRemoveJob: public CalmanObject
{
};

struct CalmanPlayFile: public CalmanObject
{
    std::string     filename;
};

struct CalmanDrop: public CalmanObject
{
};

struct CalmanCallbackObject: public CalmanObject
{
};

struct CalmanProcessingStarted: public CalmanCallbackObject
{
};

struct CalmanCallStarted: public CalmanCallbackObject
{
};

struct CalmanCallDuration: public CalmanCallbackObject
{
    uint32 t;
};

struct CalmanPlayStarted: public CalmanCallbackObject
{
};

struct CalmanPlayStopped: public CalmanCallbackObject
{
};

struct CalmanPlayFailed: public CalmanCallbackObject
{
};

struct CalmanFinishedByOtherParty: public CalmanCallbackObject
{
    uint32          errorcode;
    std::string     descr;
};

struct CalmanDropResponse: public CalmanCallbackObject
{
};

NAMESPACE_CALMAN_END

#endif  // CALMAN_OBJECTS_H
