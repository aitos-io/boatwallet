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

/*!@brief RPC wrapper

@file
rpcintf.c contains wrapper functions for RPC.
The exact implementation of the actual RPC mechanism is controlled by
RPC_USE_XXX macros.
*/

#include "wallet/boattypes.h"
#include "rpc/rpcport.h"

//!@brief  Context for RPC
RpcCtx g_rpc_ctx;

//!@brief  Options struct for RPC
RpcOption g_rpc_option;


/*!*****************************************************************************
@brief Wrapper function to initialize RPC mechanism.

Function: RpcInit()

    This function wrapper to initialize RPC mechanism.
    The exact implementation of the actual RPC mechanism is controlled by
    RPC_USE_XXX macros.
    

@return
    This function returns BOAT_SUCCESS if successful. Otherwise it returns BOAT_ERROR.
    

@param This function doesn't take any argument.

*******************************************************************************/
BOAT_RESULT RpcInit(void)
{
    BOAT_RESULT result;

#if RPC_USE_LIBCURL == 1
    result = CurlPortInit();
#endif

    return result;

}


/*!*****************************************************************************
@brief Wrapper function to de-initialize RPC mechanism.

Function: RpcDeinit()

    This function de-initializes RPC mechanism.
    The exact implementation of the actual RPC mechanism is controlled by
    RPC_USE_XXX macros.
    

@return
    This function doesn't return any value.
    

@param This function doesn't take any argument.

*******************************************************************************/
void RpcDeinit(void)
{
#if RPC_USE_LIBCURL == 1
    CurlPortDeinit();
#endif

    return;
}


/*!*****************************************************************************
@brief Wrapper function to set options for RPC mechanism.

Function: RpcSetOpt()

    This function set options for use with RPC mechanism.
    The exact implementation of the actual RPC mechanism is controlled by
    RPC_USE_XXX macros.
    

@return
    This function returns BOAT_SUCCESS if successful. Otherwise it returns BOAT_ERROR.
    

@param[in] rpc_option_ptr
    A pointer to the option struct of RpcOption. The exact struct of RPC implementation
    is controlled by RPC_USE_XXX macros.

*******************************************************************************/
BOAT_RESULT RpcSetOpt(const RpcOption *rpc_option_ptr)
{
    BOAT_RESULT result;
    
    memcpy(&g_rpc_option, rpc_option_ptr, sizeof(RpcOption));

#if RPC_USE_LIBCURL == 1
    result = CurlPortSetOpt(&g_rpc_option);
#endif

    return result;
}


/*!******************************************************************************
@brief Wrapper function to perform RPC request and receive its response synchronously.

Function: RpcRequestSync()

    This function is a wrapper for performing RPC calls synchronously.
    
    This function takes the REQUEST to transmit as input argument and outputs
    the address of the buffer containing received RPC RESPONSE. The function
    will be suspended until it receives RESPONSE or timeouts.

    The exact format and meaning of the request and response is defined by the
    wrapped function.

    The caller could only read from the response buffer and copy to its own
    buffer. The caller MUST NOT modify, free the response buffer or save the
    address of the response buffer for later use.


@return
    This function returns BOAT_SUCCESS if the RPC call is successful.\n
    If any error occurs or RPC REQUEST timeouts, it transfers the error code
    returned by the wrapped function.
    

@param[in] request_ptr
        A pointer to the buffer containing RPC REQUEST.

@param[in] request_len
        The length of the RPC REQUEST in bytes.

@param[out] response_pptr
        The address of a (UINT8 *) pointer to hold the address of the RESPONSE
        buffer. Note that whether the content in the buffer is binary stream or
        string is defined by the wrapped function. DO NOT assume it's NULL
        terminated even if it were string.\n
        If any error occurs or RPC REQUEST timeouts, the address is NULL.

@param[out] response_len_ptr
        The address of a UINT32 to hold the length of the received RESPONSE.
        Note that this doesn't imply a NULL terminator being counted even if
        the response is a string.\n
        If any error occurs or RPC REQUEST timeouts, the length is 0.
        
*******************************************************************************/
BOAT_RESULT RpcRequestSync(const UINT8 *request_ptr,
                          UINT32 request_len,
                          BOAT_OUT UINT8 **response_pptr,
                          BOAT_OUT UINT32 *response_len_ptr)
{
    BOAT_RESULT result;
    
#if RPC_USE_LIBCURL == 1
    result = CurlPortRequestSync((const CHAR *)request_ptr, request_len, (BOAT_OUT CHAR **)response_pptr, response_len_ptr);
#endif

    return result;
}




