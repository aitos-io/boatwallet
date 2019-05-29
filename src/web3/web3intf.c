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

/*!@brief Web3 interface

@file web3intf.c contains web3 interface functions for RPC.
*/

#include "wallet/boattypes.h"
#include "utilities/utility.h"

#include "rpc/rpcintf.h"
#include "rpc/curlport.h"

#include "cJSON.h"

#include "web3/web3intf.h"
#include "randgenerator.h"

//!@brief Message ID to distinguish different messages.
UINT32 g_web3_message_id = 0;

//!@brief MAX size of g_web3_json_string_buf
#define WEB3_JSON_STRING_BUF_MAX_SIZE 4096
//!@brief A JSON string buffer used for both REQUEST and RESPONSE
CHAR g_web3_json_string_buf[WEB3_JSON_STRING_BUF_MAX_SIZE];


// Re-use JSON string to store RPC "result" string parsed from JSON RESPONSE
#define WEB3_RESULT_STRING_BUF_MAX_SIZE WEB3_JSON_STRING_BUF_MAX_SIZE
CHAR *g_web3_result_string_buf = g_web3_json_string_buf;


/*!*****************************************************************************
@brief Parse an item from a JSON string

Function: web3_JSON_parse_item()

    This function parse a JSON string and find a specified item in it.

    This function can only find the first level item in the JSON string. It
    doesn't support finding an item in inner JSON struct.

@remark
    This function uses global variable g_web3_result_string_buf to output the
    content of the specified item.

@return
    This function returns BOAT_SUCCESS if the specified item is found. Otherwise
    it returns an error code.
    

@param[out] to_big_ptr
        The buffer to hold the converted binary stream.\n
        It's safe to ensure its size >= (<from_up_to_len>+1)/2 bytes.
        
@param[in] rpc_response_str
        The JSON string to parse.

@param[in] item_name
        The name of the item to search. For example, "result".

*******************************************************************************/
BOAT_RESULT web3_JSON_parse_item(const CHAR *rpc_response_str,
                                const CHAR *item_name)
{
    BOAT_RESULT result;
    
    cJSON *rpc_response_json_ptr = NULL;
    cJSON *web3_item_json_ptr = NULL;
    const char *cjson_error_ptr;
    
    char *web3_item_str;
    UINT32 web3_item_str_len;

    boat_try_declare;

    if( rpc_response_str == NULL || item_name == NULL || strlen(item_name) == 0 )
    {
        BoatLog(BOAT_LOG_NORMAL, "<rpc_response_str> or <item_name> is NULL.");
        boat_throw(BOAT_ERROR_NULL_POINTER, web3_JSON_parse_item_cleanup);
    }
    
    // Obtain RESPONSE object from it's JSON String
    rpc_response_json_ptr = cJSON_Parse(rpc_response_str);
    
    if (rpc_response_json_ptr == NULL)
    {
        cjson_error_ptr = cJSON_GetErrorPtr();
        if (cjson_error_ptr != NULL)
        {
            BoatLog(BOAT_LOG_NORMAL, "Parsing RESPONSE as JSON fails before: %s.", cjson_error_ptr);
        }
        boat_throw(BOAT_ERROR_JSON_PARSE_FAIL, web3_JSON_parse_item_cleanup);
    }

    // Obtain item object from RESPONSE object (e.g. item_name = "result")
    web3_item_json_ptr = cJSON_GetObjectItemCaseSensitive(rpc_response_json_ptr, item_name);

    if (web3_item_json_ptr == NULL)
    {
        BoatLog(BOAT_LOG_NORMAL, "Cannot find \"result\" item in RESPONSE.");
        boat_throw(BOAT_ERROR_JSON_PARSE_FAIL, web3_JSON_parse_item_cleanup);
    }

    // Convert item object to string
    web3_item_str = cJSON_GetStringValue(web3_item_json_ptr);
    
    if( web3_item_str != NULL )
    {
        BoatLog(BOAT_LOG_VERBOSE, "result = %s", web3_item_str);

        web3_item_str_len = strlen(web3_item_str);

        if( web3_item_str_len < WEB3_RESULT_STRING_BUF_MAX_SIZE )
        {
            strcpy(g_web3_result_string_buf, web3_item_str);
        }
        else
        {
            BoatLog(BOAT_LOG_NORMAL, "web3_item_str is too long: %s.", web3_item_str);
            boat_throw(BOAT_ERROR_OUT_OF_MEMORY, web3_JSON_parse_item_cleanup);
        }
    }

    // Clean Up
    cJSON_Delete(rpc_response_json_ptr);

    result = BOAT_SUCCESS;
    

    // Exceptional Clean Up
    boat_catch(web3_JSON_parse_item_cleanup)
    {
        BoatLog(BOAT_LOG_NORMAL, "Exception: %d", boat_exception);

        if( rpc_response_json_ptr != NULL )
        {
            cJSON_Delete(rpc_response_json_ptr);
        }

        result = boat_exception;
    }

    return result;
}


/*!*****************************************************************************
@brief Initialize web3 interface

Function: web3_init()

    This function initialize resources for web3 interface.


@return
    This function always returns BOAT_SUCCESS.
    

@param This function doesn't take any argument.

*******************************************************************************/
BOAT_RESULT web3_init(void)
{
    g_web3_message_id = random32();
    
    return BOAT_SUCCESS;
}



/*!*****************************************************************************
@brief Perform eth_getTransactionCount RPC method and get the transaction count
       of the specified account

Function: web3_eth_getTransactionCount()

    This function calls RPC method eth_getTransactionCount and returns a string
    representing the transaction count of the specified address.

    The typical RPC REQUEST is similar to:
    {"jsonrpc":"2.0","method":"eth_getTransactionCount","params":["0xc94770007dda54cF92009BFF0dE90c06F603a09f","latest"],"id":1}
    
    The typical RPC RESPONSE from blockchain node is similar to:
    {"id":1,"jsonrpc": "2.0","result": "0x1"}


    This function takes 2 arguments. The first is the URL to the blockchian
    node. Protocol and port must be explicitly specified in URL, such as
    "http://127.0.0.1:7545". The second argument is a struct containing all
    RPC call parameters as specified by ethereum.

    See following wiki for details about RPC parameters and return value:
    https://github.com/ethereum/wiki/wiki/JSON-RPC#json-rpc-api

    This function returns a string representing the item "result" of the
    RESPONSE from the RPC call. The buffer storing the string is maintained by
    web3intf and the caller shall NOT modify it, free it or save the address
    for later use.

@return
    This function returns a string representing the transaction count of the\n
    address specified in <param_ptr>.\n
    For example, an address that has initiated 3 transactions will return\n
    "0x3". Note the leading zeros are trimmed as specified in ethereum RPC\n
    interface.\n
    The transaction count is typically used as the "nonce" in a new transaction.\n
    If any error occurs or RPC call timeouts, it returns NULL.
    

@param node_url_str
        A string indicating the URL of blockchain node.

@param param_ptr
        The parameters of the eth_getTransactionCount RPC method.\n
        address_str:\n
            DATA, 20 Bytes - address, a string such as "0x123456..."\n
        block_num_str:\n
            QUANTITY|TAG - a string of integer block number, or "latest", "earliest" or "pending"
        
*******************************************************************************/
CHAR *web3_eth_getTransactionCount(
                                    const char *node_url_str,
                                    const Param_eth_getTransactionCount *param_ptr)
{
    CHAR *rpc_response_str;
    UINT32 rpc_response_len;

    RpcOption rpc_option;

    SINT32 expected_string_size;
    BOAT_RESULT result;
    CHAR *return_value_ptr;
    
    boat_try_declare;
    

    g_web3_message_id++;
    
    if( node_url_str == NULL || param_ptr == NULL)
    {
        BoatLog(BOAT_LOG_NORMAL, "Arguments cannot be NULL.");
        boat_throw(BOAT_ERROR_NULL_POINTER, web3_eth_getTransactionCount_cleanup);
    }
    
   
    // Construct the REQUEST

    expected_string_size = snprintf(
             g_web3_json_string_buf,
             WEB3_JSON_STRING_BUF_MAX_SIZE,
             "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionCount\",\"params\":"
             "[\"%s\",\"%s\"],\"id\":%u}",
             param_ptr->address_str,
             param_ptr->block_num_str,
             g_web3_message_id
            );

    if( expected_string_size >= WEB3_JSON_STRING_BUF_MAX_SIZE - 1)
    {
        boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, web3_eth_getTransactionCount_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "REQUEST: %s", g_web3_json_string_buf);

    // POST the REQUEST through curl

#if RPC_USE_LIBCURL == 1
    rpc_option.node_url_str = node_url_str;
#endif

    RpcSetOpt(&rpc_option);
    
    result = RpcRequestSync(
                    (const UINT8*)g_web3_json_string_buf,   // g_web3_json_string_buf stores REQUEST
                    expected_string_size,
                    (BOAT_OUT UINT8 **)&rpc_response_str,
                    &rpc_response_len);

    if( result != BOAT_SUCCESS )
    {
        BoatLog(BOAT_LOG_NORMAL, "RpcRequestSync() fails.");
        boat_throw(result, web3_eth_getTransactionCount_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "RESPONSE: %s", rpc_response_str);

    // Parse RESPONSE and get web3_result item "result"
    result = web3_JSON_parse_item(rpc_response_str, "result");

    if (result != BOAT_SUCCESS)
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to parse RESPONSE as JSON.");
        boat_throw(BOAT_ERROR_JSON_PARSE_FAIL, web3_eth_getTransactionCount_cleanup);
    }
    

    return_value_ptr = g_web3_result_string_buf;

    // Exceptional Clean Up
    boat_catch(web3_eth_getTransactionCount_cleanup)
    {
        BoatLog(BOAT_LOG_NORMAL, "Exception: %d", boat_exception);
        return_value_ptr = NULL;
    }

    return return_value_ptr;
}


/*!*****************************************************************************
@brief Perform eth_gasPrice RPC method and get the current price per gas in wei
       of the specified network.

Function: web3_eth_gasPrice()

    This function calls RPC method eth_gasPrice and returns a string
    representing the current price per gas in wei of the specified network.

    The typical RPC REQUEST is similar to:
    {"jsonrpc":"2.0","method":"eth_gasPrice","params":[],"id":73}
    
    The typical RPC RESPONSE from blockchain node is similar to:
    {"id":73,"jsonrpc": "2.0","result": "0x09184e72a000"}


    This function takes 1 arguments, which is the URL to the blockchian
    node. Protocol and port must be explicitly specified in URL, such as
    "http://127.0.0.1:7545". The eth_gasPrice RPC method itself doesn't take
    any parameter.

    See following wiki for details about RPC parameters and return value:
    https://github.com/ethereum/wiki/wiki/JSON-RPC#json-rpc-api

    This function returns a string representing the item "result" of the
    RESPONSE from the RPC call. The buffer storing the string is maintained by
    web3intf and the caller shall NOT modify it, free it or save the address
    for later use.

@return
    This function returns a string representing the current price per gas in\n
    wei of the specified network.\n
    For example, if current gas price is 2000000000/gas, it will return\n
    "0x77359400". Note the leading zeros are trimmed as specified in ethereum RPC\n
    interface.\n
    The gasPrice returned from network is a reference for use in a transaction.\n
    Specify a higher gasPrice in transaction may increase the posibility that\n
    the transcaction is get mined quicker and vice versa.\n
    If any error occurs or RPC call timeouts, it returns NULL.
    

@param node_url_str
        A string indicating the URL of blockchain node.

*******************************************************************************/
CHAR *web3_eth_gasPrice(const char *node_url_str)
{
    CHAR *rpc_response_str;
    UINT32 rpc_response_len;

    RpcOption rpc_option;

    SINT32 expected_string_size;
    BOAT_RESULT result;
    CHAR *return_value_ptr;
    
    boat_try_declare;
    

    g_web3_message_id++;
    
    if( node_url_str == NULL)
    {
        BoatLog(BOAT_LOG_NORMAL, "Arguments cannot be NULL.");
        boat_throw(BOAT_ERROR_NULL_POINTER, web3_eth_gasPrice_cleanup);
    }
    
   
    // Construct the REQUEST

    expected_string_size = snprintf(
             g_web3_json_string_buf,
             WEB3_JSON_STRING_BUF_MAX_SIZE,
             "{\"jsonrpc\":\"2.0\",\"method\":\"eth_gasPrice\",\"params\":"
             "[],\"id\":%u}",
             g_web3_message_id
            );

    if( expected_string_size >= WEB3_JSON_STRING_BUF_MAX_SIZE - 1)
    {
        boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, web3_eth_gasPrice_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "REQUEST: %s", g_web3_json_string_buf);

    // POST the REQUEST through curl

#if RPC_USE_LIBCURL == 1
    rpc_option.node_url_str = node_url_str;
#endif

    RpcSetOpt(&rpc_option);
    
    result = RpcRequestSync(
                    (const UINT8*)g_web3_json_string_buf,   // g_web3_json_string_buf stores REQUEST
                    expected_string_size,
                    (BOAT_OUT UINT8 **)&rpc_response_str,
                    &rpc_response_len);

    if( result != BOAT_SUCCESS )
    {
        BoatLog(BOAT_LOG_NORMAL, "RpcRequestSync() fails.");
        boat_throw(result, web3_eth_gasPrice_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "RESPONSE: %s", rpc_response_str);

    // Parse RESPONSE and get web3_result item "result"
    result = web3_JSON_parse_item(rpc_response_str, "result");

    if (result != BOAT_SUCCESS)
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to parse RESPONSE as JSON.");
        boat_throw(BOAT_ERROR_JSON_PARSE_FAIL, web3_eth_gasPrice_cleanup);
    }
    

    return_value_ptr = g_web3_result_string_buf;

    // Exceptional Clean Up
    boat_catch(web3_eth_gasPrice_cleanup)
    {
        BoatLog(BOAT_LOG_NORMAL, "Exception: %d", boat_exception);
        return_value_ptr = NULL;
    }

    return return_value_ptr;
}


/*!*****************************************************************************
@brief Perform eth_getBalance RPC method and get the balance of the specified account

Function: web3_eth_getBalance()

    This function calls RPC method eth_getBalance and returns a string
    representing the balance of the specified address.

    The typical RPC REQUEST is similar to:
    {"jsonrpc":"2.0","method":"eth_getBalance","params":["0xc94770007dda54cF92009BFF0dE90c06F603a09f", "latest"],"id":1}
    
    The typical RPC RESPONSE from blockchain node is similar to:
    {"id":1,"jsonrpc": "2.0","result": "0x0234c8a3397aab58"}


    This function takes 2 arguments. The first is the URL to the blockchian
    node. Protocol and port must be explicitly specified in URL, such as
    "http://127.0.0.1:7545". The second argument is a struct containing all
    RPC call parameters as specified by ethereum.

    See following wiki for details about RPC parameters and return value:
    https://github.com/ethereum/wiki/wiki/JSON-RPC#json-rpc-api

    This function returns a string representing the item "result" of the
    RESPONSE from the RPC call. The buffer storing the string is maintained by
    web3intf and the caller shall NOT modify it, free it or save the address
    for later use.

@return
    This function returns a string representing the balance (Unit: wei, i.e.\n
    1e-18 ETH) of the address specified in <param_ptr>.\n
    For example, an address that has a balance of 1024 wei will return "0x400".\n
    Note the leading zeros are trimmed as specified in ethereum RPC interface.\n
    If any error occurs or RPC call timeouts, it returns NULL.
    

@param node_url_str
        A string indicating the URL of blockchain node.

@param param_ptr
        The parameters of the eth_getTransactionCount RPC method.\n
        address_str:\n
            DATA, 20 Bytes - address to check for balance.\n
        block_num_str:\n
            QUANTITY|TAG - a string of integer block number, or "latest", "earliest" or "pending"
        
*******************************************************************************/
CHAR *web3_eth_getBalance(
                                    const char *node_url_str,
                                    const Param_eth_getBalance *param_ptr)
{
    CHAR *rpc_response_str;
    UINT32 rpc_response_len;

    RpcOption rpc_option;

    SINT32 expected_string_size;
    BOAT_RESULT result;
    CHAR *return_value_ptr;
    
    boat_try_declare;
    

    g_web3_message_id++;
    
    if( node_url_str == NULL || param_ptr == NULL)
    {
        BoatLog(BOAT_LOG_NORMAL, "Arguments cannot be NULL.");
        boat_throw(BOAT_ERROR_NULL_POINTER, web3_eth_getBalance_cleanup);
    }
    
   
    // Construct the REQUEST

    expected_string_size = snprintf(
             g_web3_json_string_buf,
             WEB3_JSON_STRING_BUF_MAX_SIZE,
             "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getBalance\",\"params\":"
             "[\"%s\",\"%s\"],\"id\":%u}",
             param_ptr->address_str,
             param_ptr->block_num_str,
             g_web3_message_id
            );

    if( expected_string_size >= WEB3_JSON_STRING_BUF_MAX_SIZE - 1)
    {
        boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, web3_eth_getBalance_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "REQUEST: %s", g_web3_json_string_buf);

    // POST the REQUEST through curl

#if RPC_USE_LIBCURL == 1
    rpc_option.node_url_str = node_url_str;
#endif

    RpcSetOpt(&rpc_option);
    
    result = RpcRequestSync(
                    (const UINT8*)g_web3_json_string_buf,   // g_web3_json_string_buf stores REQUEST
                    expected_string_size,
                    (BOAT_OUT UINT8 **)&rpc_response_str,
                    &rpc_response_len);

    if( result != BOAT_SUCCESS )
    {
        BoatLog(BOAT_LOG_NORMAL, "RpcRequestSync() fails.");
        boat_throw(result, web3_eth_getBalance_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "RESPONSE: %s", rpc_response_str);

    // Parse RESPONSE and get web3_result item "result"
    result = web3_JSON_parse_item(rpc_response_str, "result");

    if (result != BOAT_SUCCESS)
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to parse RESPONSE as JSON.");
        boat_throw(BOAT_ERROR_JSON_PARSE_FAIL, web3_eth_getBalance_cleanup);
    }
    

    return_value_ptr = g_web3_result_string_buf;

    // Exceptional Clean Up
    boat_catch(web3_eth_getBalance_cleanup)
    {
        BoatLog(BOAT_LOG_NORMAL, "Exception: %d", boat_exception);
        return_value_ptr = NULL;
    }

    return return_value_ptr;
}

/*!*****************************************************************************
@brief Perform eth_sendRawTransaction RPC method.

Function: web3_eth_sendRawTransaction()

    This function calls RPC method eth_sendRawTransaction and returns a string
    representing the transaction hash.

    The typical RPC REQUEST is similar to:
    {"jsonrpc":"2.0","method":"eth_sendRawTransaction","params":["0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675"],"id":1}
    
    The typical RPC RESPONSE from blockchain node is similar to:
    {"id":1,"jsonrpc": "2.0","result": "0xe670ec64341771606e55d6b4ca35a1a6b75ee3d5145a99d05921026d1527331"}

    This function takes 2 arguments. The first is the URL to the blockchian
    node. Protocol and port must be explicitly specified in URL, such as
    "http://127.0.0.1:7545". The second argument is a struct containing all
    RPC call parameters as specified by ethereum.

    See following wiki for details about RPC parameters and return value:
    https://github.com/ethereum/wiki/wiki/JSON-RPC#json-rpc-api

    This function returns a string representing the item "result" of the
    RESPONSE from the RPC call. The buffer storing the string is maintained by
    web3intf and the caller shall NOT modify it, free it or save the address
    for later use.

@return
    This function returns a string representing a 32-byte transaction hash\n
    of the transaction if the blockchain node accepts the transaction in its\n
    pool. If this transaction is not yet avaialble, the returned hash is "0x0".\n    
    If the blockchain node returns error or RPC call timeouts, it returns NULL.\n\n
    Note: A successful return from eth_sendRawTransaction DOES NOT mean the\n
    transaction is confirmed. The caller shall priodically polling the receipt\n
    using eth_getTransactionReceipt method with the transaction hash returned\n
    by eth_sendRawTransaction.


@param node_url_str
        A string indicating the URL of blockchain node.

@param param_ptr
        The parameters of the eth_sendRawTransaction RPC method.\n
        signedtx_str:\n
            DATA, The signed transaction data as a HEX string, with "0x" prefix.

*******************************************************************************/
CHAR *web3_eth_sendRawTransaction(
                                    const char *node_url_str,
                                    const Param_eth_sendRawTransaction *param_ptr)
{
    CHAR *rpc_response_str;
    UINT32 rpc_response_len;

    RpcOption rpc_option;

    SINT32 expected_string_size;
    BOAT_RESULT result;
    CHAR *return_value_ptr;
    
    boat_try_declare;
    

    g_web3_message_id++;
    
    if( node_url_str == NULL || param_ptr == NULL)
    {
        BoatLog(BOAT_LOG_NORMAL, "Arguments cannot be NULL.");
        boat_throw(BOAT_ERROR_NULL_POINTER, web3_eth_sendRawTransaction_cleanup);
    }
    

    // Construct the REQUEST

    expected_string_size = snprintf(
             g_web3_json_string_buf,
             WEB3_JSON_STRING_BUF_MAX_SIZE,
             "{\"jsonrpc\":\"2.0\",\"method\":\"eth_sendRawTransaction\",\"params\":"
             "[\"%s\"],\"id\":%u}",
             param_ptr->signedtx_str,
             g_web3_message_id
            );

    if( expected_string_size >= WEB3_JSON_STRING_BUF_MAX_SIZE - 1)
    {
        boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, web3_eth_sendRawTransaction_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "REQUEST: %s", g_web3_json_string_buf);

    // POST the REQUEST through curl

#if RPC_USE_LIBCURL == 1
    rpc_option.node_url_str = node_url_str;
#endif

    RpcSetOpt(&rpc_option);
    
    result = RpcRequestSync(
                    (const UINT8*)g_web3_json_string_buf,   // g_web3_json_string_buf stores REQUEST
                    expected_string_size,
                    (BOAT_OUT UINT8 **)&rpc_response_str,
                    &rpc_response_len);

    if( result != BOAT_SUCCESS )
    {
        BoatLog(BOAT_LOG_NORMAL, "RpcRequestSync() fails.");
        boat_throw(result, web3_eth_sendRawTransaction_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "RESPONSE: %s", rpc_response_str);
    
    // Parse RESPONSE and get web3_result item "result"
    result = web3_JSON_parse_item(rpc_response_str, "result");

    if (result != BOAT_SUCCESS)
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to parse RESPONSE as JSON.");
        boat_throw(BOAT_ERROR_JSON_PARSE_FAIL, web3_eth_sendRawTransaction_cleanup);
    }
    

    return_value_ptr = g_web3_result_string_buf;

    // Exceptional Clean Up
    boat_catch(web3_eth_sendRawTransaction_cleanup)
    {
        BoatLog(BOAT_LOG_NORMAL, "Exception: %d", boat_exception);
        return_value_ptr = NULL;
    }

    return return_value_ptr;
}



/*!*****************************************************************************
@brief Perform eth_getStorageAt RPC method.

Function: web3_eth_getStorageAt()

    This function calls RPC method eth_getStorageAt and returns a string
    representing the storage data.

    The typical RPC REQUEST is similar to:
    {"jsonrpc":"2.0", "method": "eth_getStorageAt", "params": ["0x295a70b2de5e3953354a6a8344e616ed314d7251", "0x0", "latest"], "id": 1}
    
    The typical RPC RESPONSE from blockchain node is similar to:
    {"jsonrpc":"2.0","id":1,"result":"0x00000000000000000000000000000000000000000000000000000000000004d2"}

    This function takes 2 arguments. The first is the URL to the blockchian
    node. Protocol and port must be explicitly specified in URL, such as
    "http://127.0.0.1:7545". The second argument is a struct containing all
    RPC call parameters as specified by ethereum.

    See following wiki for details about RPC parameters and return value:
    https://github.com/ethereum/wiki/wiki/JSON-RPC#json-rpc-api

    The way to calculate <position_str> is quite complicated if the datum is
    not a simple element type. The position conforms to the storage slot as
    specified by Solidity memory layout.

    Solidity Memory Layout:
    https://solidity.readthedocs.io/en/latest/miscellaneous.html#layout-of-state-variables-in-storage

    A easy-to-understand explanation can be found at:
    https://medium.com/aigang-network/how-to-read-ethereum-contract-storage-44252c8af925
    

    This function returns a string representing the item "result" of the
    RESPONSE from the RPC call. The buffer storing the string is maintained by
    web3intf and the caller shall NOT modify it, free it or save the address
    for later use.

@return
    This function returns a string representing a 32-byte value of the data\n
    stored at slot <position_str> of <address_str> of <block_num_str> as\n
    specified in <param_ptr>. Note that multiple data may be packed in one\n
    storage slot. See Solidity Memory Layout to understand how they are packed.\n
    If the blockchain node returns error or RPC call timeouts, it returns NULL.


@param node_url_str
        A string indicating the URL of blockchain node.

@param param_ptr
        The parameters of the eth_getStorageAt RPC method.\n        
        address_str:\n
            DATA, 20 Bytes - address, a string such as "0x123456..."\n
        position_str:\n
            QUANTITY - integer of the position in the storage.\n        
        block_num_str:\n
            QUANTITY|TAG - a string of integer block number, or "latest", "earliest" or "pending"

*******************************************************************************/
CHAR *web3_eth_getStorageAt(
                                    const char *node_url_str,
                                    const Param_eth_getStorageAt *param_ptr)
{
    CHAR *rpc_response_str;
    UINT32 rpc_response_len;

    RpcOption rpc_option;

    SINT32 expected_string_size;
    BOAT_RESULT result;
    CHAR *return_value_ptr;
    
    boat_try_declare;
    

    g_web3_message_id++;
    
    if( node_url_str == NULL || param_ptr == NULL)
    {
        BoatLog(BOAT_LOG_NORMAL, "Arguments cannot be NULL.");
        boat_throw(BOAT_ERROR_NULL_POINTER, web3_eth_getStorageAt_cleanup);
    }
    

    // Construct the REQUEST

    expected_string_size = snprintf(
             g_web3_json_string_buf,
             WEB3_JSON_STRING_BUF_MAX_SIZE,
             "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getStorageAt\",\"params\":"
             "[\"%s\",\"%s\",\"%s\"],\"id\":%u}",
             param_ptr->address_str,
             param_ptr->position_str,
             param_ptr->block_num_str,
             g_web3_message_id
            );

    if( expected_string_size >= WEB3_JSON_STRING_BUF_MAX_SIZE - 1)
    {
        boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, web3_eth_getStorageAt_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "REQUEST: %s", g_web3_json_string_buf);
    
    // POST the REQUEST through curl

#if RPC_USE_LIBCURL == 1
    rpc_option.node_url_str = node_url_str;
#endif

    RpcSetOpt(&rpc_option);
    
    result = RpcRequestSync(
                    (const UINT8*)g_web3_json_string_buf,   // g_web3_json_string_buf stores REQUEST
                    expected_string_size,
                    (BOAT_OUT UINT8 **)&rpc_response_str,
                    &rpc_response_len);

    if( result != BOAT_SUCCESS )
    {
        BoatLog(BOAT_LOG_NORMAL, "RpcRequestSync() fails.");
        boat_throw(result, web3_eth_getStorageAt_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "RESPONSE: %s", rpc_response_str);

    // Parse RESPONSE and get web3_result item "result"
    result = web3_JSON_parse_item(rpc_response_str, "result");

    if (result != BOAT_SUCCESS)
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to parse RESPONSE as JSON.");
        boat_throw(BOAT_ERROR_JSON_PARSE_FAIL, web3_eth_getStorageAt_cleanup);
    }
    

    return_value_ptr = g_web3_result_string_buf;

    // Exceptional Clean Up
    boat_catch(web3_eth_getStorageAt_cleanup)
    {
        BoatLog(BOAT_LOG_NORMAL, "Exception: %d", boat_exception);
        return_value_ptr = NULL;
    }

    return return_value_ptr;
}


/*!*****************************************************************************
@brief Perform eth_getTransactionReceipt RPC method.

Function: web3_eth_getTransactionReceipt()

    This function calls RPC method eth_getTransactionReceipt and returns a
    string representing the result.status of the receipt object of the
    specified transaction hash.

    The typical RPC REQUEST is similar to:
    {"jsonrpc":"2.0","method":"eth_getTransactionReceipt","params":["0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"],"id":1}
    
    The typical RPC RESPONSE from blockchain node is similar to:
>    {\n
>    "id":1,\n
>    "jsonrpc":"2.0",\n
>    "result": {\n
>         transactionHash: '0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238',\n
>         transactionIndex:  '0x1', // 1\n
>         blockNumber: '0xb', // 11\n
>         blockHash: '0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b',\n
>         cumulativeGasUsed: '0x33bc', // 13244\n
>         gasUsed: '0x4dc', // 1244\n
>         contractAddress: '0xb60e8dd61c5d32be8058bb8eb970870f07233155', // or null, if none was created\n
>         logs: [{\n
>             // logs as returned by getFilterLogs, etc.\n
>         }, ...],\n
>         logsBloom: "0x00...0", // 256 byte bloom filter\n
>         status: '0x1'\n
>      }\n
>    }


    This function takes 2 arguments. The first is the URL to the blockchian
    node. Protocol and port must be explicitly specified in URL, such as
    "http://127.0.0.1:7545". The second argument is a struct containing all
    RPC call parameters as specified by ethereum.

    See following wiki for details about RPC parameters and return value:
    https://github.com/ethereum/wiki/wiki/JSON-RPC#json-rpc-api

    This function returns a string representing the item "result.status" of the
    RESPONSE from the RPC call. The buffer storing the string is maintained by
    web3intf and the caller shall NOT modify it, free it or save the address
    for later use.

@return
    This function returns a string representing the status of the transaction\n
    receipt, "0x1" for success and "0x0" for failure. If the transaction is\n
    pending, it returns a null string i.e. a string containing only '\0'\n
    instead of a NULL pointer.\n
    If any error occurs or RPC call timeouts, it returns NULL.
    

@param node_url_str
        A string indicating the URL of blockchain node.

@param param_ptr
        The parameters of the eth_getTransactionReceipt RPC method.\n        
        tx_hash_str:\n
            DATA, 32 Bytes - hash of a transaction
        
*******************************************************************************/
CHAR *web3_eth_getTransactionReceiptStatus(
                                    const char *node_url_str,
                                    const Param_eth_getTransactionReceipt *param_ptr)
{
    CHAR *rpc_response_str;
    UINT32 rpc_response_len;
    cJSON *rpc_response_json_ptr;
    cJSON *web3_result_json_ptr;
    cJSON *web3_result_status_json_ptr;
    CHAR *web3_result_status_str;
    UINT32 web3_result_status_str_len;
    const char *cjson_error_ptr;

    RpcOption rpc_option;

    SINT32 expected_string_size;
    BOAT_RESULT result;
    CHAR *return_value_ptr;
    
    boat_try_declare;
    

    g_web3_message_id++;
    
    if( node_url_str == NULL || param_ptr == NULL)
    {
        BoatLog(BOAT_LOG_NORMAL, "Arguments cannot be NULL.");
        boat_throw(BOAT_ERROR_NULL_POINTER, web3_eth_getTransactionReceiptStatus_cleanup);
    }
    
   
    // Construct the REQUEST

    expected_string_size = snprintf(
             g_web3_json_string_buf,
             WEB3_JSON_STRING_BUF_MAX_SIZE,
             "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionReceipt\",\"params\":"
             "[\"%s\"],\"id\":%u}",
             param_ptr->tx_hash_str,
             g_web3_message_id
            );

    if( expected_string_size >= WEB3_JSON_STRING_BUF_MAX_SIZE - 1)
    {
        boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, web3_eth_getTransactionReceiptStatus_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "REQUEST: %s", g_web3_json_string_buf);

    // POST the REQUEST through curl

#if RPC_USE_LIBCURL == 1
    rpc_option.node_url_str = node_url_str;
#endif

    RpcSetOpt(&rpc_option);
    
    result = RpcRequestSync(
                    (const UINT8*)g_web3_json_string_buf,   // g_web3_json_string_buf stores REQUEST
                    expected_string_size,
                    (BOAT_OUT UINT8 **)&rpc_response_str,
                    &rpc_response_len);

    if( result != BOAT_SUCCESS )
    {
        BoatLog(BOAT_LOG_NORMAL, "RpcRequestSync() fails.");
        boat_throw(result, web3_eth_getTransactionReceiptStatus_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "RESPONSE: %s", rpc_response_str);

    // Obtain RESPONSE object from it's JSON String
    rpc_response_json_ptr = cJSON_Parse(rpc_response_str);
    
    if (rpc_response_json_ptr == NULL)
    {
        cjson_error_ptr = cJSON_GetErrorPtr();
        if (cjson_error_ptr != NULL)
        {
            BoatLog(BOAT_LOG_NORMAL, "Parsing RESPONSE as JSON fails before: %s.", cjson_error_ptr);
        }
        boat_throw(BOAT_ERROR_JSON_PARSE_FAIL, web3_eth_getTransactionReceiptStatus_cleanup);
    }

    // Obtain result object from RESPONSE object
    web3_result_json_ptr = cJSON_GetObjectItemCaseSensitive(rpc_response_json_ptr, "result");

    if (web3_result_json_ptr == NULL)
    {
        BoatLog(BOAT_LOG_NORMAL, "Cannot find \"result\" item in RESPONSE.");
        boat_throw(BOAT_ERROR_JSON_PARSE_FAIL, web3_eth_getTransactionReceiptStatus_cleanup);
    }

    // Obtain result.status object from result object
    web3_result_status_json_ptr = cJSON_GetObjectItemCaseSensitive(web3_result_json_ptr, "status");

    if (web3_result_status_json_ptr == NULL)
    {
        BoatLog(BOAT_LOG_NORMAL, "Cannot find \"result.status\" item in RESPONSE.");
        boat_throw(BOAT_ERROR_JSON_PARSE_FAIL, web3_eth_getTransactionReceiptStatus_cleanup);
    }


    // Convert result.status object to string
    web3_result_status_str = cJSON_GetStringValue(web3_result_status_json_ptr);
    
    if( web3_result_status_str != NULL )
    {
        BoatLog(BOAT_LOG_VERBOSE, "result.status = %s", web3_result_status_str);

        web3_result_status_str_len = strlen(web3_result_status_str);

        if( web3_result_status_str_len < WEB3_RESULT_STRING_BUF_MAX_SIZE )
        {
            strcpy(g_web3_result_string_buf, web3_result_status_str);
        }
        else
        {
            BoatLog(BOAT_LOG_NORMAL, "result.status is too long: %s.", web3_result_status_str);
            boat_throw(BOAT_ERROR_OUT_OF_MEMORY, web3_eth_getTransactionReceiptStatus_cleanup);
        }
    }

    // Clean Up
    cJSON_Delete(rpc_response_json_ptr);

    return_value_ptr = g_web3_result_string_buf;


    // Exceptional Clean Up
    boat_catch(web3_eth_getTransactionReceiptStatus_cleanup)
    {
        BoatLog(BOAT_LOG_NORMAL, "Exception: %d", boat_exception);
        return_value_ptr = NULL;
    }

    return return_value_ptr;
}





/*!*****************************************************************************
@brief Perform eth_call RPC method.

Function: web3_eth_call()

    This function calls RPC method eth_call and returns the return value of the
    specified contract function.

    This function can only call contract functions that don't change the block
    STATE. To call contract functions that may change block STATE, use
    eth_sendRawTransaction instead.

    The typical RPC REQUEST is similar to:
>    {"jsonrpc":"2.0","method":"eth_call","params":[{\n
>      "from": "0xb60e8dd61c5d32be8058bb8eb970870f07233155", // Optional \n
>      "to": "0xd46e8dd67c5d32be8058bb8eb970870f07244567",\n
>      "gas": "0x76c0", // Optional, 30400\n
>      "gasPrice": "0x9184e72a000", // Optional, 10000000000000\n
>      "value": "0x0", // Optional \n
>      "data": "0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870" //Optional \n
>    }],"id":1}
    
    The typical RPC RESPONSE from blockchain node is similar to:
    {"id":1,"jsonrpc": "2.0","result": "0x0"}

    This function takes 2 arguments. The first is the URL to the blockchian
    node. Protocol and port must be explicitly specified in URL, such as
    "http://127.0.0.1:7545". The second argument is a struct containing all
    RPC call parameters as specified by ethereum.

    See following wiki for details about RPC parameters and return value:
    https://github.com/ethereum/wiki/wiki/JSON-RPC#json-rpc-api

    This function could call a contract function without creating a transaction
    on the block chain. The only mandatory parameter of eth_call is "to", which
    is the address of the contract. Typically parameter "data" is also mandatory,
    which is composed of 4-byte function selector followed by 0 or more parameters
    regarding the contract function being called. See Ethereum Contract ABI for
    the details about how to compose "data" field:
    https://github.com/ethereum/wiki/wiki/Ethereum-Contract-ABI

    An eth_call doesn't consume gas, but it's a good practice to specifiy the
    "gas" parameter for better compatibility.
    
    This function returns a string representing the item "result" of the
    RESPONSE from the RPC call. The buffer storing the string is maintained by
    web3intf and the caller shall NOT modify it, free it or save the address
    for later use.

@return
    This function returns a string representing the returned value of the called\n
    contract function.

@param node_url_str
        A string indicating the URL of blockchain node.

@param param_ptr
        The parameters of the eth_call RPC method. Some optional parameters are omitted.\n
        to: The address of the contract.\n
        gas: The gasLimit.\n
        gasPrice: The gasPrice in wei.\n
        data: The function selector followed by parameters.\n

*******************************************************************************/
CHAR *web3_eth_call(
                                    const char *node_url_str,
                                    const Param_eth_call *param_ptr)
{
    CHAR *rpc_response_str;
    UINT32 rpc_response_len;

    RpcOption rpc_option;

    SINT32 expected_string_size;
    BOAT_RESULT result;
    CHAR *return_value_ptr;
    
    boat_try_declare;
    

    g_web3_message_id++;
    
    if( node_url_str == NULL || param_ptr == NULL)
    {
        BoatLog(BOAT_LOG_NORMAL, "Arguments cannot be NULL.");
        boat_throw(BOAT_ERROR_NULL_POINTER, web3_eth_call_cleanup);
    }
    

    // Construct the REQUEST

    expected_string_size = snprintf(
             g_web3_json_string_buf,
             WEB3_JSON_STRING_BUF_MAX_SIZE,
             "{\"jsonrpc\":\"2.0\",\"method\":\"eth_call\",\"params\":"
             "[{\"to\":\"%s\",\"gas\":\"%s\",\"gasPrice\":\"%s\",\"data\":\"%s\"}],\"id\":%u}",
             param_ptr->to,
             param_ptr->gas,
             param_ptr->gasPrice,
             param_ptr->data,
             g_web3_message_id
            );


    if( expected_string_size >= WEB3_JSON_STRING_BUF_MAX_SIZE - 1)
    {
        boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, web3_eth_call_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "REQUEST: %s", g_web3_json_string_buf);

    // POST the REQUEST through curl

#if RPC_USE_LIBCURL == 1
    rpc_option.node_url_str = node_url_str;
#endif

    RpcSetOpt(&rpc_option);
    
    result = RpcRequestSync(
                    (const UINT8*)g_web3_json_string_buf,   // g_web3_json_string_buf stores REQUEST
                    expected_string_size,
                    (BOAT_OUT UINT8 **)&rpc_response_str,
                    &rpc_response_len);

    if( result != BOAT_SUCCESS )
    {
        BoatLog(BOAT_LOG_NORMAL, "RpcRequestSync() fails.");
        boat_throw(result, web3_eth_call_cleanup);
    }

    BoatLog(BOAT_LOG_VERBOSE, "RESPONSE: %s", rpc_response_str);
    
    // Parse RESPONSE and get web3_result item "result"
    result = web3_JSON_parse_item(rpc_response_str, "result");

    if (result != BOAT_SUCCESS)
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to parse RESPONSE as JSON.");
        boat_throw(BOAT_ERROR_JSON_PARSE_FAIL, web3_eth_call_cleanup);
    }
    

    return_value_ptr = g_web3_result_string_buf;

    // Exceptional Clean Up
    boat_catch(web3_eth_call_cleanup)
    {
        BoatLog(BOAT_LOG_NORMAL, "Exception: %d", boat_exception);
        return_value_ptr = NULL;
    }

    return return_value_ptr;
}

