/*

Call manager.

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

#ifndef CALMAN_CONFIG_H
#define CALMAN_CONFIG_H

#include <cstdint>                  // uint32_t
#include "namespace_lib.h"          // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

struct Config
{
    uint32_t    sleep_time_ms;  // try 3 ms
};

NAMESPACE_CALMAN_END

#endif  // CALMAN_CONFIG_H
