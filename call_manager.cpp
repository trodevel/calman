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


// $Id: call_manager.cpp 1187 2014-10-22 18:16:17Z serge $

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

bool CallManager::insert_job( IJobPtr job )
{
    SCOPE_LOCK( mutex_ );

    bool res = impl_->insert_job( job );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::wakeup, impl_ ) ) ) );

    return res;
}
bool CallManager::remove_job( IJobPtr job )
{
    SCOPE_LOCK( mutex_ );

    bool res = impl_->remove_job( job );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::wakeup, impl_ ) ) ) );

    return res;
}

bool CallManager::shutdown()
{
    SCOPE_LOCK( mutex_ );

    return proxy_->shutdown();
}

// IDialerCallback interface
void CallManager::on_registered( bool b )
{
    SCOPE_LOCK( mutex_ );

    impl_->on_registered( b );
}
void CallManager::on_call_initiate_response( bool is_initiated, uint32 status, dialer::CallIPtr call )
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_call_initiate_response, impl_, is_initiated, status, call ) ) ) );
}
void CallManager::on_ready()
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_ready, impl_ ) ) ) );
}
void CallManager::on_busy()
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_busy, impl_ ) ) ) );
}
void CallManager::on_error( uint32 errorcode )
{
    SCOPE_LOCK( mutex_ );

    proxy_->add_event( asyncp::IEventPtr( asyncp::new_event( boost::bind( &CallManagerImpl::on_error, impl_, errorcode ) ) ) );
}

NAMESPACE_CALMAN_END
