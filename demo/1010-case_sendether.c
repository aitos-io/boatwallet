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

BOAT_RESULT CaseGetBalance(void)
{
    CHAR * balance_wei;

    balance_wei = BoatWalletGetBalance();
    if( balance_wei == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to get balance");
        return BOAT_ERROR;
    }

    BoatLog(BOAT_LOG_NORMAL, "Balance: %s wei", balance_wei);

    return BOAT_SUCCESS;
}


BOAT_RESULT EtherTransfer(void)
{
    BoatAddress recipient;
    TxFieldMax32B value;
    BOAT_RESULT result;
   

    // Set nonce
    result = BoatTxSetNonce();
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;
    

    // Set recipient

    UtilityHex2Bin(
                    recipient,
                    20,
                    "0x22d491bde2303f2f43325b2108d26f1eaba1e32b",
                    //"0x23966d599fe894d362a15c95f72eef2425c7fb0f",
                    //"0x19c91A4649654265823512a457D2c16981bB64F5",
                    //"0xe8b05f9d0ddf9e9ea83b4b7db832909108e9f8cf",
                    //"0x0c3e03942c186670c5187b15b4d0314b03a153b3",
                    TRIMBIN_TRIM_NO,
                    BOAT_TRUE
                  );

    result = BoatTxSetRecipient(recipient);

    if( result != BOAT_SUCCESS ) return BOAT_ERROR;
    

    // Set value

    value.field_len =
    UtilityHex2Bin(
                    value.field,
                    32,
                    //"0x2386F26FC10000", // 0.01ETH or 1e16 wei, value
                    //"0xDE0B6B3A7640000", // 1ETH or 1e18 wei, value
                    "0x29A2241AF62C0000",  // 3ETH or 3e18 wei, value
                    TRIMBIN_LEFTTRIM,
                    BOAT_TRUE
                  );

    result =BoatTxSetValue(&value);

    if( result != BOAT_SUCCESS ) return BOAT_ERROR;


    // Set data
    result =BoatTxSetData(NULL);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;

    
    // Perform the transaction
    // NOTE: Field v,r,s are calculated automatically
    result = BoatTxSend();
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;

    return BOAT_SUCCESS;
}


BOAT_RESULT CaseSendEtherMain(void)
{
    CaseGetBalance();
    EtherTransfer();
    CaseGetBalance();
    
    return BOAT_SUCCESS;
}

