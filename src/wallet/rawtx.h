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

/*!@brief Header file for performing RAW transaction

@file
rawtx.h is header file for RAW transaction construction and performing.
*/

#ifndef __RAWTX_H__
#define __RAWTX_H__

#include "wallet/boattypes.h"

/*!
Enum Type RlpFieldType
*/
typedef enum
{
    RLP_FIELD_TYPE_STRING = 0,
    RLP_FIELD_TYPE_LIST
}RlpFieldType;

#ifdef __cplusplus
extern "C" {
#endif

BOAT_RESULT RawtxPerform(BoatWalletInfo *boat_wallet_info_ptr, BOAT_INOUT TxInfo *tx_info_ctx_ptr);

#ifdef __cplusplus
}
#endif /* end of __cplusplus */

#endif
