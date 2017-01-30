/*

Example.

Copyright (C) 2015 Sergey Kolevatov

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

// $Revision: 5614 $ $Date:: 2017-01-24 #$ $Author: serge $

#include <iostream>         // cout
#include <typeinfo>
#include <sstream>          // stringstream
#include <atomic>           // std::atomic
#include <vector>           // std::vector

#include "../simple_voip/i_simple_voip_callback.h"  // simple_voip::ISimpleVoipCallback
#include "call_manager.h"               // calman::CallManager
#include "../simple_voip/object_factory.h"          // simple_voip::create_message_t

#include "../dialer_detect/dialer_detector.h"   // dialer_detector::DialerDetector
#include "../skype_service/skype_service.h"     // SkypeService
#include "../utils/dummy_logger.h"      // dummy_log_set_log_level
#include "../scheduler/scheduler.h"     // Scheduler

namespace sched
{
extern unsigned int MODULE_ID;
}

class Callback: virtual public simple_voip::ISimpleVoipCallback
{
public:
    Callback( simple_voip::ISimpleVoip * calman, sched::Scheduler * sched ):
        calman_( calman ),
        sched_( sched )
    {
    }

    // interface ISimpleVoipCallback
    void consume( const simple_voip::CallbackObject * req )
    {
        std::cout << "got " << typeid( *req ).name() << std::endl;

        if( typeid( *req ) == typeid( simple_voip::InitiateCallResponse ) )
        {
            std::cout << "got InitiateCallResponse"
                    << " job_id " << dynamic_cast< const simple_voip::InitiateCallResponse *>( req )->job_id
                    << std::endl;
        }
        else if( typeid( *req ) == typeid( simple_voip::ErrorResponse ) )
        {
            std::cout << "got ErrorResponse"
                    << " job_id " << dynamic_cast< const simple_voip::ErrorResponse *>( req )->job_id
                    << "  " << dynamic_cast< const simple_voip::ErrorResponse *>( req )->descr
                    << std::endl;
        }
        else if( typeid( *req ) == typeid( simple_voip::RejectResponse ) )
        {
            std::cout << "got RejectResponse"
                    << " job_id " << dynamic_cast< const simple_voip::RejectResponse *>( req )->job_id
                    << " " << dynamic_cast< const simple_voip::RejectResponse *>( req )->descr
                    << std::endl;
        }
        else if( typeid( *req ) == typeid( simple_voip::DropResponse ) )
        {
            std::cout << "got DropResponse"
                    << " job_id " << dynamic_cast< const simple_voip::DropResponse *>( req )->job_id
                    << std::endl;
        }
        else if( typeid( *req ) == typeid( simple_voip::Failed ) )
        {
            std::cout << "got Failed"
                    << " call_id " << dynamic_cast< const simple_voip::Failed *>( req )->call_id
                    << " " << dynamic_cast< const simple_voip::Failed *>( req )->descr
                    << std::endl;
        }
        else if( typeid( *req ) == typeid( simple_voip::ConnectionLost ) )
        {
            std::cout << "got ConnectionLost"
                    << " call_id " << dynamic_cast< const simple_voip::ConnectionLost *>( req )->call_id
                    << " " << dynamic_cast< const simple_voip::ConnectionLost *>( req )->descr
                    << std::endl;
        }
        else if( typeid( *req ) == typeid( simple_voip::Connected ) )
        {
            std::cout << "got Connected"
                    << " call_id " << dynamic_cast< const simple_voip::Connected *>( req )->call_id
                    << std::endl;
        }
        else if( typeid( *req ) == typeid( simple_voip::DtmfTone ) )
        {
            std::cout << "got DtmfTone"
                    << " call_id " << dynamic_cast< const simple_voip::DtmfTone *>( req )->call_id
                    << " tone " << static_cast<uint16_t>(
                            dynamic_cast< const simple_voip::DtmfTone *>( req )->tone )
                    << std::endl;
        }
        else if( typeid( *req ) == typeid( simple_voip::PlayFileResponse ) )
        {
            std::cout << "got PlayFileResponse"
                    << " job_id " << dynamic_cast< const simple_voip::PlayFileResponse *>( req )->job_id
                    << std::endl;
        }
        else
        {
            std::cout << "got unknown event" << std::endl;
        }

        delete req;
    }

    void control_thread()
    {
        std::cout << "type exit or quit to quit: " << std::endl;
        std::cout << "call <job_id> <party>" << std::endl;
        std::cout << "drop <job_id> <call_id>" << std::endl;
        std::cout << "play <job_id> <call_id> <file>" << std::endl;

        std::string input;

        while( true )
        {
            std::cout << "enter command: ";

            std::getline( std::cin, input );

            std::cout << "your input: " << input << std::endl;

            bool b = process_input( input );

            if( b == false )
                break;
        };

        std::cout << "exiting ..." << std::endl;

        sched_->shutdown();
    }

private:
    bool process_input( const std::string & input )
    {
        std::string cmd;

        std::stringstream stream( input );
        if( stream >> cmd )
        {
            if( cmd == "exit" || cmd == "quit" )
            {
                return false;
            }
            else if( cmd == "call" )
            {
                uint32_t job_id;
                std::string s;
                stream >> job_id >> s;

                calman_->consume( simple_voip::create_initiate_call_request( job_id, s ) );
            }
            else if( cmd == "drop" )
            {
                uint32_t job_id;
                uint32_t call_id;
                stream >> job_id >> call_id;

                calman_->consume( simple_voip::create_drop_request( job_id, call_id ) );
            }
            else if( cmd == "play" )
            {
                uint32_t job_id;
                uint32_t call_id;
                std::string filename;
                stream >> job_id >> call_id >> filename;

                calman_->consume( simple_voip::create_play_file_request( job_id, call_id, filename ) );
            }
            else
                std::cout << "ERROR: unknown command '" << cmd << "'" << std::endl;
        }
        else
        {
            std::cout << "ERROR: cannot read command" << std::endl;
        }
        return true;
    }

private:
    simple_voip::ISimpleVoip    * calman_;
    sched::Scheduler            * sched_;
};

void scheduler_thread( sched::Scheduler * sched )
{
    sched->start( true );
}

int main()
{
    sched::MODULE_ID            = dummy_logger::register_module( "Scheduler" );
    calman::Call::CLASS_ID      = dummy_logger::register_module( "Call" );

    dummy_logger::set_log_level( sched::MODULE_ID,          log_levels_log4j::ERROR );
    dummy_logger::set_log_level( calman::Call::CLASS_ID,    log_levels_log4j::TRACE );
    dummy_logger::set_log_level( log_levels_log4j::DEBUG );

    skype_service::SkypeService     sio;
    dialer_detector::DialerDetector dialer( 16000 );
    calman::CallManager             calman;
    sched::Scheduler                sched;

    uint16_t                    port = 3217;

    calman::Config              cfg;

    cfg.sleep_time_ms   = 3;

    sched.load_config();
    sched.init();

    {
        bool b = calman.init( & dialer, cfg );
        if( !b )
        {
            std::cout << "cannot initialize Calman" << std::endl;
            return 0;
        }
    }

    {
        bool b = dialer.init( & sio, & sched, port );
        if( !b )
        {
            std::cout << "cannot initialize Dialer" << std::endl;
            return 0;
        }

        dialer.register_callback( & calman );
    }

    {
        bool b = sio.init();

        if( !b )
        {
            std::cout << "cannot initialize SkypeService - " << sio.get_error_msg() << std::endl;
            return 0;
        }

        sio.register_callback( & dialer );
    }

    Callback test( & calman, & sched );
    calman.register_callback( &test );

    dialer.start();
    calman.start();

    std::vector< std::thread > tg;

    tg.push_back( std::thread( std::bind( &Callback::control_thread, &test ) ) );
    tg.push_back( std::thread( std::bind( &scheduler_thread, &sched ) ) );

    for( auto & t : tg )
        t.join();

    dialer.DialerDetector::shutdown();
    calman.shutdown();

    std::cout << "Done! =)" << std::endl;

    return 0;
}
