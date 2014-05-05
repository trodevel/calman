/*

Call manager interface.

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


// $Id: i_call_manager.h 509 2014-05-05 17:30:47Z serge $

#ifndef I_CALMAN_H
#define I_CALMAN_H

#include <string>                   // std::string
#include "../utils/types.h"         // uint32

#include "namespace_calman.h"       // NAMESPACE_CALMAN_START
#include "i_job.h"                  // IJob

NAMESPACE_CALMAN_START

class ICallManager
{
public:
    enum state_e
    {
        UNDEF   = 0,
        IDLE,
        BUSY
    };

public:
    virtual ~ICallManager() {};

    virtual bool insert_job( IJobPtr job )                          = 0;
    virtual bool remove_job( IJobPtr job )                          = 0;

    virtual bool shutdown()                                         = 0;
};

NAMESPACE_CALMAN_END

#endif  // I_CALMAN_H
