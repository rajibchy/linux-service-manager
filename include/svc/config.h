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

// 11:22 AM 2/13/2025
// by Rajib Chy

#ifdef _MSC_VER
#pragma once
#endif//_MSC_VER

#ifndef _fsys_svc_config_h
#define _fsys_svc_config_h

#include <cstring>
#include <string>
#include <vector>
#include <svc/time-range.h>
#include <svc/dust-cleaner.h>

/**
 * @enum service_state
 * @brief Represents the state of a service.
 */
enum class service_state {
    ACTIVE,   ///< Service is currently active and running.
    INACTIVE, ///< Service is not running.
    ERROR     ///< Service encountered an error state.
};

struct svc_config {
    bool is_restarted = false;
    bool required_workday = false;
    bool is_restart_support = false;
    bool has_dependent_service = false;
    std::string service_name; // fixc_dse.service
    std::string start_time;// "08:30:15";
    std::string end_time;// "23:10:15";
    std::string restart_time;
    time_range_t* time_range = nullptr; // time_range_t
    service_state state = service_state::INACTIVE;
    std::vector<std::string> dependent; /**< List of dependent service. */
};


#ifdef USE_HTTP_DAY_STATUS

void _load_config( 
    std::vector<svc_config*>& svc_configs, 
    std::vector<dust_clean_config*>& dust_configs,
    std::string& http_server, std::string& http_port 
);

#else

void _load_config( 
    std::vector<svc_config*>& svc_configs, 
    std::vector<dust_clean_config*>& dust_configs
);

#endif //!USE_HTTP_DAY_STATUS

#endif //!_fsys_svc_config_h