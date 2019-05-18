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

/*!@brief Web3 interface header file

@file
web3intf.h is the header file for web3 interface.
*/

#ifndef __WEB3INTF_H__
#define __WEB3INTF_H__


#ifdef __cplusplus
extern "C" {
#endif


BOAT_RESULT web3_init(void);

//!@brief Parameter for web3_eth_getTransactionCount()
typedef struct TParam_eth_getTransactionCount
{
    CHAR *address_str; //!< String of 20-byte Ethereum address, e.g. "0x123456..."
    CHAR *block_num_str;  //!< String of either block number or one of "latest", "earliest" and "pending"
}Param_eth_getTransactionCount;

CHAR *web3_eth_getTransactionCount(
                                    const char *node_url_str,
                                    const Param_eth_getTransactionCount *param_ptr);


//!@brief Parameter for web3_eth_getBalance()
typedef struct TParam_eth_getBalance
{
    CHAR *address_str; //!< String of 20-byte Ethereum address, e.g. "0x123456..."
    CHAR *block_num_str;  //!< String of either block number or one of "latest", "earliest" and "pending"
}Param_eth_getBalance;

CHAR *web3_eth_getBalance(
                                    const char *node_url_str,
                                    const Param_eth_getBalance *param_ptr);


//!@brief Parameter for web3_eth_sendRawTransaction()
typedef struct TParam_eth_sendRawTransaction
{
    CHAR *signedtx_str;  //!< String of the signed transaction in HEX with "0x" prefixed
}Param_eth_sendRawTransaction;

CHAR *web3_eth_sendRawTransaction(
                                    const char *node_url_str,
                                    const Param_eth_sendRawTransaction *param_ptr);

CHAR *web3_eth_gasPrice(const char *node_url_str);


//!@brief Parameter for web3_eth_getStorageAt()
typedef struct TParam_eth_getStorageAt
{
    CHAR *address_str;    //!< String of 20-byte Ethereum address, e.g. "0x123456..."
    CHAR *position_str;   //!< String of storage position
    CHAR *block_num_str;  //!< String of either block number or one of "latest", "earliest" and "pending"

}Param_eth_getStorageAt;

CHAR *web3_eth_getStorageAt(
                                    const char *node_url_str,
                                    const Param_eth_getStorageAt *param_ptr);


//!@brief Parameter for web3_eth_getTransactionReceiptStatus()
typedef struct TParam_eth_getTransactionReceipt
{
    CHAR *tx_hash_str; //!< String of 32-byte transaction hash, e.g. "0x123456..."
}Param_eth_getTransactionReceipt;

CHAR *web3_eth_getTransactionReceiptStatus(
                                    const char *node_url_str,
                                    const Param_eth_getTransactionReceipt *param_ptr);

//!@brief Parameter for web3_eth_call()
typedef struct TParam_eth_call
{
    CHAR *to;       //!< The address of the contract.
    CHAR *gas;      //!< The gasLimit.
    CHAR *gasPrice; //!< The gasPrice in wei.
    CHAR *data;     //!< The function selector followed by parameters.
}Param_eth_call;

CHAR *web3_eth_call(
                                    const char *node_url_str,
                                    const Param_eth_call *param_ptr);

#ifdef __cplusplus
}
#endif /* end of __cplusplus */

#endif
