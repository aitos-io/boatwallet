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

/*!@brief RPC wrapper header file

@file
rpcintf.h is header file for RPC wrapper functions.
The exact implementation of the actual RPC mechanism is controlled by
RPC_USE_XXX macros.
*/
#ifndef __RPCINTF_H__
#define __RPCINTF_H__

#include "wallet/boattypes.h"
#include "wallet/boatoptions.h"

#if RPC_USE_LIBCURL == 1
#include "curl/curl.h"
#endif

//!@brief Context for RPC
typedef struct TRpcCtx
{
#if RPC_USE_LIBCURL == 1
    CURL *curl_ctx_ptr;     //!< CURL pointer returned by curl_easy_init()
#endif
}RpcCtx;

//!@brief Options struct for RPC
typedef struct TRpcOption
{
#if RPC_USE_LIBCURL == 1
    const CHAR *node_url_str;   //!< The URL of blockchain node, in a form of "http://a.b.com:7545"
#endif
}RpcOption;



#ifdef __cplusplus
extern "C" {
#endif

BOAT_RESULT RpcInit(void);

void RpcDeinit(void);

BOAT_RESULT RpcSetOpt(const RpcOption *rpc_option_ptr);

BOAT_RESULT RpcRequestSync(const UINT8 *request_ptr,
                          UINT32 request_len,
                          BOAT_OUT UINT8 **response_pptr,
                          BOAT_OUT UINT32 *response_len_ptr);

#ifdef __cplusplus
}
#endif /* end of __cplusplus */

#endif
