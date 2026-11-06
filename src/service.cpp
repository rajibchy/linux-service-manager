/*!
* MIT License
* 
* Copyright (c) 2025 Rajib Chowdhury

* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

// This file contains the 'main' function. Program execution begins and ends there.
// 9:44 AM 2/14/20251
// by Rajib Chy

#ifdef _MSC_VER
#pragma once
#endif //!_MSC_VER

#ifdef USE_PRODUCTION_BUILD

#include <signal.h>
#include <csignal>
#include <memory>
#include <atomic>
#include <stdexcept>
#include <exception>
#include <svc/handler.h>


static volatile std::sig_atomic_t _exit_signalled = 0;
static std::shared_ptr<service_handler_t> _handler;

static void on_signal_caught( int signaled ) {
    
    if (_exit_signalled) return;

    _exit_signalled = 1;

    if (!_handler) {
        fprintf(stderr, "Handler is null. Exiting.\n" );
        std::_Exit(EXIT_FAILURE);
        return;
    }

    _handler->_logger->info("Exit signal received; System Id: ", signaled);
    _handler->_logger->flush( );
    _handler->exit( );

}


static void _attach_signal( ) {
    // Interrupt application with ctrl+c or other(s)
    std::signal( SIGINT, on_signal_caught );
    // Software termination signal from kill
    std::signal( SIGTERM, on_signal_caught );
    // Abnormal termination triggered by abort call
    std::signal( SIGABRT, on_signal_caught );

#ifdef _WIN32
    // Ctrl-Break sequence
    std::signal( SIGBREAK, on_signal_caught );
#endif //!_WIN32
}

int main( int argc, char **argv ) {

    _attach_signal( );

    fprintf( stdout, "Initializing \"Service Manager\"\n" );

    fprintf( stdout, "Press Ctrl+C to exit...\n" );

    try {

        _handler = std::make_shared<service_handler_t>( );

    } catch( std::exception& w ) {

        fprintf( stderr, "%s\n", w.what( ) );
        return EXIT_FAILURE;

    }

    if( _handler->prepare( ) == 0 ) {
        _handler.reset();
        fprintf( stderr, "\"Service Manager\" exited with failed prepare call.\n" );
        return EXIT_FAILURE;
    }

    if ( _handler->block( ) == 0 ) {
        _handler.reset( );
        fprintf( stderr, "\"Service Manager\" exited with failed block call.\n" );
        return EXIT_FAILURE;
    }

    _handler->_logger->info( "\"Service Manager\" exited properly" );
    _handler.reset( );

    fprintf( stdout, "ALL IS WELL\n" );

    return EXIT_SUCCESS;
}

#endif // !USE_PRODUCTION_BUILD