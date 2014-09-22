/*

Get WAV file duration.

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


// $Id: get_wav_duration.h 1051 2014-09-22 18:05:20Z serge $

#ifndef CALMAN_GET_WAV_DURATION_H
#define CALMAN_GET_WAV_DURATION_H

#include <string>               // std::string
#include "../utils/types.h"     // uint32

#include "namespace_lib.h"      // NAMESPACE_CALMAN_START

NAMESPACE_CALMAN_START

uint32 get_wav_duration( const std::string & filename );

NAMESPACE_CALMAN_END

#endif  // CALMAN_GET_WAV_DURATION_H
