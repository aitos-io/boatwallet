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

/*!@brief libcurl porting header file

@file
curlport.h is the header file of libcurl porting of RPC.

DO NOT call functions in this file directly. Instead call wrapper functions
provided by rpcport.

To use libcurl porting, RPC_USE_LIBCURL in boatoptions.h must set to 1.
*/

#ifndef __CURLPORT_H__
#define __CURLPORT_H__

#if RPC_USE_LIBCURL == 1

#include "wallet/boattypes.h"
#include "wallet/boatoptions.h"

#ifdef __cplusplus
extern "C" {
#endif

BOAT_RESULT CurlPortInit(void);

void CurlPortDeinit(void);

BOAT_RESULT CurlPortSetOpt(const RpcOption *rpc_option_ptr);

BOAT_RESULT CurlPortRequestSync(const CHAR *request_str,
                               UINT32 request_len,
                               BOAT_OUT CHAR **response_str_ptr,
                               BOAT_OUT UINT32 *response_len_ptr);


#ifdef __cplusplus
}
#endif /* end of __cplusplus */

#endif // end of #if RPC_USE_LIBCURL == 1

#endif

