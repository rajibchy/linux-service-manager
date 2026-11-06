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

// 1:21 AM 1/12/2025
// by Rajib Chy

#ifdef _MSC_VER
#pragma once
#endif//_MSC_VER

#ifndef _svc_logger_h
#define _svc_logger_h

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>

#ifndef APP_VERSION
#define APP_VERSION "3.0.10.200"
#endif //!APP_VERSION

/**
 * @file svc_logger.h
 * @brief Service Logger Class
 *
 * This class provides logging functionality with different severity levels.
 * It supports formatted logging using variadic templates and writes logs to a file.
 */

constexpr int SVC_LOGGER_INFO = 1;   ///< Log level for informational messages
constexpr int SVC_LOGGER_DEBUG = 2;  ///< Log level for debug messages
constexpr int SVC_LOGGER_ERROR = 3;  ///< Log level for error messages

/**
 * @class svc_logger
 * @brief A simple logging utility class.
 *
 * The svc_logger class handles writing logs to a file with different severity levels.
 * It supports variadic templates for formatted messages and ensures efficient file writing.
 */
class svc_logger {
public:
    /**
     * @brief Constructs a new svc_logger instance.
     */
    svc_logger();

    /**
     * @brief Destroys the svc_logger instance.
     */
    ~svc_logger();

    /**
     * @brief Opens the log file for writing.
     * @return Returns 0 on success, or an error code on failure.
     */
    int open();

    /**
     * @brief Flushes any buffered log data to the file.
     */
    void flush();

    /**
     * @brief Closes the log file.
     */
    void close();

    /**
     * @brief Renews the logger by closing and reopening the log file.
     * 
     * This function writes a log entry indicating the logger is switching,
     * flushes any pending logs, closes the current log file, reopens a new log file,
     * and logs that the renewal process is complete.
     */
    void renew();

    /**
     * @brief Writes a log message with a specified log level.
     * @param log_label The log level (INFO, DEBUG, ERROR).
     * @param message The log message.
     */
    void write(int log_label, const std::string& message);

    /**
     * @brief Logs an informational message.
     * @tparam Args Variadic template parameters for formatted messages.
     * @param message The log message format.
     * @param args Arguments to format into the message.
     */
    template <typename... Args>
    void info(const std::string& message, Args&&... args) {
        prepare_message(SVC_LOGGER_INFO, message, std::forward<Args>(args)...);
    }

    /**
     * @brief Logs a debug message.
     * @tparam Args Variadic template parameters for formatted messages.
     * @param message The log message format.
     * @param args Arguments to format into the message.
     */
    template <typename... Args>
    void debug(const std::string& message, Args&&... args) {
        prepare_message(SVC_LOGGER_DEBUG, message, std::forward<Args>(args)...);
    }

    /**
     * @brief Logs an error message.
     * @tparam Args Variadic template parameters for formatted messages.
     * @param message The log message format.
     * @param args Arguments to format into the message.
     */
    template <typename... Args>
    void error(const std::string& message, Args&&... args) {
        prepare_message(SVC_LOGGER_ERROR, message, std::forward<Args>(args)...);
    }

private:
    /**
     * @brief Prepares and formats a log message before writing.
     * @tparam Args Variadic template parameters for additional message details.
     * @param log_level The log severity level.
     * @param message The primary log message.
     * @param args Additional arguments to append to the message.
     */
    template <typename... Args>
    void prepare_message(int log_level, const std::string& message, Args&&... args) {
        std::ostringstream log_stream;
        log_stream << message;
        (append_to_stream(log_stream, std::forward<Args>(args)), ...);
        log_stream << "\n";
        write(log_level, log_stream.str());
    }

    /**
     * @brief Appends an argument to the log message stream.
     * @tparam T The type of argument to append.
     * @param stream The log message stream.
     * @param arg The argument to append.
     */
    template <typename T>
    void append_to_stream(std::ostringstream& stream, T&& arg) {
        stream << arg;
    }

    /**
     * @brief Writes a timestamp to the log file.
     */
    void _write_time();

    /**
     * @brief Writes the log timestamp.
     */
    void _write_log_time();

    /**
     * @brief Writes log level info.
     * @param level The log level (INFO, DEBUG, ERROR).
     */
    void _write_info(int level);

    /**
     * @brief Writes introductory log messages (e.g., log session start).
     * @param is_exists Indicates if the log file already exists.
     */
    void _write_intro(bool is_exists);

    /**
     * @brief Writes raw data to the log file.
     * @param data The log message to write.
     */
    void _write(const std::string& data);

    /**
     * @brief Writes a raw character stream to the log file.
     * @param data The character buffer.
     * @param size The size of the buffer.
     */
    void _write_stream(const char* data, size_t size);

private:
    size_t _write_byte = 0;            ///< Total bytes written
    size_t _need_flush = 0;            ///< Buffer threshold for flushing
    std::shared_ptr<std::ofstream> _out; ///< Log file output stream
};


#endif //!_svc_logger_h