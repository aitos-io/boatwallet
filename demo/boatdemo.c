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

#include "wallet/boatwallet.h"

// Case declaration
BOAT_RESULT CaseSendEtherMain(void);
BOAT_RESULT CaseGpsTraceMain(void);


BOAT_RESULT SetCommonParam(CHAR *node_url_ptr)
{
    UINT8 priv_key_array[32];
    TxFieldMax32B gas_price;
    TxFieldMax32B gas_limit;
    BOAT_RESULT result;

    // Step 1: Set Wallet Parameters
    
    // Set EIP-155 Compatibility
    result = BoatWalletSetEIP155Comp(BOAT_FALSE);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;


    // Set Node URL
    result = BoatWalletSetNodeUrl(node_url_ptr);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;


    // Set Chain ID (If EIP-155 compatibility is FALSE, Chain ID is ignored)
    result = BoatWalletSetChainId(5777);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;


    // Set Private Key
    // PRIVATE KEY MUST BE SET BEFORE SETTING TRANSACTION PARAMETERS

    UtilityHex2Bin(
                    priv_key_array,
                    32,
                    "0xe55464c12b9e034ab00f7dddeb01874edcf514b3cd77a9ad0ad8796b4d3b1fdb",
                    TRIMBIN_TRIM_NO,
                    BOAT_FALSE
                  );

    result = BoatWalletSetPrivkey(priv_key_array);

    // Destroy private key in local variable
    memset(priv_key_array, 0x00, 32);

    if( result != BOAT_SUCCESS ) return BOAT_ERROR;


    // Step 2: Set Transaction Common Parameters

    // Set gasprice
    // Either manually set the gas price or get the price from network
    #if 0
    // Manually
    gas_price.field_len =
    UtilityHex2Bin(
                    gas_price.field,
                    32,
                    "0x8250de00",
                    TRIMBIN_LEFTTRIM,
                    BOAT_TRUE
                  );
    #else
    // To use the price obtained from network, simply call BoatTxSetGasPrice(NULL)
    result = BoatTxSetGasPrice(NULL/*&gas_price*/);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;
    #endif

    // Set gaslimit

    gas_limit.field_len =
    UtilityHex2Bin(
                    gas_limit.field,
                    32,
                    "0x1fffff",
                    TRIMBIN_LEFTTRIM,
                    BOAT_TRUE
                  );

    result = BoatTxSetGasLimit(&gas_limit);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;
    
    return BOAT_SUCCESS;
}



int main(int argc, char *argv[])
{
    BOAT_RESULT result;
    CHAR *keystore_passwd = "boaTaEsp@ssW0rd";
    

    // Usage Example: boatdemo http://127.0.0.1:7545
    
    if( argc != 2 )
    {
        BoatLog(BOAT_LOG_CRITICAL, "Usage: %s http://<IP Address or URL for node>:<port>\n", argv[0]);
        return BOAT_ERROR;
    }

    BoatWalletInit();
    

    
    // Set common parameters such as Node URL, Chain ID, Private key, gasprice, gaslimit
    result = SetCommonParam(argv[1]);
    if( result != BOAT_SUCCESS ) goto main_destruct;
    
    // Save/Load account
    result = BoatWalletSaveWallet((UINT8 *)keystore_passwd, strlen(keystore_passwd), "keystore.sav");
    if( result != BOAT_SUCCESS ) goto main_destruct;
    
    result = BoatWalletLoadWallet((UINT8 *)keystore_passwd, strlen(keystore_passwd), "keystore.sav");
    if( result != BOAT_SUCCESS ) goto main_destruct;



    // Case 1010: CaseSendEther
    BoatLog(BOAT_LOG_NORMAL, "====== Testing CaseSendEther ======");
    result = CaseSendEtherMain();
    if( result != BOAT_SUCCESS ) goto main_destruct;


    // Case 1020: CaseGpsTrace
    BoatLog(BOAT_LOG_NORMAL, "====== Testing CaseGpsTrace ======");
    result = CaseGpsTraceMain();
    if( result != BOAT_SUCCESS ) goto main_destruct;


main_destruct:


    BoatWalletDeInit();
    
    return 0;
}
