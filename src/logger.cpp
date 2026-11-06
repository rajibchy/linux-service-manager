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

// 8:39 PM 2/12/2025
// by Rajib Chy
#include <svc/logger.h>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

constexpr size_t MAX_SIZE = 20000000 * 2; // 40mb

int _create_log_directory( const std::string& path ) {
	fs::path upath = path.c_str( );
	// check is file path
	try {
		if ( !upath.is_absolute( ) ) {
			fs::path dpath = upath.parent_path( );
			// check if directory exists
			if ( !fs::is_directory( dpath ) ) {
				fprintf( stdout, "Creating directory for %s\n", dpath.string( ).c_str( ) );
				// create directory and sub directory if it doesn't exist
				fs::create_directories( dpath );
			}
			
		} else {
			// check if directory exists
			if ( !fs::is_directory( upath ) ) {
				fprintf( stdout, "Creating directory for %s\n", upath.string( ).c_str( ) );
				// create directory and sub directory if it doesn't exist
				fs::create_directories( upath );
			}
		}
		
		return 1;
		
	} catch ( const std::filesystem::filesystem_error& ex ) {
		fprintf( stderr, "%s directory create error %s \n", path.c_str(), ex.what() );
		return -1;
	}
}

 void _create_log_path( std::string& result ) {
	std::time_t now = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now( ) );
	std::string date_str( 30, '\0' );
	size_t size = std::strftime( &date_str[0], date_str.size( ), "%Y_%m_%d", std::localtime( &now ) );
	date_str.resize( size );
	result.append( date_str.c_str( ) ).append( ".log" );
	return;
}

void _create_time( std::string& result ) {
	// print 13:14:48.686
	auto now = std::chrono::system_clock::now( );
	std::time_t now_c = std::chrono::system_clock::to_time_t( now );
	std::tm tm_c = *std::localtime( &now_c );
	std::string time_str( 20, '\0' );
	size_t size = std::snprintf( &time_str[0], time_str.size( ), "%02d:%02d:%02d.%03d", tm_c.tm_hour, tm_c.tm_min, tm_c.tm_sec, static_cast<int>( std::chrono::duration_cast<std::chrono::milliseconds>( now.time_since_epoch( ) ).count( ) % 1000 ) );
	time_str.resize( size );
	time_str.swap( result );
}

svc_logger::svc_logger( ){ }

svc_logger::~svc_logger( ) { }

void svc_logger::_write_stream( const char* data, size_t size ) {

	std::cout << data;

	if ( _write_byte >= MAX_SIZE ) {
		return;
	}
	_write_byte += size;
	_out->write( data, size );
	if ( _write_byte >= MAX_SIZE ) {
		_out->write( "\nMAX_SIZE_EXCCEDED\n", 19 );
	}

}

void svc_logger::_write( const std::string& data ) {
	_write_stream( data.c_str( ), data.size( ) );
}

void svc_logger::_write_time( ) {
	std::time_t now = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now( ) );
	std::string time_str( 30, '\0' );
	// print 2023-04-07 12:28:44
	size_t size = std::strftime( &time_str[0], time_str.size( ), "%Y-%m-%d %H:%M:%S", std::localtime( &now ) );
	time_str.resize( size );
	_write( time_str );
	return;
}

void svc_logger::_write_log_time( ) {
	// print 13:14:48.686
	std::string time_str;
	_create_time( time_str );
	time_str.append( "\t" );
	_write( time_str );
}

void svc_logger::_write_info( int level ) {
	_write_log_time( );
	switch ( level ) {
		case SVC_LOGGER_DEBUG:
			_write_stream( "DEBUG\t", 6 ); break;
		case SVC_LOGGER_ERROR:
			_write_stream( "FATAL\t", 6 ); break;
		case SVC_LOGGER_INFO:
		default: _write_stream( "INFO\t", 5 ); break;
	}
}

void svc_logger::_write_intro( bool is_exists ) {
	
	std::string str( "" );
	str.insert( 0, 65, '-' );
	str.append( "\n" );
	
	if ( is_exists ) {
		
		_write_stream( str.c_str( ), str.size( ) );
		
	} else {
		
		_write( str );
		_write( "This Log generated at " );
		
		_write_time( );
		
		std::string f( " for " );
		f.append( "Service Manager" ).append( " " ).append( APP_VERSION"\n" );
		_write( f );
		
		_write( str );
	}
	
	_need_flush++;
}

int svc_logger::open( ) {
	
	_out = std::make_shared<std::ofstream>( );
	std::string l_path( "./svcm/log/" );
	
	_create_log_path( l_path );
	
	if ( _create_log_directory( l_path ) < 0 ) {
		return -1;
	}
	
	bool is_exists = fs::exists( l_path );
	_out->open( l_path, std::ios::out | std::ios::app );
	
	_write_intro( is_exists );

	return 1;
	
}

void svc_logger::renew() {
	
    // Log the start of the logger switching process
    write(SVC_LOGGER_INFO, "Logger Switching\n");
    
    // Ensure all pending logs are written to disk
    flush();

    // Close the current log file
    close();

    // Open a new log file for logging
    open();

    // Log the completion of the renewal process
    write(SVC_LOGGER_INFO, "Logger Renewed\n");

    // Flush to ensure the log entry is written immediately
    flush();
}


void svc_logger::flush( ) {
	
	if ( _need_flush < 1 )return;
	
	if ( _out->good( ) ) {
		_out->flush( );
	}
	
	_need_flush = 0;

}

void svc_logger::close( ) {
	
	_need_flush = 0;
	
	if ( _out->good( ) ) {
		_out->close( );
	}

	_write_byte = 0;
}
	
void svc_logger::write( int log_label, const std::string& message ) {

	if ( _need_flush == 0 ) {
		_need_flush++;
	}
	
	if ( _out->good( ) ) {
		_write_info( log_label );
		_write_stream( message.c_str(), message.size() );
	}

}