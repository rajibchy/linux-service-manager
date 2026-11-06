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

// 9:56 PM 2/11/2025
// by Rajib Chy

#ifdef USE_HTTP_DAY_STATUS

#include <svc/httpc.h>
#include <svc/http.h>

const char* http_client::get_host() const {
    return _host.c_str();
}

int http_client::get( const std::string&path, std::string& result ) {

    int sock = http_create_connection(_host.c_str(), _port.c_str());
    
    if (sock < 0) {
        set_last_error("Failed to connect to ", _host.c_str());
        return 0;
    }

    if (http_send_request(sock, _host.c_str(), path.c_str()) < 0) {
        set_last_error("Failed to send request to ", _host.c_str());
        close(sock);
        return 0;
    }

    char *response = NULL;
    if (http_read_response(sock, &response) < 0) {
        set_last_error("Failed to read response from ", _host.c_str());
        close(sock);
        return 0;
    }

    char *body = http_extract_body(response);
    std::string(body).swap(result);

    http_cleanup(sock, response);
    return 1;
}

const char* http_client::get_last_error() const{
    if(_last_error.empty()){
        return "";
    }
    return _last_error.c_str();
}

void http_client::set_last_error( const char* prefix, const char* error_detail ) {
    std::string(prefix).append(" ").append(error_detail).swap(_last_error);
}

#endif //!USE_HTTP_DAY_STATUS