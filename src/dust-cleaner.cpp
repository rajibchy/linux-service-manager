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

// 11:23 AM 2/17/2025
// by Rajib Chy

#include <svc/dust-cleaner.h>
#include <fstream>

bool dust_cleaner_t::is_deletable( const fs::file_time_type& creation_time ) {
    using namespace std::chrono;

    auto now = system_clock::now( );
    auto fileTime = time_point_cast<system_clock::duration>( creation_time - fs::file_time_type::clock::now( ) + system_clock::now( ) );
    
    auto elapsed = duration_cast<hours>( now - fileTime ).count( );
    return elapsed >= ( 5 * 24 );
}


bool dust_cleaner_t::need_delete( const fs::path& filePath, const std::string& ext ) {
    try {
        if (filePath.extension() != ext) return false;
        auto ftime = fs::last_write_time(filePath);
        return is_deletable(ftime);
    } catch (...) {
        return false;
    }
}

constexpr char CACHE_KEY[] = "/cache/";

void dust_cleaner_t::delete_log_files( 
    std::shared_ptr<svc_logger>& logger, const fs::path& dir, 
    const std::string& ext, bool is_cache 
) {
    
    if ( !fs::exists( dir ) || !fs::is_directory( dir ) ) {
        logger->info( "Directory not found: ", dir );
        return;
    }

    for ( const auto& entry : fs::recursive_directory_iterator( dir ) ) {

        const std::filesystem::path & path = entry.path( );

        if ( !fs::is_regular_file( path ) ) continue;

        if ( is_cache ) {

            const std::string& path_str = path.string();
            
            if ( path_str.find( CACHE_KEY ) == std::string::npos ) {
                continue;
            }

        }
		
        
        if ( !need_delete( path, ext ) ) continue;
        
        logger->info( "Deleting file: ", path );

        try {

            fs::remove( path );

        } catch ( const std::exception& e ) {

            logger->error( "Unable to delete file ", path, " due to: ", e.what() );

        }

    }
}

void dust_cleaner_t::clean_empty_dirs( const fs::path& dir, std::shared_ptr<svc_logger>& logger ) {
    
    for ( const auto& entry : fs::directory_iterator( dir ) ) {

        if ( fs::is_directory( entry ) ) {

            clean_empty_dirs( entry.path( ), logger );

            if ( fs::is_empty( entry.path( ) ) ) {

                fs::remove( entry.path( ) );
                logger->info( "Deleted empty directory: ", entry.path() );

            }
        }
    }

}

void dust_cleaner_t::start_clean( std::shared_ptr<svc_logger>& logger ) {
    
    logger->info( "Starting \"Dust Cleaner\"..." );

    for ( const auto& config : _dust_configs ) {

        if ( config->extensions.empty( ) ) continue;

        fs::path root_dir( config->dust_dir );

        if ( !fs::exists( root_dir ) || !fs::is_directory( root_dir ) ) {
            logger->info( "Root dir not found: ", root_dir );
            continue;
        }

        for ( const auto& ext : config->extensions ) {
            delete_log_files( logger, root_dir, ext, config->is_cache );
        }

        if ( config->delete_empty_dir ) {
            clean_empty_dirs( root_dir, logger );
        }
    }

    logger->info( "End \"Dust Cleaner\"..." );

}

void dust_cleaner_t::clean( std::shared_ptr<svc_logger>& logger ) {
    start_clean( logger );
}

void dust_cleaner_t::set_dust_config( std::vector<dust_clean_config*>& configs ) {
    configs.swap( _dust_configs );
}

bool dust_cleaner_t::is_empty( ) const {
    return _dust_configs.empty( );
}

void dust_cleaner_t::dispose() {
    if ( _is_disposed ) return;
    _is_disposed = true;
    _dust_configs.clear();
}

dust_cleaner_t::~dust_cleaner_t( ) {
    dispose( );
}

void _set_current_dir( std::string& dir ) {
    dir = std::filesystem::current_path().string();
}

void _dust_config_from_json( const nlohmann::json& j, dust_clean_config*& cfg ) {

    if ( !cfg ) cfg = new dust_clean_config; // Allocate memory if null

    if( j.contains( "is_cache" ) ) {
        j.at( "is_cache" ).get_to( cfg->is_cache );
    }

    if( j.contains( "dir" ) ) {
        j.at( "dir" ).get_to( cfg->dust_dir );
    }

    if ( cfg->dust_dir.empty() ) {
        _set_current_dir( cfg->dust_dir );
    }

    if( j.contains( "delete_empty_dir" ) ) {
        j.at( "delete_empty_dir" ).get_to( cfg->delete_empty_dir );
    }

    if( j.contains( "ext" ) ) {
        j.at( "ext" ).get_to( cfg->extensions );
    }
}