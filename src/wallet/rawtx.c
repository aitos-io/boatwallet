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

/*!@brief Perform RAW transaction

@file
rawtx.c contains functions to construct a raw transaction, serialize it with RLP,
perform it and wait for its receipt.
*/

#include "wallet/boattypes.h"
#include "utilities/utility.h"
#include "wallet/boatwallet.h"
#include "web3/web3intf.h"
#include "wallet/rawtx.h"




/*!*****************************************************************************
@brief Encode a field as per RLP encoding rules.

Function: RlpFieldEncode()

    This function encodes a field as per RLP encoding rules.
    
    The output encoded stream's length varies between <field_len> and
    <field_len> + 9 bytes.
    
    There are 3 possible types of RLP encoding structure as per RLP rules:
    1. encoded = <field>, if field_len is 1
    2. encoded = <1 byte prefix>|<field>, if field_len is in range [0,55] except 1
    3. encoded = <1 byte prefix>|<field_len>|<field>, if field_len >= 56
    where "|" means concatenaion.

    NOTE: In Case 3, <field_len> is represented in bigendian with leading zeros
          trimed. For example, 0x00000123 is represented as {0x01,0x23} in
          memory from low byte address to high byte address and sizeof(field_len)
          is 2 bytes.
    
    The maximum sizeof(field_len) RLP rules allowing is 8 bytes, which means the
    maximum length of a field is 2^64 - 1 bytes. Thus in maximum case the total
    length of the encoded output stream is (9 + field_len), including:
    <1 byte prefix> + <8 bytes field_len> + <field_len bytes of field data>.
    
    Yet because the length of a field is reasonably up to some kilo-bytes, thus
    sizeof(field_len) is typically 1 or 2 bytes.
    
    For simplicity, it's safe to allocate at least (9 + field_len) bytes to
    hold the output encoded result.



    RESTRICTION:
    Though RLP rules allow field length to be up to 2^64 - 1 bytes, this function
    restricts field length to no more than BOAT_REASONABLE_MAX_LEN bytes.
    
    
    RESTRICTION:
    This function doesn't support nested structure. To encode a nested
    structure, the caller should split it into multiple steps.


    Especially for a basic LIST structure such as [str0, str1], which is a LIST
    nesting 2 string elements, there is a way to improve performance.

    Basically the caller should split it into 3 steps:
        Step 1. encoded_a = encode(str1);
        Step 2. encoded_b = encode(str2);
        Step 3. encoded_result = encode([encoded_a | encoded_b]);
                
    To reduce the memory usage and avoid unnecessary memory copy, it's desired
    to place encoded_a and encoded_b continuously in memory and encode only the
    LIST header in Step 3 and prefix it to encoded_a|encoded_b. The caller
    should reserve at least 9 bytes for the LIST header. Thus the typical
    process looks like:

        Step 0   Reserve 9 bytes at the beginning of the RLP output buffer.
                 |  9 bytes  |

        Step 1.1 Calculate RLP header of str1 and save it after the 9 reserved
                 bytes.
                 |  9 bytes  | header1 |

        Step 1.2 Copy str1 to RLP output buffer following its header.
                 |  9 bytes  | header1 | str1 |
                 
        Step 2   Repeat similar steps for str2 and save it in RLP output buffer
                 following str1.
                 |  9 bytes  | header1 |str1 | header2 | str2 |
        
        Step 3   Encode LIST header and save it in RLP output buffer before
                 header1. RLP stream encoded in Step 1 and 2 are not copied.
                 |LIST header| header1 |str1 | header2 | str2 |

    This function takes an argument <prefix_header_to_field> to control the
    behavior. If <prefix_header_to_field> is BOAT_TRUE, the header of <field_ptr>
    is encoded and prefixed to <field_ptr>. The caller should MAKE SURE there
    are at least 9 bytes reserved before the address specified by <field_ptr>.

    To avoid any accidently misuse of <prefix_header_to_field>, if it's set
    BOAT_TRUE, <rlp_output_ptr> MUST be NULL.


@return 
    If <prefix_header_to_field> is BOAT_FALSE:\n
    This function returns the address of the byte immediately after the last
    encoded RLP byte if the encoding is successful.\n\n
    If <prefix_header_to_field> is BOAT_TRUE:\n
    This function returns the address of the first byte of the encoded header
    if the encoding is successful.\n\n
    If encoding fails, it returns NULL.


    @param[out] rlp_output_ptr
        The beginning address of the space to store encoded RLP stream.
        The caller should allocate enough space according to above description.
        If <prefix_header_to_field> is BOAT_TRUE, this argument MUST be NULL.

    @param[in] field_ptr
        The address of the field to encode.

    @param[in] field_len
        The length of <field_ptr> in bytes.
        
    @param[in] field_type
        Either RLP_FIELD_TYPE_STRING or RLP_FIELD_TYPE_LIST.

    @param[in] prefix_header_to_field
        BOAT_TURE:  Only encode header of the field and prefix the header to <field_ptr>.
                   To avoid accidently setting it BOAT_TRUE, <rlp_output_ptr> MUST be
                   NULL in this case.
                   It's used for encoding nested LIST.\n
        BOAT_FALSE: Normally encode header and field content.


*******************************************************************************/
UINT8 *RlpFieldEncode(
                BOAT_OUT UINT8 *rlp_output_ptr,  // Pointing to the space to store encoded RLP stream
                UINT8 *field_ptr,           // The field to encode
                UINT32 field_len,           // Length of the field in bytes
                RlpFieldType field_type,    // RLP_FIELD_TYPE_STRING or RLP_FIELD_TYPE_LIST
                BOATBOOL prefix_header_to_field  // Prefix RLP header to the field if BOAT_TRUE
              )
{
    UINT32 offset;
    UINT8 prefix_base;
    UINT8 trimmed_sizeof_field_len;
    boat_try_declare;

    if( field_ptr == NULL && field_len != 0)
    {
        BoatLog(BOAT_LOG_NORMAL, "<field_ptr> cannot be null unless its length is 0.");
        boat_throw(BOAT_ERROR_NULL_POINTER, RlpFieldEncode_cleanup);
    }

    if( field_len > BOAT_REASONABLE_MAX_LEN )
    {
        BoatLog(BOAT_LOG_NORMAL, "<field_len> = %u exceeds BOAT_REASONABLE_MAX_LEN.", field_len);
        boat_throw(BOAT_ERROR_INVALID_LENGTH, RlpFieldEncode_cleanup);
    }

    if( rlp_output_ptr == NULL && prefix_header_to_field == BOAT_FALSE )
    {
        BoatLog(BOAT_LOG_NORMAL, "<prefix_header_to_field> is FALSE but <rlp_output_ptr> is NULL.");
        boat_throw(BOAT_ERROR_NULL_POINTER, RlpFieldEncode_cleanup);
    }
    
    if( rlp_output_ptr != NULL && prefix_header_to_field != BOAT_FALSE )
    {
        BoatLog(BOAT_LOG_NORMAL, "<prefix_header_to_field> is TRUE but <rlp_output_ptr> is not NULL.");
        boat_throw(BOAT_ERROR_INCOMPATIBLE_ARGUMENTS, RlpFieldEncode_cleanup);
    }

    offset = 0;

    // Case 1. encoded = <field>, if field_len is 1    
    if( field_len == 1 && field_ptr[0] <= 0x7f)
    {
        if( prefix_header_to_field == BOAT_FALSE )
        {
            // <field> only
            rlp_output_ptr[offset++] = field_ptr[0];
        }
        else
        {
            offset = 0;
        }
    }
    else
    {
        if( field_type == RLP_FIELD_TYPE_STRING )
        {
            prefix_base = 0x80;
        }
        else // field_type == RLP_FIELD_TYPE_LIST
        {
            prefix_base = 0xC0;
        }
 
        // Case 2. encoded = <1 byte prefix>|<field>, if field_len is in range [0,55] except 1
        if( field_len <= 55 )
        {
            if( prefix_header_to_field == BOAT_FALSE )
            {
                // <prefix>
                rlp_output_ptr[offset++] = prefix_base + field_len;
                
                // <field>
                if( field_len != 0 )
                {
                    memcpy(rlp_output_ptr + offset, field_ptr, field_len);
                    offset += field_len;
                }
            }
            else
            {
                // prefix <prefix> to <field_ptr>
                offset++;
                *(field_ptr - offset) = prefix_base + field_len;
            }
                
        }
        else // Case 3. encoded = <1 byte prefix>|<field_len>|<field>, if field_len >= 56
        {
            if( prefix_header_to_field == BOAT_FALSE )
            {
                UINT8 trimmed_field_len[8];
                
                // Calculate trimmed size of <field_len>
                trimmed_sizeof_field_len = UtilityUint32ToBigend(
                                                            trimmed_field_len,
                                                            field_len,
                                                            TRIMBIN_LEFTTRIM
                                                         );

                // <prefix>
                rlp_output_ptr[offset++] = prefix_base + trimmed_sizeof_field_len + 55;

                // <field_len>
                memcpy(rlp_output_ptr + offset, trimmed_field_len, trimmed_sizeof_field_len);
                offset += trimmed_sizeof_field_len;
                
                // <field>
                memcpy(rlp_output_ptr + offset, field_ptr, field_len);
                offset += field_len;
            }
            else
            {
                UINT32 temp_bigend_field_len;
                
                // Prefix <field_len> to <field_ptr>
                trimmed_sizeof_field_len = UtilityUint32ToBigend(
                                                            (UINT8*)&temp_bigend_field_len,
                                                            field_len,
                                                            TRIMBIN_LEFTTRIM
                                                         );
                offset = trimmed_sizeof_field_len;
                
                memcpy( field_ptr - offset,
                        &temp_bigend_field_len,
                        trimmed_sizeof_field_len);

                // Prefix <prefix> to <field_len>
                offset++;
                *(field_ptr - offset) = prefix_base + trimmed_sizeof_field_len + 55;
            }
        }
    }

    boat_catch(RlpFieldEncode_cleanup)
    {
        BoatLog(BOAT_LOG_NORMAL, "Exception: %d", boat_exception);
        return(NULL);
    }

    if( prefix_header_to_field == BOAT_FALSE )
    {    
        return rlp_output_ptr + offset;
    }
    else
    {
        return field_ptr - offset;
    }
}


/*!*****************************************************************************
@brief This function estimates the encoded RLP stream's size for a transaction.
    
Function: TxRlpStreamSizeEstimate()

    This function estimates the encoded RLP stream's size for a transaction.
    It's safe to allocate a buffer of this size to hold RLP stream.
    
    As per RLP encoding rules an encoded field consists of a header up to 9
    bytes and the field content itself. Thus the maximum possible size of an
    encoded field is <field_len> + 9 bytes.

    A raw transaction consisits 9 fields which are packed in a LIST and thus
    the estimated maximum size is: 
    
        9 bytes + (9 bytes * 9 fields) + sum of all fields' length
        
        where the first 9 bytes is the maximum size of the header of the LIST
        and the following (9 bytes * 9 fields) is the maximum sum of the 9
        fields' header size.
        
    Because field v,r,s are calculated during constructing the transaction,
    their exact lengthes are unknown before the transaction is signed. Thus
    their lengthes are estimated using their maximum possible length.
    

@return
    This function returns the estimated maximum size of the encoded RLP stream
    of the specified transactions.\n
    If any error is encountered, it returns 0.
    

@param[in] tx_info_ctx_ptr
        A pointer to the transaction context.

*******************************************************************************/
UINT32 TxRlpStreamSizeEstimate(TxInfo *tx_info_ctx_ptr)
{
    UINT64 estimated_size;
    boat_try_declare;

    estimated_size = 0;
    
    if( tx_info_ctx_ptr == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "<tx_info_ctx_ptr> cannot be null.");
        boat_throw(BOAT_ERROR_NULL_POINTER, TxRlpStreamSizeEstimate_cleanup);
    }

    estimated_size =   tx_info_ctx_ptr->rawtx_fields.nonce.field_len
                     + tx_info_ctx_ptr->rawtx_fields.gasprice.field_len
                     + tx_info_ctx_ptr->rawtx_fields.gaslimit.field_len
                     + sizeof(tx_info_ctx_ptr->rawtx_fields.recipient)
                     + tx_info_ctx_ptr->rawtx_fields.value.field_len
                     + tx_info_ctx_ptr->rawtx_fields.data.field_len
                     + sizeof(tx_info_ctx_ptr->rawtx_fields.v.field)
                     + sizeof(tx_info_ctx_ptr->rawtx_fields.sig.r32B)
                     + sizeof(tx_info_ctx_ptr->rawtx_fields.sig.s32B)
                     + 9 + 9*9;

    if( estimated_size > BOAT_REASONABLE_MAX_LEN )
    {
        BoatLog(BOAT_LOG_NORMAL, "Too big estimated_size of the transaction: %llu", estimated_size);
        boat_throw(BOAT_ERROR_INVALID_LENGTH, TxRlpStreamSizeEstimate_cleanup);
    }

    
    boat_catch(TxRlpStreamSizeEstimate_cleanup)
    {
        BoatLog(BOAT_LOG_NORMAL, "Exception: %d", boat_exception);
        return(0);
    }

    return((UINT32)estimated_size);
}


/*!*****************************************************************************
@brief Construct a raw transacton and encodes it as per RLP rules.

Function: RawtxPerform()

    This function constructs a raw transacton and encodes it as per RLP rules.
    
    AN INTRODUCTION OF HOW RAW TRANSACTION IS CONSTRUCTED
    
    [FIELDS IN A RAW TRANSACTION]
    
    A RAW transaction consists of following 9 fields:
        1. nonce;
        2. gasprice;
        3. gaslimit;
        4. recipient;
        5. value(optional);
        6. data(optional);
        7. v;
        8. signature.r;
        9. signature.s;

    These transaction fields are encoded as elements of a LIST in above order
    as per RLP encoding rules. "LIST" is a type of RLP field.


    EXCEPTION:
    
    For Ethereum any fields (except <recipient>) having a value of zero are
    treated as NULL stream in RLP encoding instead of 1-byte-size stream whose
    value is 0. For example, nonce = 0 is encoded as 0x80 which represents NULL
    instead of 0x00 which represents a 1-byte-size stream whose value is 0.


    [HOW TO CONSTRUCT A RAW TRANSACTION]
    
    A RAW transaction is constructed in 4 steps in different ways according to
    the blockchain network's EIP-155 compatibility. 

    See following article for details about EIP-155: 
    https://github.com/ethereum/EIPs/blob/master/EIPS/eip-155.md

    
    CASE 1: If the blockchain network does NOT support EIP-155:
    
        Step 1: Encode a LIST containing only the first 6 fields.
        Step 2: Calculate SHA3 hash of the encoded stream in Step 1.
        Step 3: Sign the hash in Step 2. This generates r, s and parity (0 or 1) for recovery identifier.
        Step 4: Encode a LIST containing all 9 fields, where
                First 6 fields are same as what they are;
                v = parity + 27, where parity is given in Step 3;
                r and s are given in Step 3.


    CASE 2: If the blockchain network DOES support EIP-155:

        Step 1: Encode all 9 fields (a LIST containing all 9 fields), where
                First 6 fields are same as what they are;
                v = Chain ID;
                r = 0;
                s = 0;

                NOTE: zero value fields other than <recipient> are encoded as NULL stream.

        Step 2: Same as CASE 1.
        Step 3: Same as CASE 1.

        Step 4: Encode a LIST containing all 9 fields, where
                First 6 fields are same as what they are;
                v = Chain ID * 2 + parity + 35, where parity is given in Step 3;
                r and s are given in Step 3.


@return
    This function returns BOAT_SUCCESS if successful. Otherwise it returns BOAT_ERROR.
    

@param[in] boat_wallet_info_ptr
        A pointer to wallet infor structure.

@param[in] tx_info_ctx_ptr
        A pointer to the context of the transaction.

*******************************************************************************/
BOAT_RESULT RawtxPerform(BoatWalletInfo *boat_wallet_info_ptr, BOAT_INOUT TxInfo *tx_info_ctx_ptr)
{
    unsigned int chain_id_len;
        
    CHAR *tx_hash_str;
    CHAR tx_hash[67];
    CHAR *tx_status_str;

    
    #define RLP_STREAM_RESERVE_HEADER 9
    UINT32 rlp_stream_size_estimate;
    CHAR *rlp_stream_hex_str = NULL;    // Storage for RLP stream HEX string for use with web3 interface
    UINT8 *rlp_stream_start_position_ptr = NULL; // Point to storeage of RLP stream binary
    UINT8 *rlp_stream_current_position_ptr;
    UINT8 *rlp_stream_v_position_ptr;
    UINT32 message_len;
    UINT8 message_digest[32];
    UINT8 sig_parity;
    UINT32 v;

    Param_eth_sendRawTransaction param_eth_sendRawTransaction;
    Param_eth_getTransactionReceipt param_eth_getTransactionReceipt;
    SINT32 tx_mined_timeout;
    
    BOAT_RESULT result;
    boat_try_declare;


    if( boat_wallet_info_ptr == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "<boat_wallet_info_ptr> cannot be null.");
        boat_throw(BOAT_ERROR_NULL_POINTER, RawtxPerform_cleanup);
    }
    
    if( tx_info_ctx_ptr == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "<tx_info_ctx_ptr> cannot be null.");
        boat_throw(BOAT_ERROR_NULL_POINTER, RawtxPerform_cleanup);
    }
    
    rlp_stream_size_estimate = TxRlpStreamSizeEstimate(tx_info_ctx_ptr);

    // Allocate memory for RLP stream binary
    rlp_stream_start_position_ptr = BoatMalloc(rlp_stream_size_estimate);
    
    if( rlp_stream_start_position_ptr == NULL )
    {
        BoatLog(BOAT_LOG_CRITICAL, "Unable to dynamically allocate memory to store RLP stream.");
        boat_throw(BOAT_ERROR_OUT_OF_MEMORY, RawtxPerform_cleanup);
    }

    // Allocate memory for RLP stream HEX string
    // It's a storage for HEX string converted from RLP stream binary. The
    // HEX string is used as input for web3. It's in a form of "0x1234ABCD".
    // Where *2 for binary to HEX conversion, +2 for "0x" prefix, + 1 for null terminator.
    rlp_stream_hex_str = BoatMalloc(rlp_stream_size_estimate * 2 + 2 + 1);

    if( rlp_stream_hex_str == NULL )
    {
        BoatLog(BOAT_LOG_CRITICAL, "Unable to dynamically allocate memory to store RLP HEX string.");
        boat_throw(BOAT_ERROR_OUT_OF_MEMORY, RawtxPerform_cleanup);
    }
    

    /**************************************************************************
    * STEP 1: Construction RAW transaction without real v/r/s                 *
    *         (See above description for details)                             *
    **************************************************************************/
    
    // Reserve first 9 bytes for outer LIST's RLP header
    rlp_stream_current_position_ptr = rlp_stream_start_position_ptr + RLP_STREAM_RESERVE_HEADER;

    // Encode nonce
    rlp_stream_current_position_ptr = RlpFieldEncode( rlp_stream_current_position_ptr,
                                              tx_info_ctx_ptr->rawtx_fields.nonce.field,
                                              tx_info_ctx_ptr->rawtx_fields.nonce.field_len,
                                              RLP_FIELD_TYPE_STRING,
                                              BOAT_FALSE);
    if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);

    // Encode gasprice
    rlp_stream_current_position_ptr = RlpFieldEncode( rlp_stream_current_position_ptr,
                                              tx_info_ctx_ptr->rawtx_fields.gasprice.field,
                                              tx_info_ctx_ptr->rawtx_fields.gasprice.field_len,
                                              RLP_FIELD_TYPE_STRING,
                                              BOAT_FALSE);
    if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);
    
    // Encode gaslimit
    rlp_stream_current_position_ptr = RlpFieldEncode( rlp_stream_current_position_ptr,
                                              tx_info_ctx_ptr->rawtx_fields.gaslimit.field,
                                              tx_info_ctx_ptr->rawtx_fields.gaslimit.field_len,
                                              RLP_FIELD_TYPE_STRING,
                                              BOAT_FALSE);
    if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);
    
    // Encode recipient
    rlp_stream_current_position_ptr = RlpFieldEncode( rlp_stream_current_position_ptr,
                                              tx_info_ctx_ptr->rawtx_fields.recipient,
                                              20,
                                              RLP_FIELD_TYPE_STRING,
                                              BOAT_FALSE);
    if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);

    // Encode value
    rlp_stream_current_position_ptr = RlpFieldEncode( rlp_stream_current_position_ptr,
                                              tx_info_ctx_ptr->rawtx_fields.value.field,
                                              tx_info_ctx_ptr->rawtx_fields.value.field_len,
                                              RLP_FIELD_TYPE_STRING,
                                              BOAT_FALSE);
    if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);

    // Encode data
    rlp_stream_current_position_ptr = RlpFieldEncode( rlp_stream_current_position_ptr,
                                              tx_info_ctx_ptr->rawtx_fields.data.field_ptr,
                                              tx_info_ctx_ptr->rawtx_fields.data.field_len,
                                              RLP_FIELD_TYPE_STRING,
                                              BOAT_FALSE);
    if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);


    // Record the position of "v" for use in Step 4
    rlp_stream_v_position_ptr = rlp_stream_current_position_ptr;


    // If EIP-155 is required, encode v = chain id, r = s = NULL in this step
    if( boat_wallet_info_ptr->network_info.eip155_compatibility == BOAT_TRUE )
    {
        // v = Chain ID
        // Currently max chain id supported is (2^32 - 1 - 36)/2, because v is
        // finally calculated as chain_id * 2 + 35 or 36 as per EIP-155.
        v = boat_wallet_info_ptr->network_info.chain_id;
        chain_id_len = UtilityUint32ToBigend(tx_info_ctx_ptr->rawtx_fields.v.field,
                                             v,
                                             TRIMBIN_LEFTTRIM
                                            );
        tx_info_ctx_ptr->rawtx_fields.v.field_len = chain_id_len;
        
        // r = s = NULL
        tx_info_ctx_ptr->rawtx_fields.sig.r_len = 0;
        tx_info_ctx_ptr->rawtx_fields.sig.s_len = 0;


        // Encode v
        rlp_stream_current_position_ptr = RlpFieldEncode( rlp_stream_current_position_ptr,
                                                  tx_info_ctx_ptr->rawtx_fields.v.field,
                                                  tx_info_ctx_ptr->rawtx_fields.v.field_len,
                                                  RLP_FIELD_TYPE_STRING,
                                                  BOAT_FALSE);
        if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);

        // Encode r
        rlp_stream_current_position_ptr = RlpFieldEncode( rlp_stream_current_position_ptr,
                                                  tx_info_ctx_ptr->rawtx_fields.sig.r32B,
                                                  tx_info_ctx_ptr->rawtx_fields.sig.r_len,
                                                  RLP_FIELD_TYPE_STRING,
                                                  BOAT_FALSE);
        if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);

        // Encode s
        rlp_stream_current_position_ptr = RlpFieldEncode( rlp_stream_current_position_ptr,
                                                  tx_info_ctx_ptr->rawtx_fields.sig.s32B,
                                                  tx_info_ctx_ptr->rawtx_fields.sig.s_len,
                                                  RLP_FIELD_TYPE_STRING,
                                                  BOAT_FALSE);
        if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);

    }

    // Encode LIST header
    message_len = (UINT32)(rlp_stream_current_position_ptr - (rlp_stream_start_position_ptr + 9));
    rlp_stream_current_position_ptr = RlpFieldEncode( NULL,
                                                      rlp_stream_start_position_ptr + RLP_STREAM_RESERVE_HEADER,
                                                      message_len,
                                                      RLP_FIELD_TYPE_LIST,
                                                      BOAT_TRUE);
    if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);

    message_len += (UINT32)(rlp_stream_start_position_ptr + RLP_STREAM_RESERVE_HEADER - rlp_stream_current_position_ptr);



    /**************************************************************************
    * STEP 2: Calculate SHA3 hash of message                                  *
    **************************************************************************/

    // Hash the message
    keccak_256(rlp_stream_current_position_ptr, message_len, message_digest);



    /**************************************************************************
    * STEP 3: Sign the transaction                                            *
    **************************************************************************/

    // Sign the transaction
    ecdsa_sign_digest(
                       &secp256k1, // const ecdsa_curve *curve
                       boat_wallet_info_ptr->account_info.priv_key_array, //const uint8_t *priv_key
                       message_digest, //const uint8_t *digest
                       tx_info_ctx_ptr->rawtx_fields.sig.sig64B, //uint8_t *sig,
                       &sig_parity, //uint8_t *pby,
                       NULL  //int (*is_canonical)(uint8_t by, uint8_t sig[64]))
                       );


    // Trim r
    UINT8 trimed_r[32];
    
    tx_info_ctx_ptr->rawtx_fields.sig.r_len =
         UtilityTrimBin(
                trimed_r,
                tx_info_ctx_ptr->rawtx_fields.sig.r32B,
                32,
                TRIMBIN_LEFTTRIM,
                BOAT_TRUE
                );

    memcpy(tx_info_ctx_ptr->rawtx_fields.sig.r32B,
           trimed_r,
           tx_info_ctx_ptr->rawtx_fields.sig.r_len);

    // Trim s
    UINT8 trimed_s[32];
    
    tx_info_ctx_ptr->rawtx_fields.sig.s_len =
         UtilityTrimBin(
                trimed_s,
                tx_info_ctx_ptr->rawtx_fields.sig.s32B,
                32,
                TRIMBIN_LEFTTRIM,
                BOAT_TRUE
                );

    memcpy(tx_info_ctx_ptr->rawtx_fields.sig.s32B,
           trimed_s,
           tx_info_ctx_ptr->rawtx_fields.sig.s_len);
    
    /**************************************************************************
    * STEP 4: Encode full RAW transaction with updated v/r/s                  *
    *         (See above description for details)                             *
    **************************************************************************/

    // Re-encode v
    if( boat_wallet_info_ptr->network_info.eip155_compatibility == BOAT_TRUE )
    {
        // v = Chain ID * 2 + parity + 35
        v = boat_wallet_info_ptr->network_info.chain_id * 2 + sig_parity + 35;
    }
    else
    {
        // v = parity + 27
        v = sig_parity + 27;
    }
        
    chain_id_len = UtilityUint32ToBigend(tx_info_ctx_ptr->rawtx_fields.v.field,
                                         v,
                                         TRIMBIN_LEFTTRIM
                                        );
    tx_info_ctx_ptr->rawtx_fields.v.field_len = chain_id_len;

    // Recall the recorded position of v to replace it with the updated one
    rlp_stream_current_position_ptr = RlpFieldEncode( rlp_stream_v_position_ptr,
                                              tx_info_ctx_ptr->rawtx_fields.v.field,
                                              tx_info_ctx_ptr->rawtx_fields.v.field_len,
                                              RLP_FIELD_TYPE_STRING,
                                              BOAT_FALSE);
    if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);


    // Re-encode r
    rlp_stream_current_position_ptr = RlpFieldEncode( rlp_stream_current_position_ptr,
                                              tx_info_ctx_ptr->rawtx_fields.sig.r32B,
                                              tx_info_ctx_ptr->rawtx_fields.sig.r_len,
                                              RLP_FIELD_TYPE_STRING,
                                              BOAT_FALSE);
    if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);

    // Re-encode s
    rlp_stream_current_position_ptr = RlpFieldEncode( rlp_stream_current_position_ptr,
                                              tx_info_ctx_ptr->rawtx_fields.sig.s32B,
                                              tx_info_ctx_ptr->rawtx_fields.sig.s_len,
                                              RLP_FIELD_TYPE_STRING,
                                              BOAT_FALSE);
    if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);


    // Re-encode LIST header
    message_len = (UINT32)(rlp_stream_current_position_ptr - (rlp_stream_start_position_ptr + 9));
    rlp_stream_current_position_ptr = RlpFieldEncode( NULL,
                                                      rlp_stream_start_position_ptr + RLP_STREAM_RESERVE_HEADER,
                                                      message_len,
                                                      RLP_FIELD_TYPE_LIST,
                                                      BOAT_TRUE);
    if( rlp_stream_current_position_ptr == NULL )  boat_throw(BOAT_ERROR_RLP_ENCODING_FAIL, RawtxPerform_cleanup);

    message_len += (UINT32)(rlp_stream_start_position_ptr + RLP_STREAM_RESERVE_HEADER - rlp_stream_current_position_ptr);



    // Print transaction recipient to log

    if( 0 == UtilityBin2Hex(
        rlp_stream_hex_str,
        tx_info_ctx_ptr->rawtx_fields.recipient,
        20,
        BIN2HEX_LEFTTRIM_UFMTDATA,
        BIN2HEX_PREFIX_0x_YES,
        BOAT_FALSE
        ))
    {
        strcpy(rlp_stream_hex_str, "NULL");
    }

    BoatLog(BOAT_LOG_NORMAL, "Transaction to: %s", rlp_stream_hex_str);

#ifdef DEBUG_LOG

    // To save memory, re-use rlp_stream_hex_str to print debug information
    // As rlp_stream_hex_str's size is enough to hold the entire RLP hex string,
    // thus it's far more than enough to hold any field of the RLP hex string,
    // which is printed.

    printf("Transaction Message:\n");

    // Print nonce
    if(0 == UtilityBin2Hex(
        rlp_stream_hex_str,
        tx_info_ctx_ptr->rawtx_fields.nonce.field,
        tx_info_ctx_ptr->rawtx_fields.nonce.field_len,
        BIN2HEX_LEFTTRIM_QUANTITY,
        BIN2HEX_PREFIX_0x_YES,
        BOAT_FALSE
        ))
    {
        strcpy(rlp_stream_hex_str, "NULL");
    }

    printf("Nonce: %s\n", rlp_stream_hex_str);
            

    // Print Sender

    if( 0 == UtilityBin2Hex(
        rlp_stream_hex_str,
        boat_wallet_info_ptr->account_info.address,
        20,
        BIN2HEX_LEFTTRIM_UFMTDATA,
        BIN2HEX_PREFIX_0x_YES,
        BOAT_FALSE
        ))
    {
        strcpy(rlp_stream_hex_str, "NULL");
    }

    printf("Sender: %s\n", rlp_stream_hex_str);

    
    // Print recipient

    if( 0 == UtilityBin2Hex(
        rlp_stream_hex_str,
        tx_info_ctx_ptr->rawtx_fields.recipient,
        20,
        BIN2HEX_LEFTTRIM_UFMTDATA,
        BIN2HEX_PREFIX_0x_YES,
        BOAT_FALSE
        ))
    {
        strcpy(rlp_stream_hex_str, "NULL");
    }

    printf("Recipient: %s\n", rlp_stream_hex_str);


    // Print value

    if( 0 == UtilityBin2Hex(
        rlp_stream_hex_str,
        tx_info_ctx_ptr->rawtx_fields.value.field,
        tx_info_ctx_ptr->rawtx_fields.value.field_len,
        BIN2HEX_LEFTTRIM_QUANTITY,
        BIN2HEX_PREFIX_0x_YES,
        BOAT_FALSE
        ))
    {
        strcpy(rlp_stream_hex_str, "NULL");
    }
        
    printf("Value: %s\n", rlp_stream_hex_str);

   
    // Print data

    if( 0 == UtilityBin2Hex(
        rlp_stream_hex_str,
        tx_info_ctx_ptr->rawtx_fields.data.field_ptr,
        tx_info_ctx_ptr->rawtx_fields.data.field_len,
        BIN2HEX_LEFTTRIM_UFMTDATA,
        BIN2HEX_PREFIX_0x_YES,
        BOAT_FALSE
        ))
    {
        strcpy(rlp_stream_hex_str, "NULL");
    }

    printf("Data: %s\n\n", rlp_stream_hex_str);
        
#endif



    UtilityBin2Hex(
                rlp_stream_hex_str,
                rlp_stream_current_position_ptr,
                message_len,
                BIN2HEX_LEFTTRIM_UFMTDATA,
                BIN2HEX_PREFIX_0x_YES,
                BOAT_FALSE
                );

    param_eth_sendRawTransaction.signedtx_str = rlp_stream_hex_str;
    
    tx_hash_str = web3_eth_sendRawTransaction( boat_wallet_info_ptr->network_info.node_url_ptr,
                                               &param_eth_sendRawTransaction);

    if( tx_hash_str == NULL ) boat_throw(BOAT_ERROR_RPC_FAIL, RawtxPerform_cleanup);

    tx_info_ctx_ptr->tx_hash.field_len =
            UtilityHex2Bin(
                            tx_info_ctx_ptr->tx_hash.field,
                            32,
                            tx_hash_str,
                            TRIMBIN_LEFTTRIM,
                            BOAT_FALSE
                           );

    // NOT GOOD
    strcpy(tx_hash, tx_hash_str);

    tx_mined_timeout = BOAT_WAIT_PENDING_TX_TIMEOUT;
    param_eth_getTransactionReceipt.tx_hash_str = tx_hash;

    do
    {
        sleep(BOAT_MINE_INTERVAL); // Sleep waiting for the block being mined
        
        tx_status_str = web3_eth_getTransactionReceiptStatus(
                                        boat_wallet_info_ptr->network_info.node_url_ptr,
                                        &param_eth_getTransactionReceipt);
        if( tx_status_str == NULL )   boat_throw(BOAT_ERROR_RPC_FAIL, RawtxPerform_cleanup);

        // tx_status_str == "": the transaction is pending
        // tx_status_str == "0x1": the transaction is successfully mined
        // tx_status_str == "0x0": the transaction fails
        if( tx_status_str[0] != '\0' )
        {
            if( strcmp(tx_status_str, "0x1") == 0 )
            {
                BoatLog(BOAT_LOG_NORMAL, "Transaction has got mined.");
            }
            else
            {
                BoatLog(BOAT_LOG_NORMAL, "Transaction fails.");
            }

            break;
        }

        tx_mined_timeout -= BOAT_MINE_INTERVAL;
        
    }while(tx_mined_timeout > 0);

    if( tx_mined_timeout <= 0)
    {
        BoatLog(BOAT_LOG_NORMAL, "Wait for pending transaction timeout. This does not mean the transaction fails.");
    }

    // Clean Up

    // Free RLP stream buffer
    if( rlp_stream_start_position_ptr != NULL )
    {
        BoatFree(rlp_stream_start_position_ptr);
    }

    // Free RLP string buffer
    if( rlp_stream_hex_str != NULL )
    {
        BoatFree(rlp_stream_hex_str);
    }

    result = BOAT_SUCCESS;

    // Exceptional Clean Up
    boat_catch(RawtxPerform_cleanup)
    {
        BoatLog(BOAT_LOG_NORMAL, "Exception: %d", boat_exception);

        // Free RLP stream buffer
        if( rlp_stream_start_position_ptr != NULL )
        {
            BoatFree(rlp_stream_start_position_ptr);
        }

        // Free RLP string buffer
        if( rlp_stream_hex_str != NULL )
        {
            BoatFree(rlp_stream_hex_str);
        }

        result = boat_exception;
    }
   
    return result;

}




