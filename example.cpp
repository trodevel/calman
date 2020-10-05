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

// $Revision: 13958 $ $Date:: 2020-10-05 #$ $Author: serge $

#include <iostream>         // cout
#include <typeinfo>
#include <sstream>          // stringstream
#include <atomic>           // std::atomic
#include <vector>           // std::vector

#include "call_manager.h"                       // calman::CallManager
#include "simple_voip/objects.h"
#include "simple_voip/str_helper.h"
#include "simple_voip/object_factory.h"         // simple_voip::create_initiate_call_request
#include "simple_voip/i_simple_voip_callback.h" // simple_voip::ISimpleVoipCallback

#include "simple_voip_dummy/dummy.h"            // simple_voip_dummy::Dummy
#include "simple_voip_dummy/init_config.h"      // simple_voip_dummy::init_config

#include "utils/dummy_logger.h"              // dummy_log_set_log_level
#include "scheduler/scheduler.h"             // Scheduler

class Callback: virtual public simple_voip::ISimpleVoipCallback
{
public:
    Callback():
        calman_( nullptr ),
        last_req_id_( 0 )
    {
    }

    void init( simple_voip::ISimpleVoip * calman )
    {
        calman_ = calman;
    }

    // interface ISimpleVoipCallback
    void consume( const simple_voip::CallbackObject * req )
    {
        std::cout << "got " << *req << std::endl;

        delete req;
    }

    void control_thread()
    {
        std::cout << "type exit or quit to quit: " << std::endl;
        std::cout << "call <party>" << std::endl;
        std::cout << "drop <call_id>" << std::endl;
        std::cout << "play <call_id> <file>" << std::endl;
        std::cout << "stop_play <call_id>" << std::endl;

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
                last_req_id_++;

                std::string s;
                stream >> s;

                calman_->consume( simple_voip::create_initiate_call_request( last_req_id_, s ) );
            }
            else if( cmd == "drop" )
            {
                last_req_id_++;

                uint32_t call_id;
                stream >> call_id;

                calman_->consume( simple_voip::create_drop_request( last_req_id_, call_id ) );
            }
            else if( cmd == "play" )
            {
                last_req_id_++;

                uint32_t call_id;
                std::string filename;
                stream >> call_id >> filename;

                calman_->consume( simple_voip::create_play_file_request( last_req_id_, call_id, filename ) );
            }
            else if( cmd == "stop_play" )
            {
                last_req_id_++;

                uint32_t call_id;
                stream >> call_id;

                calman_->consume( simple_voip::create_play_file_stop_request( last_req_id_, call_id ) );
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
    uint32_t                    last_req_id_;
};

int main( int argc, char **argv )
{
    unsigned max_active_calls = 1;

    if( argc > 1 )
    {
        max_active_calls = std::stoi( argv[1] );

        if( max_active_calls < 1 || max_active_calls > 10 )
        {
            std::cerr << "ERROR: max_active_calls not [1; 10] -  " << max_active_calls << std::endl;

            return EXIT_FAILURE;
        }
    }

    std::cout << "max_active_calls = " << max_active_calls << std::endl;

    dummy_logger::set_log_level( log_levels_log4j::Debug );

    simple_voip_dummy::Dummy        dialer;
    scheduler::Scheduler            sched( scheduler::Duration( std::chrono::milliseconds( 1 ) ) );
    calman::CallManager             calman;
    Callback test;

    calman::Config              cfg;

    cfg.max_active_calls   = max_active_calls;

    simple_voip_dummy::Config config;

    config_reader::ConfigReader cr;

    std::string config_file( "voip_config.ini" );

    cr.init( config_file );

    std::cout << "loaded config file " + config_file << std::endl;

    simple_voip_dummy::init_config( & config, cr );

    std::string error_msg;

    auto log_id_calman      = dummy_logger::register_module( "CallManager" );
    auto log_id_dummy       = dummy_logger::register_module( "SimpleVoipDummy" );
    auto log_id_call        = dummy_logger::register_module( "Call" );
    auto log_id_sched       = dummy_logger::register_module( "Scheduler" );

    dummy_logger::set_log_level( log_id_calman,     log_levels_log4j::TRACE );
    dummy_logger::set_log_level( log_id_dummy,      log_levels_log4j::INFO );
    dummy_logger::set_log_level( log_id_call,       log_levels_log4j::TRACE );
    dummy_logger::set_log_level( log_id_sched,      log_levels_log4j::TRACE );

    sched.init_log( log_id_sched );

    test.init( & calman );

    bool b = dialer.init( log_id_dummy, log_id_call, config, & calman, & sched, & error_msg );
    if( b == false )
    {
        std::cout << "cannot initialize voip module: " << error_msg << std::endl;
        return EXIT_FAILURE;
    }

    {
        bool b = calman.init( log_id_calman, & dialer, & test, cfg, & error_msg );
        if( !b )
        {
            std::cout << "cannot initialize Calman: " << error_msg << std::endl;
            return EXIT_FAILURE;
        }
    }

    dialer.start();

    std::vector< std::thread > tg;

    tg.push_back( std::thread( std::bind( &Callback::control_thread, &test ) ) );

    sched.run();

    for( auto & t : tg )
        t.join();

    dialer.shutdown();
    calman.shutdown();
    sched.shutdown();

    std::cout << "Done! =)" << std::endl;

    return 0;
}
