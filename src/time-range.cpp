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

// 10:47 AM 2/13/2025
// by Rajib Chy

#include <svc/time-range.h>
#include <regex>
#include <cstring> // Include for memset

/**
 * @brief Retrieves the current system date in "YYYY-MM-DD" format.
 *
 * This function gets the current system date, formats it as "YYYY-MM-DD",
 * and stores it in the provided string reference.
 *
 * @param[out] result A string reference where the formatted date will be stored.
 *
 * @note The function uses thread-safe versions of `localtime` (localtime_s for Windows, localtime_r for POSIX).
 */
void _get_current_date( std::string& result ) {
    auto now = std::chrono::system_clock::now( );
    std::time_t now_c = std::chrono::system_clock::to_time_t( now );
    std::tm tm_struct;

#ifdef _WIN32
        localtime_s( &tm_struct, &now_c ); // Windows-safe localtime
#else
        localtime_r( &now_c, &tm_struct ); // POSIX-safe localtime
#endif //!_WIN32

    std::ostringstream oss;
    oss << std::put_time( &tm_struct, "%Y-%m-%d" );
    oss.str( ).swap( result );
}

// Regular expression to match YYYY-mm-dd format
const std::regex date_pattern(R"(^(\d{4})-(\d{2})-(\d{2})$)");

// Days in each month (index 0 is unused)
const int days_in_month[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/**
 * @brief Validates if a given date string follows the "YYYY-MM-DD" format and represents a real calendar date.
 *
 * This function first checks if the input string follows the correct format using a regular expression.
 * Then, it extracts the year, month, and day, validates the month range (1-12), 
 * and verifies that the day value is within the expected range, considering leap years.
 *
 * @param[in] date_str The input date string in "YYYY-MM-DD" format.
 * @return `true` if the date is valid, `false` otherwise.
 */
bool _is_valid_date( const std::string& date_str ) {
    
    std::smatch match;

    if ( !std::regex_match( date_str, match, date_pattern ) ) {
        return false; // Format is incorrect
    }

    // Extract year, month, and day as integers
    int year, month, day;
    std::istringstream( match[1] ) >> year;
    std::istringstream( match[2] ) >> month;
    std::istringstream( match[3] ) >> day;

    // Validate month (must be 1-12)
    if (month < 1 || month > 12) return false;

    // Check for leap year and adjust February days
    bool is_leap_year = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    int max_days = (month == 2 && is_leap_year) ? 29 : days_in_month[month];

    // Validate day (must be within valid range)
    return (day >= 1 && day <= max_days);
}

/**
 * @brief Converts a time string (HH:MM:SS) into a `std::tm` structure with today's date.
 *
 * This function parses a time string formatted as "HH:MM:SS" and fills a `std::tm` structure.
 * It also assigns the current date (year, month, day) to ensure a valid `std::tm` value.
 *
 * @param[in] time_str The time string in the format "HH:MM:SS".
 * @param[out] tm The `std::tm` structure that will be populated with the parsed time and today's date.
 * 
 * @throws std::runtime_error If the time string cannot be parsed.
 */
void _string_to_tm( const std::string& time_str, std::tm& tm ) {

    std::istringstream ss( time_str );
    ss >> std::get_time( &tm, "%H:%M:%S" );

    if ( ss.fail( ) ) {
        throw std::runtime_error( "Failed to parse time string" );
    }

    // Set a valid date (e.g., today) to avoid mktime() issues
    std::time_t t = std::time(nullptr);
    std::tm now;
#ifdef _WIN32
    localtime_s(&now, &t);  // Windows thread-safe version
#else
    localtime_r(&t, &now);  // POSIX thread-safe version
#endif //!_WIN32

    // Set current date
    tm.tm_year = now.tm_year;
    tm.tm_mon  = now.tm_mon;
    tm.tm_mday = now.tm_mday;

    return;
}

/**
 * @brief Converts a `std::tm` structure into a `std::time_t` epoch timestamp.
 *
 * @param[in] tm The `std::tm` structure representing a date and time.
 * @return std::time_t The corresponding epoch time (seconds since Unix epoch).
 */
std::time_t _tm_to_time_t( std::tm& tm ) {
    return std::mktime( &tm );
}

/**
 * @brief Converts a time string (HH:MM:SS) into a time_t epoch value for today's date.
 *
 * This function takes a time string in the format "HH:MM:SS", parses it into a `std::tm` structure,
 * and sets the current date before converting it into an epoch time (`std::time_t`).
 *
 * @param[in] time_str The input time string in the format "HH:MM:SS".
 * @param[out] epoch The output epoch time (seconds since Unix epoch) for today at the given time.
 *
 * @throws std::runtime_error if the input time string is not in a valid "HH:MM:SS" format.
 *
 * @note The function assumes the system's local timezone unless modified explicitly.
 */
void _convert_to_epoch( const std::string& time_str, std::time_t& epoch ) {
        // guaranteed zero initialization
        std::tm start_tm;
        memset( &start_tm, 0, sizeof( std::tm ) );

        // Convert time strings to struct tm
        _string_to_tm( time_str, start_tm );

        // Convert struct tm to time_t for easier time-based comparisons
        epoch =  _tm_to_time_t( start_tm );
}

/**
 * @brief Formats a given epoch time (time_t) into a human-readable string representation.
 *
 * This function takes an epoch time value, converts it into a `std::tm` structure representing
 * the local time, and then formats it into a string with the format: "%a %b %d %H:%M:%S %Y".
 * The formatted string is stored in the provided `std::string& result`.
 *
 * The conversion to local time is performed using the thread-safe `localtime_s` (Windows) or 
 * `localtime_r` (POSIX) functions to ensure correct handling in multi-threaded environments.
 *
 * @param epoch_time The epoch time (time_t) to be converted and formatted.
 * @param result A reference to a `std::string` where the formatted time will be stored.
 *               This string is updated with the formatted date and time.
 *
 * @note The format used is "%a %b %d %H:%M:%S %Y" (e.g., "Sat Feb 22 07:54:00 2025").
 *       The `result` string will contain the formatted time string after this function completes.
 */
void _format_time(const std::time_t& epoch_time, std::string& result) {
    std::tm tm_struct;
#ifdef _WIN32
    localtime_s(&tm_struct, &epoch_time);  // Windows thread-safe version
#else
    localtime_r(&epoch_time, &tm_struct);  // POSIX thread-safe version
#endif //!_WIN32

    std::ostringstream oss;
    oss << std::put_time( &tm_struct, "%a %b %d %H:%M:%S %Y" );

    oss.str( ).swap( result );

}

void time_range_t::print( std::shared_ptr<svc_logger>& logger ) const {

    if ( _start_epoch == 0 || _end_epoch == 0 ){
        logger->debug( "Service is running in uninterrupted mode." );
    } else {

        std::string start_time_str;
        _format_time( _start_epoch, start_time_str );

        std::string end_time_str;
        _format_time( _end_epoch, end_time_str );

        // Verify that both times are different
        if ( start_time_str != end_time_str ) {
            logger->debug( "Scheduled Start: ", start_time_str, " and End: ", end_time_str );
        } else {
            logger->debug( "Start and End times are the same: ", start_time_str );
        }

    }

    if( _restart_epoch > 0 ) {

        std::string restart_time_str;
        _format_time( _restart_epoch, restart_time_str );
        logger->debug( "Scheduled restart at: ", restart_time_str );

    }
    
}

constexpr char EMPTY_TIME[] = "00:00:00";

time_range_t::time_range_t( const std::string& start_time, const std::string& end_time, const std::string& restart_time ) {

    _end_time = std::string( end_time );
    _start_time = std::string( start_time );
    _restart_time = std::string( restart_time );

    prepare( );
}

bool time_range_t::is_restart_supported( ) const {
    return _restart_epoch > 0;
}

void time_range_t::prepare() {

    if ( _restart_time.empty() || _restart_time == EMPTY_TIME ) {
        // If time values are invalid, reset epochs to 0
        _restart_epoch = 0;
    } else {
        _convert_to_epoch( _restart_time, _restart_epoch );
    }

    // Check if either start time or end time is empty or set to EMPTY_TIME
    if ( _start_time.empty() || _start_time == EMPTY_TIME || 
        _end_time.empty() || _end_time == EMPTY_TIME) {
        
        // If time values are invalid, reset epochs to 0
        _start_epoch = 0;
        _end_epoch = 0;

    } else {

        _convert_to_epoch( _end_time, _end_epoch );
        _convert_to_epoch( _start_time, _start_epoch );

    }
}

bool time_range_t::need_restart( const std::time_t& now_time ) const {
    if ( _restart_epoch == 0) return false;
    std::time_t nearest_time = _restart_epoch + 60; // 60 seconds after _restart_epoch

    return ( now_time >= _restart_epoch && now_time <= nearest_time );
}

bool time_range_t::is_between_times(const std::time_t& now_time) const {
    if ( _start_epoch == 0 || _end_epoch == 0 ) return true;
    return ( now_time >= _start_epoch && now_time <= _end_epoch );
}

// bool time_range_t::is_between_times() {
    
//     if ( _start_epoch == 0 || _end_epoch == 0 ) return true;

//     auto now = std::chrono::system_clock::now( );
//     std::time_t now_time = std::chrono::system_clock::to_time_t( now );

//     return ( now_time >= _start_epoch && now_time <= _end_epoch );
// }
