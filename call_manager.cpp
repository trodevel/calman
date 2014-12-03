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


// $Id: call_manager.cpp 1239 2014-12-02 18:40:44Z serge $

#include "call_manager.h"                 // self

#include "../asyncp/async_proxy.h"      // AsyncProxy
#include "../asyncp/event.h"            // new_event
#include "../utils/assert.h"            // ASSERT
#include "../utils/wrap_mutex.h"        // SCOPE_LOCK
#include "../utils/dummy_logger.h"      // dummy_log

#include "call_manager_impl.h"          // CallManagerImpl

#define MODULENAME      "CallManager"

NAMESPACE_CALMAN_START

CallManager::CallManager()
{
    proxy_  = new asyncp::AsyncProxy;

    {
        asyncp::AsyncProxy::Config cfg;
        cfg.sleep_time_ms   = 1;
        cfg.name            = MODULENAME;

        ASSERT( proxy_->init( cfg ) );
    }

    impl_   = new CallManagerImpl;
}

CallManager::~CallManager()
{
    if( impl_ )
    {
        delete impl_;
        impl_   = nullptr;
    }
}

bool CallManager::init( dialer::IDialer * dialer, const Config & cfg )
{
    SCOPE_LOCK( mutex_ );

    return impl_->init( dialer, cfg );
}

void CallManager::thread_func()
{
    dummy_log_debug( MODULENAME, "thread_func: started" );

    proxy_->thread_func();

    dummy_log_debug( MODULENAME, "thread_func: ended" );
}

bool CallManager::register_callback( ICallManagerCallback * callback )
{
    SCOPE_LOCK( mutex_ );

    return impl_->register_callback( callback );
}

bool CallManager::insert_job( uint32 job_id, const std::string & party )
{
    SCOPE_LOCK( mutex_ );

    bool res = impl_->insert_job( job_id, party );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::wakeup, impl_ ) ) ) );

    return res;
}
bool CallManager::remove_job( uint32 job_id )
{
    SCOPE_LOCK( mutex_ );

    bool res = impl_->remove_job( job_id );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::wakeup, impl_ ) ) ) );

    return res;
}

bool CallManager::shutdown()
{
    SCOPE_LOCK( mutex_ );

    return proxy_->shutdown();
}

// IDialerCallback interface
void CallManager::on_call_initiate_response( uint32 call_id, uint32 status )
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_call_initiate_response, impl_, call_id, status ) ) ) );
}
void CallManager::on_error_response( uint32 call_id, const std::string & descr )
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_error_response, impl_, call_id, descr ) ) ) );
}
void CallManager::on_dial( uint32 call_id )
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_dial, impl_, call_id ) ) ) );
}
void CallManager::on_ring( uint32 call_id )
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_ring, impl_, call_id ) ) ) );
}
void CallManager::on_call_started( uint32 call_id )
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_call_started, impl_, call_id ) ) ) );
}
void CallManager::on_call_duration( uint32 call_id, uint32 t )
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_call_duration, impl_, call_id, t ) ) ) );
}
void CallManager::on_call_end( uint32 call_id, uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_call_end, impl_, call_id, errorcode ) ) ) );
}
void CallManager::on_ready()
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_ready, impl_ ) ) ) );
}
void CallManager::on_error( uint32 call_id, uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_error, impl_, call_id, errorcode ) ) ) );
}
void CallManager::on_fatal_error( uint32 call_id, uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_fatal_error, impl_, call_id, errorcode ) ) ) );
}

NAMESPACE_CALMAN_END
