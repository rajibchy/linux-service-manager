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

#include <svc/manager.h>

constexpr const char REPLACE[] = "replace";
constexpr const char GETUNIT[] = "GetUnit";
constexpr const char LOADUNIT[] = "LoadUnit";
constexpr const char STOP_UNIT[] = "StopUnit";
constexpr const char SERVICE_EXT[] = ".service";
constexpr const char START_UNIT[] = "StartUnit";
constexpr const char ACTIVE_STATE[] = "ActiveState";
constexpr const char RESTART_UNIT[] = "RestartUnit";
constexpr const char ORG_FREEDESKTOP_SYSTEMD[] = "org.freedesktop.systemd1";
constexpr const char ORG_FREEDESKTOP_SYSTEMD_PATH[] = "/org/freedesktop/systemd1";
constexpr const char ORG_FREEDESKTOP_SYSTEMD_UNIT[] = "org.freedesktop.systemd1.Unit";
constexpr const char ORG_FREEDESKTOP_SYSTEMD_MANAGER[] = "org.freedesktop.systemd1.Manager";

service_manager_t::service_manager_t( ) {
    // Create the D-Bus system bus connection only once when the object is created
    _connection = sdbus::createSystemBusConnection( );
}

// Start a service
int service_manager_t::start( const std::string& service_name ) {
    return call_systemd_method( START_UNIT, service_name, REPLACE );
}

// Stop a service
int service_manager_t::stop( const std::string& service_name ) {
    return call_systemd_method( STOP_UNIT, service_name, REPLACE );
}

// Restart a service
int service_manager_t::restart( const std::string& service_name ) {
    return call_systemd_method( RESTART_UNIT, service_name, REPLACE );
}

// Get the status of a service
int service_manager_t::get_status( const std::string& service_name, std::string& result ) {

    try {
        // Get the systemd manager object
        sdbus::ServiceName orgfsym = sdbus::ServiceName(ORG_FREEDESKTOP_SYSTEMD);
        sdbus::ObjectPath orgfsympath = sdbus::ObjectPath(ORG_FREEDESKTOP_SYSTEMD_PATH);
        std::unique_ptr<sdbus::IProxy> proxy = sdbus::createProxy(
            *_connection, orgfsym, orgfsympath
        );

        // Prepare a variable for the result
        sdbus::ObjectPath object_path;

        // Call LoadUint and retrieve the object path
        // Use LoadUnit instead of GetUnit
        
        // GetUnit only works for loaded units. If a service has never been started,
        // or if it's explicitly stopped and garbage-collected, systemd removes it from memory.
        proxy->callMethod( LOADUNIT )
            .onInterface( ORG_FREEDESKTOP_SYSTEMD_MANAGER )
            .withArguments( service_name )
            .storeResultsTo( object_path ); // The ObjectPath type will work here

        // Use the retrieved object path for the next proxy
        auto unitProxy = sdbus::createProxy(
            *_connection, orgfsym, object_path
        );

        // Retrieve the ActiveState property
        sdbus::Variant activeStateVariant = unitProxy->getProperty( ACTIVE_STATE )
            .onInterface( ORG_FREEDESKTOP_SYSTEMD_UNIT );

        // Extract the ActiveState as a string
        result = activeStateVariant.get<std::string>( );

        if ( result.empty( ) ) {
            result = "inactive";
        }

        return 1; // Success

    } catch ( const sdbus::Error& e ) {
        
        result = "inactive";

        set_last_error( "D-Bus error: ", e.what( ) );

        return -1;

    } catch ( const std::exception& e ) {

        set_last_error( "Unexpected error: ", e.what( ) );
        
        return -1;

    }
}

const char* service_manager_t::get_last_error( ) {
    if ( _last_error.empty( ) ) {
        return nullptr;
    }
    return _last_error.c_str( );
}

void service_manager_t::set_last_error( const std::string& prefix, const std::string& errror_str ) {
    std::string( prefix.c_str( ) ).append( " " ).append( errror_str.c_str( ) ).swap( _last_error );
}

// Helper to call StartUnit, StopUnit, or RestartUnit
int service_manager_t::call_systemd_method( const std::string& method, const std::string& service_name, const std::string& mode ) {

    try {
        sdbus::ServiceName orgfsym = sdbus::ServiceName(ORG_FREEDESKTOP_SYSTEMD);
        sdbus::ObjectPath orgfsympath = sdbus::ObjectPath(ORG_FREEDESKTOP_SYSTEMD_PATH);
        // Get the systemd manager object
        std::unique_ptr<sdbus::IProxy> proxy = sdbus::createProxy(
            *_connection, orgfsym, orgfsympath
        );

        // Call the specified method
        proxy->callMethod( method )
            .onInterface( ORG_FREEDESKTOP_SYSTEMD_MANAGER )
            .withArguments( service_name, mode );

        return 1;

    } catch ( const sdbus::Error& e ) {

        set_last_error( "D-Bus error: ", e.what( ) );

        return -1;
    }

}

void _normalized_service_name( std::string& service_name ) {
    // Check if the service name already has an extension (a period in the name)
    if ( service_name.find( '.' ) == std::string::npos ) {
        service_name += SERVICE_EXT; // Append the required service extension (e.g., ".service")
    }
}