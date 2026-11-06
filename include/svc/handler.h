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

// 11:26 AM 2/13/2025
// by Rajib Chy

#ifdef _MSC_VER
#pragma once
#endif//_MSC_VER

#ifndef _fsys_svc_handler_h
#define _fsys_svc_handler_h

#include <cstring>
#include <iostream>
#include <memory>
#include <vector>
#include <atomic>
#include <future>
#include <chrono>  // Required for std::chrono::seconds
#include <svc/config.h>

#ifdef USE_HTTP_DAY_STATUS
#include <svc/httpc.h>
#endif //!USE_HTTP_DAY_STATUS

#include <svc/logger.h>
#include <svc/time-range.h>
#include <svc/manager.h>
#include <svc/dust-cleaner.h>

/**
 * @class service_handler_t
 * @brief Manages the lifecycle of multiple services, including start, stop, and monitoring.
 * 
 * The service handler is responsible for controlling and monitoring services. It provides
 * methods to initialize, prepare, block execution, and exit gracefully. Additionally,
 * it manages logging and HTTP interactions.
 */
class service_handler_t {

public:

    /**
     * @brief Constructs a new service_handler_t object.
     * 
     * Initializes internal resources and prepares the service handler for operation.
     */
    service_handler_t();

    /**
     * @brief Destroys the service_handler_t object.
     * 
     * Ensures proper cleanup of dynamically allocated resources.
     */
    ~service_handler_t();

    /**
     * @brief Gracefully exits the service.
     * 
     * Signals all running services to stop and ensures resources are cleaned up before termination.
     */
    void exit();

    /**
     * @brief Blocks execution until an exit signal is received.
     * 
     * @return int Returns 0 if execution was successful, or an error code otherwise.
     */
    int block();

    /**
     * @brief Prepares the service for operation.
     * 
     * This function initializes configurations, verifies dependencies, and sets up necessary resources.
     * 
     * @return int Returns 0 if preparation is successful, or an error code otherwise.
     */
    int prepare();

private:

    /**
     * @brief Handles the transition to a new trading day.
     * 
     * This function checks if the current date has changed compared to the last recorded date.
     * If a new day has started, it updates the last recorded date, renews the logger, loads the
     * day's status, cleans up old data if necessary, and prepares the time ranges for all services.
     * 
     * @return 1 if the transition is successful, 0 if it fails to load the day status.
     */
    int switch_to_new_day( );
    
    /**
     * @brief Updates the current state of all services.
     *
     * This function iterates over the list of services and checks their status.
     * If a service is determined to be in the `ACTIVE` state, its state is explicitly
     * set to `service_state::ACTIVE`. 
     *
     * @note This function does not handle services transitioning to other states.
     */
    void update_service_current_state();
    
    /**
     * @brief Waits for a specified duration.
     * 
     * This function suspends execution for a given number of milliseconds.
     * 
     * @param ms The number of milliseconds to wait.
     * @return int Returns 1 on success, or an error code if the wait is interrupted.
     */
    int wait_for( long ms );

    /**
     * @brief Starts the specified service.
     * 
     * @param service Reference to the service configuration to be started.
     */
    void start_service(svc_config& service);

    /**
     * @brief Restarts the given service.
     * 
     * This function stops and then starts the given service, ensuring that
     * it properly restarts if it supports restart functionality.
     * 
     * @param service The service configuration object to restart.
     */
    void restart_service(svc_config& service);

    /**
     * @brief Toggles the state of dependent services based on the current time and stop flag.
     * 
     * This function iterates over a list of dependent services and determines whether to start 
     * or stop them based on their current state, operational time range, and a stop flag. 
     * 
     * Behavior:
     * - If `stop` is `true`:
     *   - Stops services that are **not already inactive**.
     *   - Recursively stops **dependent services** before stopping the current service.
     *   - Waits for dependent services to stop before proceeding.
     * - If `stop` is `false`:
     *   - Starts services that are **inactive** and **within their operational time range**.
     *   - Recursively starts **dependent services** after the current service is restarted.
     *   - Waits for dependent services to start before proceeding.
     * 
     * Exit Condition:
     * - If `_exit_flag` is set (`1`), the iteration is terminated immediately.
     * - If waiting for a dependent service (`wait_for(10000)`) times out, the function exits early.
     * 
     * @param root_service The name of the root service whose dependencies are being toggled.
     * @param dependent A list of dependent service names that need to be checked.
     * @param now_time The current system time, used to determine whether a service is within its operational time range.
     * @param stop A boolean flag indicating whether to stop (`true`) or start (`false`) the dependent services.
     * @return The number of services that were successfully toggled (started or stopped).
     */
    int toggel_dependent_service(
        const std::string& root_service,
        const std::vector<std::string>& dependent, 
        const std::time_t& now_time, 
        bool stop
    );



    /**
     * @brief Stops the specified service.
     * 
     * @param service Reference to the service configuration to be stopped.
     */
    void stop_service(svc_config& service);

    /**
     * @brief Retrieves the current status of a given service.
     * 
     * @param service Reference to the service configuration.
     * @return service_state The current state of the service (ACTIVE, INACTIVE, or ERROR).
     */
    service_state get_service_status(const svc_config& service);

#ifdef USE_HTTP_DAY_STATUS
    /**
     * @brief Attempts to load the trading day status from the server.
     * 
     * This function makes an HTTP GET request to retrieve the trading day status. 
     * It retries up to 10 times with an exponential backoff if the request fails.
     * 
     * @return int Returns 1 if the request succeeds, or 0 if all retries fail.
     */
    int load_day_status();

    /**
     * @brief Loads the last trade date from the cache and determines the working day status.
     *
     * This function retrieves the last recorded trade date from the cache file. 
     * If a valid trade date is found, it checks whether the current date (`_last_date`) 
     * matches the cached trade date to determine if it is a working day.
     *
     * @return int Returns 1 if a valid trade date is found and processed successfully.
     *             Returns 0 if the cache file is missing, invalid, or unreadable.
     *
     * @note If the cache does not contain a valid trade date, the function returns 0 without setting `_is_working_day`.
     *       This function relies on `load_last_trade_date()` for fetching the cached trade date.
     */
    int load_day_status_fallback();

    /**
     * @brief Loads the last trade date from the cache file.
     *
     * Reads the cached trade date data from the file and validates it.
     * If the cache file does not exist or contains invalid data, the function returns 0.
     * If the file contains valid trade data and the stored date matches the current date, it returns 1.
     *
     * @param trade_date Reference to a string where the trade date will be stored.
     * @return int 1 if a valid trade date for the current day is found, otherwise 0.
     */
    int load_last_trade_date( std::string& trade_date );

    /**
     * @brief Saves the last trade date to the cache file.
     *
     * This function writes the current date and the given trade date to the file.
     * The format used is: "YYYY-MM-DD~TRADE_DATE".
     *
     * @param trade_date The trade date to be saved.
     */
    void save_last_trade_date( const std::string& trade_date );

#endif //!USE_HTTP_DAY_STATUS

public:
    std::shared_ptr<svc_logger> _logger; ///< Logger instance for logging service activity.

private:
    bool _is_working_day;  ///< Indicates whether the current day is a working day.
    std::string _last_date; ///< Stores the last recorded date.
    std::future<void> _future; ///< Handles asynchronous operations.
#ifdef USE_HTTP_DAY_STATUS
    http_client* _http = nullptr; ///< HTTP client for server communication.
#endif //!USE_HTTP_DAY_STATUS
    std::atomic<int> _exit_flag = 0; ///< Flag to indicate service exit status.
    dust_cleaner_t* _cleaner = nullptr;
    std::vector<svc_config*> _services; ///< List of service configurations.
    service_manager_t* _svc_manager = nullptr; ///< Pointer to the service manager instance.
    std::shared_ptr<std::promise<void>> _promise; ///< Promise object for managing async operations.
};

#endif //!_fsys_svc_handler_h