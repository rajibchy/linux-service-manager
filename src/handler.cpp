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

// 11:45 AM 2/13/2025
// by Rajib Chy
#include <svc/handler.h>
#include <svc/time-range.h>
#include <stdexcept>
#include <exception>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm> // std::find, std::transform
#include <thread>  // Required for std::this_thread::sleep_for
#include <chrono>  // Required for std::chrono::seconds

service_handler_t::service_handler_t( ) {
    
    _logger = std::make_shared<svc_logger>();

    if ( _logger->open() < 0 ) {
        throw std::runtime_error("Unable to open logger");
    }

    _promise = std::make_shared<std::promise<void>>( );
    _future = _promise->get_future( );

#ifndef USE_HTTP_DAY_STATUS
    _is_working_day = true;
#endif //!USE_HTTP_DAY_STATUS

}

int service_handler_t::wait_for( long ms ) {
    // Wait for the specified time in secoands
    std::chrono::seconds wait_interval = std::chrono::seconds( ms / 1000 );
    return _future.wait_for( wait_interval ) == std::future_status::timeout ? 1 : 0;
}

int service_handler_t::prepare( ) {
    
    _logger->info( "Preparing \"Service Manager\"" );

#ifdef USE_HTTP_DAY_STATUS
    std::string http_port;
    std::string http_server;
#endif //!USE_HTTP_DAY_STATUS

    std::vector<dust_clean_config*> dust_configs;

    try {

#ifdef USE_HTTP_DAY_STATUS
        _load_config(
            _services, dust_configs, http_server, http_port 
        );
#else
        _load_config(
            _services, dust_configs 
        );
#endif //!USE_HTTP_DAY_STATUS

        // Iterate through all services in the `_services` list
        for ( const auto& service : _services ) {

            // Normalize the primary service name by ensuring it has the correct extension
            _normalized_service_name( service->service_name );

            // Check if the current service has dependent services
            if( service->has_dependent_service ) {

                // Normalize all dependent service names using std::transform
                std::transform( service->dependent.begin( ), service->dependent.end( ), service->dependent.begin( ), []( std::string& service_name ) {
                    _normalized_service_name( service_name );
                    return service_name;
                });

            }
        }

    } catch( std::exception& w ) {

        _logger->error( w.what() );
        _logger->flush( );

        return 0;

    }
    
    _cleaner = new dust_cleaner_t;
    _cleaner->set_dust_config( dust_configs );

#ifdef USE_HTTP_DAY_STATUS
    _http = new http_client( http_server , http_port );
#endif //!USE_HTTP_DAY_STATUS

    _svc_manager = new service_manager_t;

    if ( !_cleaner->is_empty( ) ) {
        _cleaner->clean( _logger );
    }

    return 1;
}

service_handler_t::~service_handler_t( ) {

#ifdef USE_HTTP_DAY_STATUS
    if ( _http != nullptr ) {
        delete _http;
    }
#endif //!USE_HTTP_DAY_STATUS

    if ( _svc_manager != nullptr ) {
        delete _svc_manager;
    }

    if ( _cleaner != nullptr ) {
        delete _cleaner;
    }

    if ( _logger != nullptr ) {
        _logger->flush( );
        _logger->close( );
        _logger.reset( );
    }

    if ( !_services.empty( ) ) {

        for( const auto& service : _services ) {
            delete service->time_range;
            delete service;
        }

        _services.clear();

    }
    
}
constexpr char CACH_FILE_PATH[]= "./svcm/cache.d";

int _split_string( const std::string& input, std::string& part1, std::string& part2) {
    
    size_t pos = input.find( "~" );
    if (pos != std::string::npos) {
        part1 = input.substr(0, pos);       // Before '~'
        part2 = input.substr(pos + 1);      // After '~'
        return 1;

    }

    return 0;
}

#ifdef USE_HTTP_DAY_STATUS

int service_handler_t::load_last_trade_date( std::string& trade_date ) {

    std::ifstream file( CACH_FILE_PATH, std::ios::binary | std::ios::in );
    
    if ( !file.is_open( ) ) {
        _logger->debug( "No cache file found. File: ", CACH_FILE_PATH );
        return 0;
    }

    if ( !file.good( ) ) {
        _logger->debug( "Invalid file state. File: ", CACH_FILE_PATH );
        return 0;
    }

    // Read file content into a string
    std::ostringstream buffer;
    buffer << file.rdbuf( );
    std::string str_data = std::move(buffer.str());

    std::string file_date;

    // Split the data using `_split_string()`
    if ( _split_string( str_data, file_date, trade_date ) == 0) {
        _logger->error( "Invalid date data found. Data: \"", str_data , "\" File: \"", CACH_FILE_PATH, "\"" );
        return 0;
    }

    // Validate both dates
    if ( !_is_valid_date( file_date ) || !_is_valid_date( trade_date ) ) {
        _logger->error( "Invalid date format. Data: \"", str_data, "\"; File Date: \"", file_date, "\"; Trade Date: \"", trade_date, "\" File: \"", CACH_FILE_PATH, "\"" );
        return 0;
    }

    std::string current_date;
    _get_current_date( current_date );

    return (file_date == current_date) ? 1 : 0;

}

void service_handler_t::save_last_trade_date( const std::string& trade_date ) {

    std::ofstream file( CACH_FILE_PATH, std::ios::binary | std::ios::trunc );

    if ( !file.is_open( ) ) {
        _logger->debug( "Failed to open file for writing. File: \"", CACH_FILE_PATH,  "\"" );
        return;
    }

    std::string current_date;
    _get_current_date(current_date);

    file << current_date << "~" << trade_date; // Write to file

    if (file.fail()) {
        _logger->debug( "Failed to write data to file: \"", CACH_FILE_PATH, "\"" );
    } else {
        _logger->debug( "Trade date cache: \"", trade_date ,"\" write to file: \"", CACH_FILE_PATH, "\"" );
    }
}

int service_handler_t::load_day_status_fallback( ) {

    _logger->info( "Loading trade date from cache : \"", CACH_FILE_PATH, "\"" );

    std::string trade_date;

    if ( load_last_trade_date( trade_date ) == 0 ) {
        return 0;
    }
    
    _logger->info( "Cache Trade Date found \"", trade_date, "\"" );

    _is_working_day = _last_date == trade_date;

    _logger->info( "Current Date: \"", _last_date, "\" is working day : \"", ( _is_working_day ? "true" : "false" ), "\"" );

    if ( !_is_working_day ) {
        _logger->info( "Next working day found \"", trade_date, "\"" );
    }

    return 1;
}


int service_handler_t::load_day_status( ) {

    std::string body;

    int try_count = 0;
    const int max_retries = 10;

    _logger->info( "Loading trade date from host: \"", _http->get_host(), "\"" );

    while ( try_count < max_retries ) {
        try_count++;

        if ( _http->get( "/svc/trade-date", body ) == 0 ) {

            _logger->error( "HTTP request failed: ", _http->get_last_error( ) );

            // Exponential backoff (e.g., 1sec, 2sec, 3sec, ...)
            if ( wait_for( ( 1000 * try_count ) ) == 0 ) {
                return 0;
            }

            continue;

        }

        if ( body.empty( ) ) {

            _logger->error( "HTTP response has no body" );

            // Exponential backoff (e.g., 1sec, 2sec, 3sec, ...)
            if ( wait_for( ( 1000 * try_count ) ) == 0 ) {
                return 0;
            }

            continue;
        }

        if ( !_is_valid_date( body ) ) {

            _logger->error( "Invalid date in HTTP response. Body:", body );

            // Exponential backoff (e.g., 1sec, 2sec, 3sec, ...)
            if ( wait_for( ( 1000 * try_count ) ) == 0 ) {
                return 0;
            }

            continue;
        }

        _logger->info( "Trade Date found \"", body, "\"" );

        _is_working_day = _last_date == body;

        _logger->info( "Current Date: \"", _last_date, "\" is working day : \"", ( _is_working_day ? "true" : "false" ), "\"" );

        if ( !_is_working_day ) {
            _logger->info( "Next working day found \"", body, "\"" );
        }

        save_last_trade_date( body );

        return 1; // Success
    }

    return 0; // Failed after retries

}

#endif //!USE_HTTP_DAY_STATUS

void service_handler_t::restart_service(svc_config& service) {
    _logger->info( "Re-Starting service: \"", service.service_name, "\"" );

    if ( _svc_manager->restart( service.service_name ) == 1 ) {

        service.state = service_state::ACTIVE;
        _logger->info( "\"", service.service_name, "\" restarted" );

    } else {

        _logger->error( "Failed to re-start service: \"", service.service_name, "\"" );
        _logger->error( _svc_manager->get_last_error( ) );

    }
}

void service_handler_t::start_service( svc_config& service ) {

    _logger->info( "Starting service: \"", service.service_name, "\"" );

    if ( _svc_manager->start( service.service_name ) == 1 ) {

        service.state = service_state::ACTIVE;
        _logger->info( "\"", service.service_name, "\" status change to active" );

    } else {

        _logger->error( "Failed to start service: \"", service.service_name, "\"" );
        _logger->error( _svc_manager->get_last_error( ) );

    }

}

void service_handler_t::stop_service( svc_config& service ) {

    _logger->info( "Stopping service: \"", service.service_name, "\"" );

    if( _svc_manager->stop( service.service_name ) == 1 ) {

        service.state = service_state::INACTIVE;
        _logger->info( "\"", service.service_name, "\" status change to in-active" );

    } else {

        _logger->error( "Failed to stop service: \"", service.service_name, "\"" );
        _logger->error( _svc_manager->get_last_error( ) );

    }

}

constexpr char SERVICE_ACTIVE[] = "active";
constexpr char SERVICE_INACTIVE[] = "inactive";
constexpr char SERVICE_ACTIVATING[] = "activating";
constexpr char SERVICE_DEACTIVATING[] = "deactivating";

service_state service_handler_t::get_service_status( const svc_config& service ) {

    std::string result;

    if ( _svc_manager->get_status( service.service_name, result ) < 0 ) {

        _logger->error( "Failed to check status of service: \"", service.service_name, "\"" );
        _logger->error( _svc_manager->get_last_error( ) );
        // here we mean that this service failed or not running
        return service_state::INACTIVE;

    }

    if ( result != SERVICE_ACTIVE ) {

        _logger->info( "Service: \"", service.service_name, "\" Status found :", result );

        if ( result == SERVICE_DEACTIVATING ) {
            return service_state::INACTIVE;
        }

        if ( result == SERVICE_ACTIVATING ) {
            return service_state::ACTIVE;
        }

        return service_state::INACTIVE;
    }

    return service_state::ACTIVE;
    
}

void service_handler_t::update_service_current_state( ) {
    // update service current status
    for ( const auto& service : _services ) {

        _logger->debug( "Prepare service : \"", service->service_name, "\"" );
        service->time_range->print( _logger );

        if ( get_service_status( *service ) == service_state::ACTIVE ) {
            
            service->state = service_state::ACTIVE;
            _logger->debug( "\"", service->service_name, "\" Service status : Active" );

        } else {

            service->state = service_state::INACTIVE;
            _logger->debug( "\"", service->service_name, "\" Service status : Inactive" );
            
        }
    }
}

int service_handler_t::toggel_dependent_service(
    const std::string& root_service,
    const std::vector<std::string>& dependent, 
    const std::time_t& now_time, 
    bool stop 
) {
    int count = 0; // Counter for successfully toggled services

    // Ensure there are dependent services to process
    if ( !dependent.empty( ) ) {

        _logger->info( "Iterate through each dependent service of \"", root_service, "\"" );

        // Iterate over each dependent service
        for ( const auto& service_name : dependent ) {

            if ( _exit_flag.load( ) == 1 ) break;

            // Locate the service configuration using a linear search
            auto it = std::find_if( _services.begin( ), _services.end( ), [&service_name]( svc_config* svc ) {
                return svc && svc->service_name == service_name;
            });

            // If the service is not found, log and skip
            if (it == _services.end( ) || !(*it)) {
                _logger->info("Service \"", service_name, "\" not found");
                continue;
            }

            // Retrieve the service configuration object
            svc_config* service = *it;

            // Fetch the current state of the service
            service_state state = get_service_status( *service );

            if ( stop ) {
                // Stop the service if it's not already inactive
                if ( state != service_state::INACTIVE ) {
                    
                    // Recursively stop all dependent services before stopping this service
                    if( service->has_dependent_service && toggel_dependent_service( service->service_name, service->dependent, now_time, stop ) > 0 ) {

                        // Wait for up to 10 seconds for the dependent service to stop
                        // If `wait_for(10000)` returns 0, it indicates a timeout or successful stop
                        // In case of a timeout, break out of the loop to avoid further processing
                        if ( wait_for( 10000 ) == 0 ) {
                            break;
                        }
                    }

                    stop_service( *service ); // Stop the service
                    service->is_restarted = true; // Mark the service for restart tracking

                    count++; // Increment toggled service count
                }
                continue; // Skip further processing since we are stopping services
            }

            // If starting, ensure service is inactive and within its operational time range
            if ( state == service_state::INACTIVE && service->time_range->is_between_times( now_time ) ) {

                start_service( *service ); // Start the service
                service->is_restarted = true; // Mark the service for restart tracking

                // Recursively start all dependent services after the restart
                if ( service->has_dependent_service && toggel_dependent_service( service->service_name, service->dependent, now_time, stop ) > 0 ) {
                    
                    // Wait for up to 10 seconds for the dependent service to start
                    // If `wait_for(10000)` returns 0, it indicates a timeout or successful start
                    // In case of a timeout, break out of the loop to avoid further processing
                    if ( wait_for( 10000 ) == 0 ) {
                        break;
                    }
                }
                count++; // Increment toggled service count
            }
        }
    }

    return count; // Return the number of services successfully toggled
}

int service_handler_t::block( ) {

    _get_current_date( _last_date );

#ifdef USE_HTTP_DAY_STATUS

    if ( load_day_status( ) == 0 ) {

        if ( load_day_status_fallback() == 0 ) {

            _logger->error( "Failed to load day status for \"", _last_date, "\"" );
            _logger->flush( );

            return 0;

        }
    }
    
 #endif //!USE_HTTP_DAY_STATUS

    update_service_current_state( );

    const long delay_ms = 30000;

    _logger->info( "Starting \"Service Manager\" with 30 sec delay monitor; Total Service: ", _services.size() );

    _logger->flush( );

    while ( true ) {

        if ( _exit_flag.load( ) == 1 ) break;


        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);

        for ( const auto& service : _services ) {

            // Check if the service requires a workday
            if ( service->required_workday ) {

                // If it's not a working day
                if ( !_is_working_day ) {

                    // Check if the service is currently active
                    if (service->state == service_state::ACTIVE) {
                        
                        // If the service is still active according to its status, stop the service
                        if ( get_service_status( *service ) == service_state::ACTIVE ) {
                            stop_service( *service );
                        } else {
                            // Force close request if the service is not active
                            _logger->info( "Initiate \"", service->service_name, "\" force close (1)" );
                            stop_service( *service );
                        }
                    }

                    // Skip the rest of the loop and move to the next service (if applicable)
                    continue;
                }
            }

            // Check if the service supports restart functionality
            if ( service->is_restart_support ) {

                // If the service has not been restarted yet
                if ( !service->is_restarted ) {

                    // Check if the service needs a restart based on the current time
                    if ( service->time_range->need_restart( now_time ) ) {

                        // Stop all dependent services before restarting this service
                        if( service->has_dependent_service && toggel_dependent_service( service->service_name, service->dependent, now_time, true ) > 0 ) {
                            
                            // Give the dependent service a chance to stop completely before continuing.
                            // If `wait_for(10000)` returns 0, it indicates a timeout or successful stop,
                            // so we break the loop to avoid further processing.
                            if ( wait_for( 10000 ) == 0 ) {
                                break;
                            }
                        }

                        // Restart the service if needed
                        restart_service( *service );
                        // Mark the service as restarted to prevent redundant restarts
                        service->is_restarted = true;
                        // Give the service a chance to restart completely before continuing.
                        // If `wait_for(10000)` returns 0, it indicates a timeout or successful stop,
                        // so we break the loop to avoid further processing.
                        if ( wait_for( 10000 ) == 0 ) {
                            break;
                        }

                        // Start all dependent services again after the restart
                        if ( service->has_dependent_service && toggel_dependent_service( service->service_name, service->dependent, now_time, false ) > 0 ) {

                            // Give the dependent service a chance to start completely before continuing.
                            // If `wait_for(10000)` returns 0, it indicates a timeout or successful stop,
                            // so we break the loop to avoid further processing.
                            if ( wait_for( 10000 ) == 0 ) {
                                break;
                            }
                        }

                        // Skip to the next iteration
                        continue;
                    }
                }
            }

            // Check if the service is within its active time range
            if ( service->time_range->is_between_times( now_time ) ) {

                // If the service is currently inactive
                if ( get_service_status( *service ) == service_state::INACTIVE ) {
                    // This means the service failed or is not running; we need to restart it
                    _logger->info( "\"", service->service_name, "\" status inactive. We've to start." );

                    // Start the service
                    start_service( *service );
                }

                // Skip the rest of the loop and move to the next service (if applicable)
                continue;
            }

            // If the service is active
            if ( service->state == service_state::ACTIVE ) {

                // Check if the service is still active based on its current status
                if ( get_service_status( *service ) == service_state::ACTIVE ) {
                    // Stop the active service
                    stop_service( *service );
                } else {
                    // Force close the service if it’s not active, and log the request
                    _logger->info( "Initiate \"", service->service_name, "\" force close (2)" );
                    stop_service( *service );
                }
            }

        }

        // Sleep for 30 seconds before checking again
        if ( wait_for( delay_ms ) == 0 ) {
            break;
        }
        
        if ( switch_to_new_day( ) == 0 ) {
            return 0;
        }

        _logger->flush( );
    }
    
    _logger->info( "\"Service manager\" thread exited." );

    return 1;
}

int service_handler_t::switch_to_new_day() {

    std::string current_date;

    // Retrieve the current date and store it in `current_date`
    _get_current_date( current_date );

    // Check if the date has changed
    if (current_date != _last_date) {
        // Update the last recorded date
        _last_date = current_date;

        // Renew the logger to reflect the new day's logs
        _logger->renew( );

#ifdef USE_HTTP_DAY_STATUS
        // Load the new day's status from an external base server
        if (load_day_status( ) == 0) {

            // Log an error if the day status failed to load
            _logger->error( "Failed to load day status for ", _last_date );
            _logger->flush( );

            return 0; // Return failure
        }
#endif //!USE_HTTP_DAY_STATUS

        // Clean up any leftover service log if the cleaner (service log) is not empty
        if ( !_cleaner->is_empty( ) ) {
            _cleaner->clean( _logger );
        }

        // Prepare the time ranges for all registered services
        for ( const auto& service : _services ) {

            _logger->debug( "Prepare service : \"", service->service_name, "\"" );

            service->time_range->prepare( );
            service->time_range->print( _logger );
            service->is_restarted = false;

            if ( get_service_status( *service ) == service_state::ACTIVE ) {
                
                service->state = service_state::ACTIVE;
                _logger->debug( "\"", service->service_name, "\" Service status : Active" );

            } else {

                service->state = service_state::INACTIVE;
                _logger->debug( "\"", service->service_name, "\" Service status : Inactive" );

            }
        }
    }

    return 1; // Return success
}

void service_handler_t::exit( ) {
    
    _logger->info( "\"Service Manager\" thread exiting..." );

    _exit_flag.store( 1 );

    _promise->set_value( );
}

