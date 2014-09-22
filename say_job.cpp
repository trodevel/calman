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


// $Id: say_job.cpp 1052 2014-09-22 18:05:57Z serge $

#include "say_job.h"                    // self

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log
#include "../gspeak/gspeak.h"           // GSpeak
#include "../utils/tune_wav.h"          // tune_wav

#include "get_wav_duration.h"           // get_wav_duration

NAMESPACE_CALMAN_START

#define MODULENAME      "SayJob"

SayJob::SayJob(
        const std::string   & party,
        gspeak::GSpeak      * gs,
        const std::string   & text,
        const std::string   & temp_path,
        uint32              start_delay ):
        Job( party ), gs_( gs ), text_( text ), temp_path_( temp_path ), start_delay_( start_delay ),
        is_job_done_( false ),
        speech_duration_( 0 )
{
}
SayJob::~SayJob()
{
}

void SayJob::on_call_duration( uint32 t )
{
    SCOPE_LOCK( mutex_ );

    if( is_job_done_ == true )
        return;

    if( t < start_delay_ )
        return;

    is_job_done_    = true;

    if( call_ == 0 )
    {
        dummy_log_error( MODULENAME, "call is NULL" );
        return;
    }

    if( !call_->is_active() )
    {
        dummy_log_error( MODULENAME, "call is not active" );
        return;
    }


    dummy_log_debug( MODULENAME, "playing '%s'", filename_.c_str() );

    {
        bool b = call_->set_input_file( filename_ );

        if( b == false )
        {
            dummy_log_error( MODULENAME, "failed sending '%s'", text_.c_str() );
            return;
        }
    }
}

// virtual functions for overloading
void SayJob::on_custom_processing_started()
{
    if( text_.empty() )
    {
        dummy_log_warn( MODULENAME, "text is empty" );
        return;
    }

    std::string temp_name = std::string( temp_path_ ).append( "/" ).append( "say_job_raw.wav" );
    filename_ = std::string( temp_path_ ).append( "/" ).append( "say_job.wav" );

    {

        bool b = gs_->say( text_, temp_name );
        if( b == false )
        {
            dummy_log_error( MODULENAME, "cannot generate text" );

            is_job_done_    = true;     // prevent from further processing

            return;
        }
    }

    gs_->save_state();

    tune_wav( temp_name, filename_ );

    speech_duration_    = get_wav_duration( filename_ );

    if( speech_duration_ == 0 )
    {
        dummy_log_error( MODULENAME, "speech duration is 0" );

        is_job_done_    = true;     // prevent from further processing

        return;
    }
}
void SayJob::on_custom_activate()
{
}
void SayJob::on_custom_finished()
{
}

NAMESPACE_CALMAN_END
