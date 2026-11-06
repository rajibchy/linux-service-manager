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

// 9:38 PM 2/11/2025
// by Rajib Chy
#ifdef _MSC_VER
#pragma once
#endif//_MSC_VER

#ifndef _fsys_http_h
#define _fsys_http_h

#ifdef USE_HTTP_DAY_STATUS

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netdb.h>
    #include <unistd.h>
#endif //!_WIN32

#include <cstring>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

/**
 * @brief Establishes an HTTP connection to the specified host and port.
 *
 * @param host The hostname or IP address of the server.
 * @param port The port number as a string (e.g., "80" for HTTP).
 * @return A socket file descriptor on success, or -1 on failure.
 */
int http_create_connection(const char *host, const char *port);

/**
 * @brief Sends an HTTP GET request to the specified host and path.
 *
 * @param sock The socket file descriptor obtained from http_create_connection().
 * @param host The hostname to include in the HTTP request header.
 * @param path The resource path to request (e.g., "/index.html").
 * @return 0 on success, or -1 on failure.
 */
int http_send_request(int sock, const char *host, const char *path);

/**
 * @brief Reads the HTTP response from the server.
 *
 * @param sock The socket file descriptor used for the connection.
 * @param response A pointer to a dynamically allocated string to store the response.
 *                 The caller must free this memory after use.
 * @return The number of bytes read on success, or -1 on failure.
 */
int http_read_response(int sock, char **response);

/**
 * @brief Extracts the body from an HTTP response.
 *
 * @param response The full HTTP response, including headers.
 * @return A pointer to the start of the response body within the given string.
 *         The original response string must not be modified or freed before using this pointer.
 */
char *http_extract_body(char *response);

/**
 * @brief Cleans up resources associated with the HTTP connection.
 *
 * @param sock The socket file descriptor to close.
 * @param response The allocated response buffer to free. Can be NULL.
 */
void http_cleanup(int sock, char *response);

#endif //!USE_HTTP_DAY_STATUS

#endif //!_fsys_http_h