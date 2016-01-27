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


// $Revision: 3198 $ $Date:: 2016-01-18 #$ $Author: serge $

#ifndef I_CALMAN_H
#define I_CALMAN_H

#include "namespace_lib.h"          // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

struct Object;

class ICallManager
{
public:

    virtual ~ICallManager() {};

    virtual void consume( const Object * req )    = 0;
};

NAMESPACE_CALMAN_END

#endif  // I_CALMAN_H
