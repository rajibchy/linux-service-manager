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

// 9:43 PM 2/11/2025
// by Rajib Chy

#ifdef USE_HTTP_DAY_STATUS

#include <svc/http.h>

#define BUFFER_SIZE 4096
/**
 * Creates a socket and connects to the given host and port.
 */
int http_create_connection(const char *host, const char *port) {
    struct addrinfo hints;
    struct addrinfo *res;
    int sock;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return -1;
    }

    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("socket");
        freeaddrinfo(res);
        return -1;
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        close(sock);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sock;
}

/**
 * Sends an HTTP GET request to the server.
 */
int http_send_request(int sock, const char *host, const char *path) {
    // prevent "heap overflow vulnerability" and "Heap Exploitation"
    char request[1024];
    int written = snprintf( request, sizeof( request ),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "User-Agent: fsys-http-client/1.0\r\n"
             "X-Req-From: service\r\n"
             "\r\n",
             path, host );
			 
    if ( written < 0 || written >= sizeof(request) ) {
        fprintf( stderr, "Error: HTTP request buffer overflow.\n" );
        return -1;  // Prevent buffer overflow issues
    }
	
    if ( send( sock, request, strlen( request ), 0) < 0 ) {
        perror( "send" );
        return -1;
    }

    return 0;
}

/**
 * Reads the entire HTTP response from the server.
 */
int http_read_response( int sock, char **response ) {

    size_t capacity = BUFFER_SIZE;
    *response = (char *)malloc( capacity );
    if (!*response) {
        perror( "malloc" );
        return -1;
    }
    size_t total_size = 0;
    ssize_t bytes;
    // prevent "heap overflow vulnerability" and "Heap Exploitation"
	
    while ( ( bytes = recv( sock, *response + total_size, BUFFER_SIZE - 1, 0 ) ) > 0) {
        total_size += bytes;
        /*if (total_size + BUFFER_SIZE > capacity) {
            capacity *= 2;
            char *new_response = (char *)realloc(*response, capacity);
            if (!new_response) {
                perror("realloc");
                free(*response);
                return -1;
            }
            *response = new_response;
        }*/
		
        // Ensure space for the next BUFFER_SIZE chunk + null-terminator
        if (total_size + BUFFER_SIZE >= capacity) {  
            size_t new_capacity = capacity * 2;
            char *new_response = (char *)realloc( *response, new_capacity );
            if ( !new_response ) {
                perror( "realloc" );
                free( *response );  // Avoid UAF by setting *response = NULL after free
                *response = NULL;
                return -1;
            }

            *response = new_response;
            capacity = new_capacity;
        }
    }

    if (bytes < 0) {
        perror( "recv" );
        free( *response );
        return -1;
    }

    (*response)[total_size] = '\0';  // Null-terminate the response
    return 0;
}

/**
 * Extracts the HTTP response body by removing headers.
 */
char *http_extract_body(char *response) {
    char *body = strstr( response, "\r\n\r\n" );
    return body ? body + 4 : response;  // Skip past headers
}

/**
 * Closes the socket and frees allocated memory.
 */
void http_cleanup( int sock, char *response ) {
    close( sock );
    free( response );
}

#endif //!USE_HTTP_DAY_STATUS