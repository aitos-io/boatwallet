/******************************************************************************
Copyright (C) 2018-2019 AITOS.IO

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
¡¡¡¡
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

/*!@brief libcurl porting for RPC

@file
curlport.c is the libcurl porting of RPC.

DO NOT call functions in this file directly. Instead call wrapper functions
provided by rpcport.

To use libcurl porting, RPC_USE_LIBCURL in boatoptions.h must set to 1.
*/

#include "wallet/boattypes.h"

#if RPC_USE_LIBCURL == 1
#include "utilities/utility.h"
#include "rpc/rpcport.h"
#include "rpc/curlport.h"
#include "curl/curl.h"

//!@brief Defines the buffer size to receive response from peer.


//!The step to dynamically expand the receiving buffer.
#define CURLPORT_RECV_BUF_SIZE_STEP 1024

//!@brief A struct to maintain a dynamic length string.
typedef struct TCurlPortStringWithLen
{
    CHAR *string_ptr;   //!< address of the string storage
    UINT32 string_len;  //!< string length in byte excluding NULL terminator, equal to strlen(string_ptr)
    UINT32 string_space;//!< size of the space <string_ptr> pointing to, including null terminator
}CurlPortStringWithLen;

CurlPortStringWithLen g_curlport_response = {NULL, 0, 0}; 



/*!*****************************************************************************
@brief Initialize libcurl.

Function: CurlPortInit()

    This function initializes libcurl. It also dynamically allocates storage to
    receive response from the peer.
    

@return
    This function returns BOAT_SUCCESS if successful. Otherwise it returns BOAT_ERROR.
    

@param This function doesn't take any argument.

*******************************************************************************/
BOAT_RESULT CurlPortInit(void)
{
    CURLcode curl_result;
    BOAT_RESULT result;
    
    curl_result = curl_global_init(CURL_GLOBAL_DEFAULT);
    
    if( curl_result != CURLE_OK)
    {
        BoatLog(BOAT_LOG_CRITICAL, "Unable to initialize curl.");
        result = BOAT_ERROR_EXT_MODULE_OPERATION_FAIL;
    }
    else
    {
        g_curlport_response.string_space = CURLPORT_RECV_BUF_SIZE_STEP;
        g_curlport_response.string_len = 0;


        g_curlport_response.string_ptr = BoatMalloc(CURLPORT_RECV_BUF_SIZE_STEP);
        
        if( g_curlport_response.string_ptr == NULL )
        {
            BoatLog(BOAT_LOG_CRITICAL, "Fail to allocate Curl RESPONSE buffer.");
            curl_global_cleanup();
            result = BOAT_ERROR_NULL_POINTER;
        }
        else
        {
            result = BOAT_SUCCESS;
        }

    }
    
    return result;

}


/*!*****************************************************************************
@brief Deinitialize libcurl.

Function: CurlPortDeinit()

    This function de-initializes libcurl. It also frees the dynamically
    allocated storage to receive response from the peer.
    

@return
    This function doesn't return any value.
    

@param This function doesn't take any argument.

*******************************************************************************/
void CurlPortDeinit(void)
{
    curl_global_cleanup();

    g_curlport_response.string_space = 0;
    g_curlport_response.string_len = 0;


    if( g_curlport_response.string_ptr != NULL )
    {
        BoatFree(g_curlport_response.string_ptr);
    }


    g_curlport_response.string_ptr = NULL;

    return;
}


/*!*****************************************************************************
@brief Set options for use with libcurl.

Function: CurlPortSetOpt()

    This function is a dummy function for compatible with RPC Port skeleton.
    curl_easy_setopt() is actually called in CurlPortRequestSync() because some
    options are per-session effective.
    

@return
    This function always returns BOAT_SUCCESS.
    

@param[in] rpc_option_ptr
    A pointer to the option struct of RpcOption.

*******************************************************************************/
BOAT_RESULT CurlPortSetOpt(const RpcOption *rpc_option_ptr)
{
    return BOAT_SUCCESS;
}


/*!*****************************************************************************
@brief Callback function to write received data from the peer to the user specified buffer.

Function: CurlPortWriteMemoryCallback()

    This function is a callback function as per libcurl CURLOPT_WRITEFUNCTION option.
    libcurl will call this function (possibly) multiple times to write received
    data from peer to the buffer specified by this function. The received data
    are typically some RESPONSE from the HTTP server.

    The receiving buffer is dynamically allocated. If the received data from
    the peer exceeds the current buffer size, a new buffer that could hold all
    data will be allocated and previously received data will be copied to the
    new buffer as well as the newly received data.

    
@see https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
    

@return
    This function returns how many bytes are written into the user buffer.
    If the returned size differs from <size>*<nmemb>, libcurl will treat it as
    a failure.
    

@param[in] data_ptr
    A pointer given by libcurl, pointing to the received data from peer.

@param[in] size
    For historic reasons, libcurl will always call with <size> = 1.

@param[in] nmemb
    <nmemb> is the size of the data chunk to write. It doesn't include null
    terminator even if the data were string.\n
    For backward compatibility, use <size> * <nmemb> to calculate the size of
    the received data.

@param[in] userdata
    <userdata> is the value previously set by CURLOPT_WRITEDATA option.
    Typically it's a pointer to a struct which contains information about the
    receiving buffer.

*******************************************************************************/
size_t CurlPortWriteMemoryCallback(void *data_ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t data_size;
    CurlPortStringWithLen *mem;
    UINT32 expand_size;
    UINT32 expand_steps;
    CHAR *expanded_str;
    UINT32 expanded_to_space;
    
    mem = (CurlPortStringWithLen*)userdata;

    // NOTE: For historic reasons, argument size is always 1 and nmemb is the
    // size of the data chunk to write. And size * nmemb doesn't include null
    // terminator even if the data were string.
    data_size = size * nmemb;
    
    // If response buffer has enough space:
    if( mem->string_space - mem->string_len > data_size ) // 1 more byte reserved for null terminator
    {
        memcpy(mem->string_ptr + mem->string_len, data_ptr, data_size);
        mem->string_len += data_size;
        mem->string_ptr[mem->string_len] = '\0';
    }
    else  // If response buffer has no enough space
    {

        // If malloc is supported, expand the response buffer in steps of
        // CURLPORT_RECV_BUF_SIZE_STEP.
        
        expand_size = data_size - (mem->string_space - mem->string_len) + 1; // plus 1 for null terminator
        expand_steps = (expand_size - 1) / CURLPORT_RECV_BUF_SIZE_STEP + 1;
        expanded_to_space = expand_steps * CURLPORT_RECV_BUF_SIZE_STEP + mem->string_space;
    
        expanded_str = BoatMalloc(expanded_to_space);

        if( expanded_str != NULL )
        {
            memcpy(expanded_str, mem->string_ptr, mem->string_len);
            memcpy(expanded_str + mem->string_len, data_ptr, data_size);
            BoatFree(mem->string_ptr);
            mem->string_ptr = expanded_str;
            mem->string_space = expanded_to_space;
            mem->string_len += data_size;
            mem->string_ptr[mem->string_len] = '\0';
        }

    }

    return data_size;

}


/*!*****************************************************************************
@brief Perform a synchronous HTTP POST and wait for its response.

Function: CurlPortRequestSync()

    This function initiates a curl session, performs a synchronous HTTP POST
    and waits for its response.

    is a callback function as per libcurl CURLOPT_WRITEFUNCTION option.
    libcurl will call this function (possibly) multiple times to write received
    data from peer to the buffer specified by this function. The received data
    are typically some RESPONSE from the HTTP server.

@see https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
    

@return
    This function returns how many bytes are written into the user buffer.
    If the returned size differs from <size>*<nmemb>, libcurl will treat it as
    a failure.
    

@param[in] request_str
    A pointer to the request string to POST.

@param[in] request_len
    The length of <request_str> excluding NULL terminator. This function is
    wrapped by RpcRequestSync() and thus takes this argument for compatibility
    with the wrapper function. Typically it equals to strlen(request_str).

@param[out] response_str_ptr
    The address of a CHAR* pointer (i.e. a double pointer) to hold the address
    of the receiving buffer.\n
    The receiving buffer is internally maintained by curlport and the caller
    shall only read from the buffer. DO NOT modify the buffer or save the address
    for later use.

@param[out] response_len_ptr
    The address of a UINT32 integer to hold the effective length of
    <response_str_ptr> excluding NULL terminator. This function is wrapped by
    RpcRequestSync() and thus takes this argument for compatibility with the
    wrapper function. Typically it equals to strlen(response_str_ptr).

*******************************************************************************/
BOAT_RESULT CurlPortRequestSync(const CHAR *request_str,
                               UINT32 request_len,
                               BOAT_OUT CHAR **response_str_ptr,
                               BOAT_OUT UINT32 *response_len_ptr)
{
    CURL *curl_ctx_ptr = NULL;
    struct curl_slist *curl_opt_list_ptr = NULL;
    CURLcode curl_result;
    
    long info;
    BOAT_RESULT result = BOAT_ERROR;
    boat_try_declare;


    if( g_rpc_option.node_url_str == NULL
       || request_str == NULL
       || response_str_ptr == NULL
       || response_len_ptr == NULL )
    {
        BoatLog(BOAT_LOG_CRITICAL, "Argument cannot be NULL.");
        result = BOAT_ERROR;
        boat_throw(BOAT_ERROR_NULL_POINTER, CurlPortRequestSync_cleanup);
    }

    curl_ctx_ptr = curl_easy_init();
    
    if(curl_ctx_ptr != NULL)
    {
        g_rpc_ctx.curl_ctx_ptr = curl_ctx_ptr;
    }
    else
    {
        BoatLog(BOAT_LOG_CRITICAL, "curl_easy_init() fails.");
        result = BOAT_ERROR_EXT_MODULE_OPERATION_FAIL;
        boat_throw(BOAT_ERROR_EXT_MODULE_OPERATION_FAIL, CurlPortRequestSync_cleanup);
    }
    
    // Set RPC URL in format "<protocol>://<target name or IP>:<port>". e.g. "http://192.168.56.1:7545"
    curl_result = curl_easy_setopt(curl_ctx_ptr, CURLOPT_URL, g_rpc_option.node_url_str);
    if( curl_result != CURLE_OK )
    {
        BoatLog(BOAT_LOG_NORMAL, "Unknown URL: %s", g_rpc_option.node_url_str);
        boat_throw(BOAT_ERROR_EXT_MODULE_OPERATION_FAIL, CurlPortRequestSync_cleanup);
    }

    // Configure all protocols to be supported
    curl_easy_setopt(curl_ctx_ptr, CURLOPT_PROTOCOLS, CURLPROTO_ALL);
                   
    // Configure SSL Certification Verification
    // If certification file is not available, set them to 0.
    // See: https://curl.haxx.se/libcurl/c/CURLOPT_SSL_VERIFYPEER.html
    curl_easy_setopt(curl_ctx_ptr, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl_ctx_ptr, CURLOPT_SSL_VERIFYHOST, 0);

    // To specify a certificate file or specify a path containing certification files
    // Only make sense when CURLOPT_SSL_VERIFYPEER is set to non-zero.
    // curl_easy_setopt(curl_ctx_ptr, CURLOPT_CAINFO, "/etc/certs/cabundle.pem");
    // curl_easy_setopt(curl_ctx_ptr, CURLOPT_CAPATH, "/etc/cert-dir");

    // Allow Re-direction
    curl_easy_setopt(curl_ctx_ptr, CURLOPT_FOLLOWLOCATION,1); 

    // Verbose Debug Info.
    // curl_easy_setopt(curl_ctx_ptr, CURLOPT_VERBOSE, 1);


    // Set HTTP Type: POST
    curl_easy_setopt(curl_ctx_ptr, CURLOPT_POST, 1L);

    // Set redirection: No
    curl_easy_setopt(curl_ctx_ptr, CURLOPT_FOLLOWLOCATION, 0);

    // Set entire curl timeout in millisecond. This time includes DNS resloving.
    curl_easy_setopt(curl_ctx_ptr, CURLOPT_TIMEOUT_MS, 30000L);

    // Set Connection timeout in millisecond
    curl_easy_setopt(curl_ctx_ptr, CURLOPT_CONNECTTIMEOUT_MS, 10000L);

    // Set HTTP HEADER Options
    curl_opt_list_ptr = curl_slist_append(curl_opt_list_ptr,"Content-Type:application/json;charset=UTF-8");
    if( curl_opt_list_ptr == NULL ) boat_throw(BOAT_ERROR_EXT_MODULE_OPERATION_FAIL, CurlPortRequestSync_cleanup);
    
    curl_opt_list_ptr = curl_slist_append(curl_opt_list_ptr,"Accept:application/json, text/javascript, */*;q=0.01");
    if( curl_opt_list_ptr == NULL ) boat_throw(BOAT_ERROR_EXT_MODULE_OPERATION_FAIL, CurlPortRequestSync_cleanup);

    curl_opt_list_ptr = curl_slist_append(curl_opt_list_ptr,"Accept-Language:zh-CN,zh;q=0.8");
    if( curl_opt_list_ptr == NULL ) boat_throw(BOAT_ERROR_EXT_MODULE_OPERATION_FAIL, CurlPortRequestSync_cleanup);

    curl_easy_setopt(curl_ctx_ptr, CURLOPT_HTTPHEADER, curl_opt_list_ptr);


    // Set callback and receive buffer for RESPONSE
    // Clean up response buffer
    g_curlport_response.string_ptr[0] = '\0';
    g_curlport_response.string_len = 0;
    curl_easy_setopt(curl_ctx_ptr, CURLOPT_WRITEDATA, &g_curlport_response);
    curl_easy_setopt(curl_ctx_ptr, CURLOPT_WRITEFUNCTION, CurlPortWriteMemoryCallback);

    // Set content to POST    
    curl_easy_setopt(curl_ctx_ptr, CURLOPT_POSTFIELDS, request_str);
    curl_easy_setopt(curl_ctx_ptr, CURLOPT_POSTFIELDSIZE, request_len);


    // Perform the RPC request
    curl_result = curl_easy_perform(curl_ctx_ptr);

    if( curl_result != CURLE_OK )
    {
        BoatLog(BOAT_LOG_NORMAL, "curl_easy_perform fails with CURLcode: %d.", curl_result);
        boat_throw(BOAT_ERROR_EXT_MODULE_OPERATION_FAIL, CurlPortRequestSync_cleanup);
    }
    

    curl_result = curl_easy_getinfo(curl_ctx_ptr, CURLINFO_RESPONSE_CODE, &info);

    if(( curl_result == CURLE_OK ) && (info == 200 || info == 201))
    {
        *response_str_ptr = g_curlport_response.string_ptr;
        *response_len_ptr = g_curlport_response.string_len;
        
        BoatLog(BOAT_LOG_VERBOSE, "Post: %s", request_str);
        BoatLog(BOAT_LOG_VERBOSE, "Result Code: %ld", info);
        BoatLog(BOAT_LOG_VERBOSE, "Response: %s", *response_str_ptr);
    }
    else
    {
        BoatLog(BOAT_LOG_NORMAL, "curl_easy_getinfo fails with CURLcode: %d, HTTP response code %ld.", curl_result, info);
        boat_throw(BOAT_ERROR_EXT_MODULE_OPERATION_FAIL, CurlPortRequestSync_cleanup);
    }    

    // Clean Up
    curl_slist_free_all(curl_opt_list_ptr);
    curl_easy_cleanup(curl_ctx_ptr);
    g_rpc_ctx.curl_ctx_ptr = NULL;
    
    result = BOAT_SUCCESS;


    // Exceptional Clean Up
    boat_catch(CurlPortRequestSync_cleanup)
    {
        BoatLog(BOAT_LOG_NORMAL, "Exception: %d", boat_exception);
        
        if( curl_opt_list_ptr != NULL )
        {
            curl_slist_free_all(curl_opt_list_ptr);
        }

        if( curl_ctx_ptr != NULL )
        {
            curl_easy_cleanup(curl_ctx_ptr);
            g_rpc_ctx.curl_ctx_ptr = NULL;
        }
        result = boat_exception;
    }
    
    return result;
    
}

#endif // end of #if RPC_USE_LIBCURL == 1
