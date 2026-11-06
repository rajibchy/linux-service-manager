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

// 7:05 PM 2/13/2025
// by Rajib Chy
#include <svc/config.h>
#include <vector>
#include <stdexcept>
#include <exception>
#include <svc/time-range.h>
#include <svc/json-config.h>


#ifdef USE_HTTP_DAY_STATUS

#ifndef MAX_PORT
#define MAX_PORT 0xFFFF  // Define the maximum port number (65535) if not already defined
#endif // !MAX_PORT

void _load_config(
     std::vector<svc_config*>& svc_configs,
     std::vector<dust_clean_config*>& dust_configs,
     std::string& http_server, std::string& http_port 
) {

#else

void _load_config(
     std::vector<svc_config*>& svc_configs,
     std::vector<dust_clean_config*>& dust_configs
) {

#endif //!USE_HTTP_DAY_STATUS

    // Load configuration from the JSON file
    json_config_t reader( "./svcm/config.json" ); // must be Object

    json_config_t part;

#ifdef USE_HTTP_DAY_STATUS

    // Read HTTP configuration
    if( reader.get_next_part( "http", part ) == 0 ) {
        throw std::runtime_error( "config->http (Object) config not found at ./svcm/config.json" );
    }

    // Extract server address from HTTP configuration
    if( part.get_string( "server", http_server ) == 0 ) {
        throw std::runtime_error( "config->http->server (string) not found at ./svcm/config.json" );
    }

    int port_num = 0;

    // Extract port number from HTTP configuration
    if( part.get_int( "port", &port_num ) == 0 ) {
        throw std::runtime_error( "config->http->port (number) not found at ./svcm/config.json" );
    }

    part.clear( );  // Clear the JSON part to free memory

    // Validate the port number (must be a valid non-HTTPS port and within range)
    if ( port_num == 0 || port_num == 443 || port_num >= MAX_PORT ) {
         throw std::runtime_error( "config->http->port (number) invalid (https port not supported). Port range must be < 65535; File: ./svcm/config.json" );
    }
    
    http_port = std::string( std::to_string( port_num ) );  // Convert port number to string

#endif //!USE_HTTP_DAY_STATUS

    // Read service configurations (array of services)
    if( reader.get_next_part( "svc", part, 1 ) == 0 ) {
        throw std::runtime_error( "config->svc (Array) config not found at ./svcm/config.json" );
    }

    // Iterate over each service configuration in the array
    part.each( [&]( json_config_t& next_part ) {
        svc_config* fcfg = new svc_config;  // Allocate new service configuration object

        // Extract service name
        if( next_part.get_string( "name", fcfg->service_name ) == 0 ) {
            throw std::runtime_error( "config->svc->[index]->name (string) not found at ./svcm/config.json" );
        }

        // Extract start time
        if( next_part.get_string( "start", fcfg->start_time ) == 0 ) {
            throw std::runtime_error( "config->svc->[index]->start (string) time not found at ./svcm/config.json" );
        }

        // Extract end time
        if( next_part.get_string("end", fcfg->end_time) == 0 ) {
            throw std::runtime_error( "config->svc->[index]->end (string) time not found at ./svcm/config.json" );
        }

        if( next_part.get_string( "restart", fcfg->restart_time ) == 0 ) {
            // this service not supported restart
        }

        // Extract required_workday flag (boolean)
        if( next_part.get_bool( "required_workday", &fcfg->required_workday ) == 0) {
            throw std::runtime_error( "config->svc->[index]->required_workday (boolean) not found at ./svcm/config.json" );
        }

        // Extract dependent service
        if( next_part.get_to( "dependent", &fcfg->dependent ) > 0) {
            fcfg->has_dependent_service = fcfg->dependent.size() > 0;
        }
        
        // Create a time range object using start and end time
        fcfg->time_range = new time_range_t( fcfg->start_time, fcfg->end_time, fcfg->restart_time );

        fcfg->is_restart_support = fcfg->time_range->is_restart_supported( );
        
        svc_configs.push_back( fcfg );  // Store the service configuration

    });

    part.clear( );  // Clear the JSON part

    // Read dust configurations (Array of dust)
    if( reader.get_next_part( "dust", part, 1 ) != 0 ) {
		
        part.each_keys( [&]( const std::string& key, json &val ) {
			
            if ( !val.is_object() ) {
                throw std::runtime_error( "config->dust->[index] (object) not found at ./svcm/config.json" );
            }
			
            dust_clean_config* cfg = new dust_clean_config;
            _dust_config_from_json( val, cfg );
            dust_configs.push_back( cfg );

        });

        part.clear( ); // Clear the JSON part
        
    }

    reader.clear( );  // Clear the JSON reader to free resources
}