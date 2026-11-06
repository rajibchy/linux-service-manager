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

// 4:17 PM 4/23/2023
// by Rajib Chy
#include <svc/json-config.h>
#include <exception>
#include <sstream> // istringstream, ostringstream
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

int _read_file( const std::string& path, std::string& result ) {
	int status = 0;
	if ( !fs::exists( path ) ) return status;
	std::ifstream file( path );
	if ( file.is_open( ) ) {
		std::stringstream buffer;
		buffer << file.rdbuf( );
		buffer.str( ).swap( result );
		file.close( );
		status = 1;
	}
	return status;
}

json_config_t::~json_config_t( ) {
	clear( );
}

json_config_t::json_config_t( const std::string& path, int is_array ) {
	std::string buff;
	
	if ( _read_file( path, buff ) == 0 ) {
		throw std::runtime_error("Unable to open config file");
	}

	_data = json::parse( buff, nullptr, false );

	std::string( ).swap( buff );

	if ( is_array == 1 ) {
		if ( _data.is_null( ) || !_data.is_array( ) ) {
			throw std::runtime_error( "Non Array config found. data type Array required." );
		}
		_data_type = json_value_t::array;
	} else {
		if ( _data.is_null( ) || !_data.is_object( ) ) {
			throw std::runtime_error( "Non object config found. data type object required." );
		}
		_data_type = json_value_t::object;
	}
	
}

json_config_t::json_config_t( json& data, int is_array ) {
	if ( is_array == 1 ) {
		if ( data.is_null( ) || !data.is_array( ) ) {
			throw std::runtime_error( "Non Array config found. data type Array required." );
		}
		_data_type = json_value_t::array;
	} else {
		if ( data.is_null( ) || !data.is_object( ) ) {
			throw std::runtime_error( "Non object config found. data type object required." );
		}
		_data_type = json_value_t::object;
	}
	_data = data;
}

int json_config_t::get_string( const char* key, std::string& result ) {
	throw_if_not_object( );

	if ( !_data.contains( key ) )return 0;
	
	_data.at(key).get_to(result);

	// ( _data[key].get<std::string>( ) ).swap( result );
	return 1;
}
int json_config_t::get_int( const char* key, int* result ) {
	throw_if_not_object( );

	if ( !_data.contains( key ) )return 0;

	// *result = _data[key].get<int>( );

	_data.at(key).get_to(*result);

	return 1;
}

int json_config_t::get_bool(const char* key, bool* result) {
	throw_if_not_object( );

	if ( !_data.contains( key ) )return 0;

	// *result = _data[key].get<double>( );

	_data.at(key).get_to(*result);

	return 1;
}

int json_config_t::get_double( const char* key, double* result ) {
	throw_if_not_object( );

	if ( !_data.contains( key ) )return 0;

	// *result = _data[key].get<double>( );

	_data.at(key).get_to(*result);

	return 1;
}

int json_config_t::get_next_part( const char* key, json_config_t& part, int is_array ) {
	throw_if_not_object( );
	if ( !_data.contains( key ) )return 0;
	part = json_config_t( _data[key], is_array );
	return 1;
}

void json_config_t::each( std::function<void( json_config_t& )> next ) {
	throw_if_not_array( );

	// Iterate over each key-value pair in the JSON Array

	for ( auto& [key, val] : _data.items( ) ) {
		// Call the provided callback function with the json_config_t
		json_config_t* d = new json_config_t( val, val.is_array( ) ? 1 : 0 );
		next( *d );
		delete d;
	}

}

void json_config_t::each_keys(std::function<void(const std::string& key, json &val)> next){
	
	// Iterate over each key-value pair in the JSON object
	for ( auto& [key, val] : _data.items( ) ) {
		// Call the provided callback function with the key and value
		next( key, val );
	}
	
}
void json_config_t::clear( ) {
	_data.clear( );
}

void json_config_t::throw_if_not_array( ) {
	if ( _data_type != json_value_t::array ) {
		throw std::runtime_error( "Config data should be Array." );
	}
}

void json_config_t::throw_if_not_object( ) {
	if ( _data_type != json_value_t::object ) {
		throw std::runtime_error( "Config data should be Object." );
	}
}
