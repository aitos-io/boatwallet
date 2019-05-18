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

/*!@brief Boatwallet SDK header file

@file
boatwallet.h is the SDK header file.
*/

#ifndef __BOATWALLET_H__
#define __BOATWALLET_H__

#include "wallet/boattypes.h"
#include "web3/web3intf.h"
#include "utilities/utility.h"
#include "wallet/rawtx.h"
#include "rpc/rpcintf.h"



extern BoatWalletInfo g_boat_wallet_info;
extern TxInfo g_tx_info;

#ifdef __cplusplus
extern "C" {
#endif


BOAT_RESULT BoatWalletInit(void);

void BoatWalletDeInit(void);

BOAT_RESULT BoatWalletSetNodeUrl(const CHAR *node_url_ptr);

BOAT_RESULT BoatWalletSetEIP155Comp(UINT8 eip155_compatibility);

BOAT_RESULT BoatWalletSetChainId(UINT32 chain_id);

BOAT_RESULT BoatWalletSetPrivkey(const UINT8 priv_key_array[32]);

BOAT_RESULT BoatWalletGeneratePrivkey(BOAT_OUT UINT8 priv_key_array[32]);

BOAT_RESULT BoatWalletCheckPrivkey(const UINT8 priv_key_array[32]);

CHAR * BoatWalletGetBalance(void);

BOAT_RESULT BoatWalletSaveWalletEx(const BoatWalletInfo *wallet_info_ptr, const UINT8 *passwd_ptr, UINT32 passwd_len, const CHAR *file_path_str);
BOAT_RESULT BoatWalletSaveWallet(const UINT8 *passwd_ptr, UINT32 passwd_len, const CHAR *file_path_str);

BOAT_RESULT BoatWalletLoadWalletEx(BoatWalletInfo *wallet_info_ptr, const UINT8 *passwd_ptr, UINT32 passwd_len, const CHAR *file_path_str);
BOAT_RESULT BoatWalletLoadWallet(const UINT8 *passwd_ptr, UINT32 passwd_len, const CHAR *file_path_str);

BOAT_RESULT BoatTxSetNonce(void);

BOAT_RESULT BoatTxSetGasPrice(TxFieldMax32B *gas_price_ptr);

BOAT_RESULT BoatTxSetGasLimit(TxFieldMax32B *gas_limit_ptr);

BOAT_RESULT BoatTxSetRecipient(BoatAddress address);

BOAT_RESULT BoatTxSetValue(TxFieldMax32B *value_ptr);

BOAT_RESULT BoatTxSetData(TxFieldVariable *data_ptr);

BOAT_RESULT BoatTxSend(void);

CHAR * BoatCallContractFunc(
                    CHAR * contract_addr_str,
                    CHAR *func_proto_str,
                    UINT8 *func_param_ptr,
                    UINT32 func_param_len);

#ifdef __cplusplus
}
#endif /* end of __cplusplus */

#endif
