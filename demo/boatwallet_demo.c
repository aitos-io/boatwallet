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
                    "0x6cbed15c793ce57650b9877cf6fa156fbef513c4e6134f022a85b1ffdd59b2a1",
                    //"8349614ba337e82ce4ce494feaa06fbf120af3308b87532e615133bf90cfd082",
                    //"0xe55464c12b9e034ab00f7dddeb01874edcf514b3cd77a9ad0ad8796b4d3b1fdb",
                    //"0x1ac150046992ffc9515aaa86bb3f3e6087043c4d1917218599a89ce39ca509da",
                    //"0x13e3ee5b517660853fd7525ed7a802d5864acae42fe73c33149f1364a5484f3b",
                    TRIMBIN_TRIM_NO,
                    BOAT_FALSE
                  );

    result = BoatWalletSetPrivkey(priv_key_array);

    // Destroy private key in local variable
    memset(priv_key_array, 0x00, 32);

    if( result != BOAT_SUCCESS ) return BOAT_ERROR;


    // Step 2: Set Transaction Common Parameters

    // Set gasprice
    /* 
    gas_price.field_len =
    UtilityHex2Bin(
                    gas_price.field,
                    32,
                    "0x8250de00",       // JuZix Juice-1.6.0 Default
                    TRIMBIN_LEFTTRIM,
                    BOAT_TRUE
                  );
    */
    // To use the price obtained from network, simply call BoatTxSetGasPrice(NULL)
    result = BoatTxSetGasPrice(NULL/*&gas_price*/);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;


    // Set gaslimit

    gas_limit.field_len =
    UtilityHex2Bin(
                    gas_limit.field,
                    32,
                    //"0xBEFE6F672000",   // PlatON Minimum
                    "0x1fffff",       // JuZix Juice-1.6.0 Default
                    TRIMBIN_LEFTTRIM,
                    BOAT_TRUE
                  );

    result = BoatTxSetGasLimit(&gas_limit);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;
    
    return BOAT_SUCCESS;
}


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

BOAT_RESULT CaseSendEther(void)
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


BOAT_RESULT CaseCallStoredataSol(CHAR * contract_addr_str)
{
    BoatAddress recipient;
    CHAR *function_prototye_str;
    UINT8 function_selector[32];
    TxFieldVariable data;
    UINT8 data_array[36];
    
    BOAT_RESULT result = BOAT_SUCCESS;
    
    if( contract_addr_str == NULL )
    {
        return BOAT_ERROR;
    }
    

    // Set nonce
    result = BoatTxSetNonce();
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;
    

    // Set recipient

    UtilityHex2Bin(
                    recipient,
                    20,
                    contract_addr_str,
                    TRIMBIN_TRIM_NO,
                    BOAT_TRUE
                  );

    result = BoatTxSetRecipient(recipient);

    if( result != BOAT_SUCCESS ) return BOAT_ERROR;


    // Set value

    result =BoatTxSetValue(NULL);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;
    

    // Set data (Function Argument)
    
    function_prototye_str = "storedata(uint256)";
    keccak_256((UINT8 *)function_prototye_str, strlen(function_prototye_str), function_selector);

    data.field_ptr = data_array;
    
    memcpy(data.field_ptr, function_selector, 4);

    data.field_len = 4 + UtilityHex2Bin(
                            data_array + 4,
                            sizeof(data_array)-4,
                            "0x0000000000000000000000000000000000000000000000000000000000000022",
                            TRIMBIN_TRIM_NO,
                            BOAT_FALSE);
    
     
    result = BoatTxSetData(&data);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;

    
    // Perform the transaction
    // NOTE: Field v,r,s are calculated automatically
    result = BoatTxSend();
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;

    return result;
}


BOAT_RESULT CaseCallGetdataSol(CHAR * contract_addr_str)
{

    BOAT_RESULT result = BOAT_SUCCESS;
    CHAR *retval_str;

    if( contract_addr_str == NULL )
    {
        return BOAT_ERROR;
    }


    retval_str = BoatCallContractFunc( contract_addr_str,
                                      "getdata()",
                                      NULL,
                                      0);

    if( retval_str != NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "retval of getdata() is %s.", retval_str);
        result = BOAT_SUCCESS;
    }
    else
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to call getdata().");
        result = BOAT_ERROR;
    }

    return result;
}


BOAT_RESULT CaseEthGetStorageAt(CHAR * contract_addr_str)
{
    CHAR *storage_content_str;
    Param_eth_getStorageAt param_eth_getStorageAt;

    if( contract_addr_str == NULL )
    {
        return BOAT_ERROR;
    }
    
    param_eth_getStorageAt.address_str = contract_addr_str;
    param_eth_getStorageAt.position_str = "0x0";
    param_eth_getStorageAt.block_num_str = "latest";
    
    storage_content_str = web3_eth_getStorageAt(g_boat_wallet_info.network_info.node_url_ptr,
                                                &param_eth_getStorageAt);

    BoatLog(BOAT_LOG_NORMAL, "Storage Content: %s\n", storage_content_str);

    return BOAT_SUCCESS;
}


BOAT_RESULT CaseCallSaveListSol(CHAR * contract_addr_str, UINT32 event)
{
    BoatAddress recipient;
    CHAR *function_prototye_str;
    UINT8 function_selector[32];
    TxFieldVariable data;
    UINT8 data_array[36];
    time_t current_time;
    
    BOAT_RESULT result;
    
    if( contract_addr_str == NULL )
    {
        return BOAT_ERROR;
    }
   
    // Set nonce
    result = BoatTxSetNonce();
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;

    // Set recipient

    UtilityHex2Bin(
                    recipient,
                    20,
                    contract_addr_str,
                    TRIMBIN_TRIM_NO,
                    BOAT_TRUE
                  );

    result = BoatTxSetRecipient(recipient);

    if( result != BOAT_SUCCESS ) return BOAT_ERROR;
    

    // Set value

    result =BoatTxSetValue(NULL);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;


    // Set data (Function Argument)
    
    function_prototye_str = "saveList(bytes32)";
    keccak_256((UINT8 *)function_prototye_str, strlen(function_prototye_str), function_selector);

    data.field_ptr = data_array;
    
    memcpy(data.field_ptr, function_selector, 4);
    
    // First 8 bytes: current time. MSB filled with 0
    current_time = time(NULL);

    if(sizeof(size_t) == 4)
    {
        // 32-bit processor
        memset(data.field_ptr+4, 0x00, 4);
        UtilityUint32ToBigend(data.field_ptr+8, current_time, TRIMBIN_TRIM_NO);
    }
    else
    {
        // 64-bit processor
        UtilityUint64ToBigend(data.field_ptr+4, current_time, TRIMBIN_TRIM_NO);
    }


    // Next 20 bytes is ethereum address of the device
    memcpy(data.field_ptr+12, g_boat_wallet_info.account_info.address, 20);
    

    // Last 4 bytes are Event ID

    UtilityUint32ToBigend(data.field_ptr+32, event, TRIMBIN_TRIM_NO);
    
    data.field_len = 36;
     
    result = BoatTxSetData(&data);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;

    
    // Perform the transaction
    // NOTE: Field v,r,s are calculated automatically
    result = BoatTxSend();
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;

    return BOAT_SUCCESS;
}


UINT32 CaseCallReadListLength(CHAR * contract_addr_str)
{
    CHAR *retval_str;
    UINT256ARRAY list_len_u256_big;
    UINT8 length_of_list_len;
    UINT32  list_len_u32_trimed;
    UINT8 i;

    UINT32 result;

    if( contract_addr_str == NULL )
    {
        return 0;
    }

    retval_str = BoatCallContractFunc(
                                      contract_addr_str,
                                      "readListLength()",
                                      NULL,
                                      0);

    if( retval_str != NULL && strlen(retval_str) != 0)
    {
        BoatLog(BOAT_LOG_NORMAL, "retval of readListLength() is %s.", retval_str);

        memset(&list_len_u256_big, 0x00, 32);
        list_len_u32_trimed = 0;

        // Convert HEX string to UINT256 in bigendian
        length_of_list_len = UtilityHex2Bin(
                        (UINT8*)&list_len_u256_big,
                        32,
                        retval_str,
                        TRIMBIN_TRIM_NO,
                        BOAT_FALSE);

        // Convert UINT256 from bigendian to littleendian
        // In this demo, only low 32 bit of the list_len is considered, i.e. up to
        // 0xFFFFFFFF records are supported

        for( i = 0; i < length_of_list_len && i < 4; i++ )
        {
            ((UINT8 *)&list_len_u32_trimed)[i] = list_len_u256_big[length_of_list_len - 1 - i];
        }

        // Check if the most significant 28 bits are all zeros
        if( length_of_list_len > 4 )
        {
            UINT8 j;
            UINT8 testhighbits;

            testhighbits = 0;
            
            for( j = i; j < length_of_list_len; j++ )
            {
                testhighbits |= list_len_u256_big[length_of_list_len - 1 - j];
            }

            // If any bit in high 28 bits is non-zero, it fails
            if( testhighbits != 0 )
            {
                BoatLog(BOAT_LOG_NORMAL, "Read fails due to too many records.");
                result = 0;
            }
            else
            {
                BoatLog(BOAT_LOG_NORMAL, "Find %u records in list.", list_len_u32_trimed);
                result = list_len_u32_trimed;
            }

        }
        else
        {
            BoatLog(BOAT_LOG_NORMAL, "Find %u records in list.", list_len_u32_trimed);
            result = list_len_u32_trimed;
        }

    }
    else
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to call getdata().");
        result = 0;
    }

    return result;
}


BOAT_RESULT CaseCallReadListByIndex(CHAR * contract_addr_str, UINT32 list_len)
{

    UINT8 func_param[32];
    CHAR *retval_str;
    UINT32 list_index;

    BOAT_RESULT result = BOAT_SUCCESS;

    if( contract_addr_str == NULL )
    {
        return BOAT_ERROR;
    }

    memset(func_param, 0x00, 32);
    
    for( list_index = 0; list_index < list_len; list_index++ )
    {
        UtilityUint32ToBigend(func_param + 28, list_index, TRIMBIN_TRIM_NO);

        retval_str = BoatCallContractFunc(
                                          contract_addr_str,
                                          "readListByIndex(uint256)",
                                          func_param,
                                          32);

        if( retval_str != NULL && strlen(retval_str) != 0)
        {
            BoatLog(BOAT_LOG_NORMAL, "%s", retval_str);
        }
        else
        {
            BoatLog(BOAT_LOG_NORMAL, "Fail to call readListByIndex().");
            result = BOAT_ERROR;
            break;
        }
    }

    return result;
}


int main(int argc, char *argv[])
{
    UINT32 list_len;
    BOAT_RESULT result;
    UINT32 event_id;
    CHAR *keystore_passwd = "boaTwaL1EtaEsp@ssW0rd";
    

    // Usage Example: boatwallet_demo http://192.168.56.1:7545
    
    if( argc != 2 )
    {
        BoatLog(BOAT_LOG_CRITICAL, "Usage: %s http://<IP Address or URL for node>:<port>\n", argv[0]);
        return BOAT_ERROR;
    }

    BoatWalletInit();
    

    
    // Set common parameters such as Node URL, Chain ID, Private key, gasprice, gaslimit
    result = SetCommonParam(argv[1]);
    if( result != BOAT_SUCCESS ) goto main_destruct;
    

    result = BoatWalletSaveWallet((UINT8 *)keystore_passwd, strlen(keystore_passwd), "keystore.sav");
    if( result != BOAT_SUCCESS ) goto main_destruct;
    
    result = BoatWalletLoadWallet((UINT8 *)keystore_passwd, strlen(keystore_passwd), "keystore.sav");
    if( result != BOAT_SUCCESS ) goto main_destruct;


    CaseGetBalance();

    // Case 1: CaseSendEther
    BoatLog(BOAT_LOG_NORMAL, "====== Testing CaseSendEther ======");
    result = CaseSendEther();
    if( result != BOAT_SUCCESS ) goto main_destruct;

    CaseGetBalance();

/*
    // Case 2: CaseCallStoredataSol
    BoatLog(BOAT_LOG_NORMAL, "====== Testing CaseCallStoredataSol ======");
    result = CaseCallStoredataSol("0xa452d62bb8066a1c56802a0fab6a1ae666985691");
    if( result != BOAT_SUCCESS ) goto main_destruct;


    // Case 3: CaseCallGetdataSol
    BoatLog(BOAT_LOG_NORMAL, "====== Testing CaseCallGetdataSol ======");
    result = CaseCallGetdataSol("0xa452d62bb8066a1c56802a0fab6a1ae666985691");
    if( result != BOAT_SUCCESS ) goto main_destruct;


    // Case 4: CaseEthGetStorageAt
    BoatLog(BOAT_LOG_NORMAL, "====== Testing CaseEthGetStorageAt ======");
    result = CaseEthGetStorageAt("0xa452d62bb8066a1c56802a0fab6a1ae666985691");
    if( result != BOAT_SUCCESS ) goto main_destruct;


    // Case 5: CaseCallsaveListSol
    BoatLog(BOAT_LOG_NORMAL, "====== Testing CaseCallsaveListSol ======");
    for( event_id = 0xABCD; event_id < 0xABD0; event_id++ )
    {
        result = CaseCallSaveListSol("0x5b34f5b2f4d6dc79540888d4cbc56eb9af699908", event_id);
        if( result != BOAT_SUCCESS ) goto main_destruct;
    }
    
    // Case 6: CaseGetSavedList
    BoatLog(BOAT_LOG_NORMAL, "====== Testing CaseCallReadListLength and CaseCallReadListByIndex ======");
    list_len = CaseCallReadListLength("0x5b34f5b2f4d6dc79540888d4cbc56eb9af699908");
    if( list_len == 0 ) goto main_destruct;

    result = CaseCallReadListByIndex("0x5b34f5b2f4d6dc79540888d4cbc56eb9af699908", list_len);
    if( result != BOAT_SUCCESS ) goto main_destruct;   
*/
main_destruct:


    BoatWalletDeInit();
    
    return 0;
}
