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

// 11:20 AM 2/17/2025
// by Rajib Chy
#ifdef _MSC_VER
#pragma once
#endif//_MSC_VER

#ifndef svc_dust_cleaner_h
#define svc_dust_cleaner_h

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>
#include <memory>
#include <svc/logger.h>

namespace fs = std::filesystem;

/**
 * @brief Configuration for dust cleaning operation.
 */
struct dust_clean_config {
    bool is_cache = false; /**< Indicates whether the file is a cache file. */
    std::string dust_dir; /**< Directory path to clean. */
    bool delete_empty_dir = false; /**< Flag to remove empty directories. */
    std::vector<std::string> extensions; /**< List of file extensions to clean. */
    // JSON serialization/deserialization
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(dust_clean_config, is_cache, dust_dir, delete_empty_dir, extensions)
};

/**
 * @brief Parses a JSON object into a dynamically allocated dust_clean_config structure.
 * 
 * @param j The JSON object containing the configuration data.
 * @param cfg A pointer to a dust_clean_config structure. If nullptr, a new instance is allocated.
 * 
 * @note If the "dir" field is missing or empty, the function sets it to the current directory.
 */
void _dust_config_from_json(const nlohmann::json& j, dust_clean_config*& cfg);

/**
 * @brief Handles cleaning of log files and empty directories.
 */
class dust_cleaner_t {

public:
    dust_cleaner_t( ) { }
    ~dust_cleaner_t( );
    dust_cleaner_t( dust_cleaner_t&& ) = default;
    dust_cleaner_t& operator=( dust_cleaner_t&& ) = delete;

private:
    std::atomic<bool> _is_disposed{false}; /**< Indicates if the cleaner has been disposed. */
    std::vector<dust_clean_config*> _dust_configs; /**< List of configurations for cleaning. */

    /**
     * @brief Determines if a file is old enough to be deleted.
     * @param creation_time File creation time.
     * @return True if the file should be deleted, false otherwise.
     */
    bool is_deletable( const fs::file_time_type& creation_time );

    /**
     * @brief Checks if a file needs to be deleted based on extension and age.
     * @param file_path Path to the file.
     * @param ext File extension to match.
     * @return True if the file should be deleted, false otherwise.
     */
    bool need_delete( const fs::path& file_path, const std::string& ext );

    /**
     * @brief Deletes log files from the specified directory.
     * @param logger Output stream for logging.
     * @param dir Directory to clean.
     * @param ext File extension to delete.
     * @param is_cache Indicates if the file is a cache file.
     */
    void delete_log_files( std::shared_ptr<svc_logger>& logger, const fs::path& dir, const std::string& ext, bool is_cache );

    /**
     * @brief Recursively deletes empty directories.
     * @param dir Directory to check.
     * @param logger Output stream for logging.
     */
    void clean_empty_dirs( const fs::path& dir, std::shared_ptr<svc_logger>& logger );

    /**
     * @brief Starts the cleaning process in a separate thread.
     */
    void start_clean( std::shared_ptr<svc_logger>& logger );

public:
    bool is_empty( ) const;
    /**
     * @brief Initiates the cleaning process.
     */
    void clean( std::shared_ptr<svc_logger>& logger );

    /**
     * @brief Sets the configuration for dust cleaning.
     * @param configs List of cleaning configurations.
     */
    void set_dust_config( std::vector<dust_clean_config*>& configs );

    /**
     * @brief Disposes the cleaner instance.
     */
    void dispose( );
};

#endif // svc_dust_cleaner_h
