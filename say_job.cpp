/*

SayJob.

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


// $Id: say_job.cpp 522 2014-05-08 17:02:03Z serge $

#include "say_job.h"                    // self

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../gspeak/gspeak.h"       // GSpeak

NAMESPACE_CALMAN_START

#define MODULENAME      "SayJob"

SayJob::SayJob( const std::string & party, gspeak::GSpeak * gs, const std::string & text, uint32 start_delay ):
        Job( party ), gs_( gs ), text_( text ), start_delay_( start_delay )
{
}
SayJob::~SayJob()
{
}

// virtual functions for overloading
void SayJob::on_custom_activate()
{
}

void SayJob::on_custom_finished()
{
}

NAMESPACE_CALMAN_END
