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

// 8:26 PM 1/26/2025
// by Rajib Chy
#ifdef _MSC_VER
#pragma once
#endif//_MSC_VER

#ifndef USE_PRODUCTION_BUILD

#include <cstring>
#include <iostream>
#include <svc/httpc.h>
#include <svc/logger.h>
#include <svc/manager.h>

int main( int argc, char** argv ) {

    svc_logger logger;
    
    if( logger.open() < 0 ) {
        return EXIT_FAILURE;
    }
   
    logger.info("Test http request\n");
    logger.flush();

    http_client http("snm.fsys.tech","80");

    std::string body;

    if( http.get("/", body) == 0) {
        
        logger.error( http.get_last_error(), "\n" );
        logger.close();

        return EXIT_FAILURE;
    }

    logger.info( body.c_str(), "\n" );

    if ( argc < 2 ) {
        logger.error("Invalid arguments. Service Name and Task requried.\n" );
        logger.close();
        return EXIT_FAILURE;
    }
    int result = 0;
    service_manager_t svc_manager;

    std::string svc_task = std::string( argv[1] );
    std::string svc_name = std::string( argv[2] );

    _normalized_service_name( svc_name );

    if ( svc_task == "start" ) {

        result = svc_manager.start( svc_name );

    } else if ( svc_task == "restart" ) {

        result = svc_manager.restart( svc_name );

    } else if ( svc_task == "stop" ) {

        result = svc_manager.stop( svc_name );

    } else if ( svc_task == "status" ) {

        std::string status;
        result = svc_manager.get_status( svc_name, status );
        if ( result > 0 ) {
            logger.info( svc_name.c_str( ), " status ", status.c_str( ), "\n" );
        }

    }
    if ( result < 0 ) {

        logger.error( "We are unable to process ", svc_name.c_str( ), " ", svc_task.c_str( ), "\n" );
        logger.error( "Due to Error: ", svc_manager.get_last_error( ), "\n" );

    } else {

        logger.info( svc_name.c_str( ), " status changed to ", svc_task.c_str( ) ," success\n" );
    }
    
    logger.close();

    return EXIT_SUCCESS;
}

#endif //!USE_PRODUCTION_BUILD