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

/*!@brief Boatwallet SDK entry

@file
boatwallet.c is the SDK main entry.
*/

#include "wallet/boatwallet.h"
#include "randgenerator.h"
#include "bignum.h"
#include "cJSON.h"

#if BOAT_USE_OPENSSL != 0
#include <openssl/evp.h>
#include <openssl/aes.h>
#endif

BoatWalletInfo g_boat_wallet_info;
TxInfo g_tx_info;


/*!*****************************************************************************
@brief Initialize BoatWallet

Function: BoatWalletInit()

    This function initialize context of BoatWallet.

    BoatWalletInit() MUST be called before any use of BoatWallet.
    BoatWalletDeInit() MUST be called after use of BoatWallet.
    
    NOTE: BoatWalelt is NOT thread-safe.

@see BoatWalletDeInit()

@return
    This function returns BOAT_SUCCESS if initialization is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param This function doesn't take any argument.
*******************************************************************************/
BOAT_RESULT BoatWalletInit(void)
{
    // Init Web3 interface
    web3_init();

    // Init RPC
    RpcInit();

    
    // Set EIP-155 Compatibility to TRUE by default
    BoatWalletSetEIP155Comp(BOAT_TRUE);

    // Allocate memory for node url string
    g_boat_wallet_info.network_info.node_url_ptr = NULL;
    return BOAT_SUCCESS;
}


/*!*****************************************************************************
@brief De-initialize BoatWallet

Function: BoatWalletDeInit()

    This function de-initialize context of BoatWallet.

    BoatWalletInit() MUST be called before any use of BoatWallet.
    BoatWalletDeInit() MUST be called after use of BoatWallet.
    
    NOTE: BoatWalelt is NOT thread-safe.

@see BoatWalletInit()

@return This function doesn't return any thing.

@param This function doesn't take any argument.
*******************************************************************************/
void BoatWalletDeInit(void)
{
    // De-init RPC
    RpcDeinit();

    // Destroy private key in wallet memory
    memset(g_boat_wallet_info.account_info.priv_key_array, 0x00, 32);

    if( g_boat_wallet_info.network_info.node_url_ptr != NULL )
    {
        BoatFree(g_boat_wallet_info.network_info.node_url_ptr);
    }

    
    return;
}


/*!*****************************************************************************
@brief Set BoatWallet: URL of blockchain node

Function: BoatWalletSetNodeUrl()

    This function sets the URL of the blockchain node to connect to.

    A URL is composed of protocol, IP address/name and port, in a form:
    http://a.b.com:8545

@return
    This function returns BOAT_SUCCESS if setting is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] node_url_ptr
    A string indicating the URL of blockchain node to connect to.
        
*******************************************************************************/
BOAT_RESULT BoatWalletSetNodeUrl(const CHAR *node_url_ptr)
{
    BOAT_RESULT result;
    
    // Set Node URL
    if( node_url_ptr != NULL )
    {
        if( g_boat_wallet_info.network_info.node_url_ptr != NULL )
        {
            BoatFree(g_boat_wallet_info.network_info.node_url_ptr);
        }

        // +1 for NULL Terminator
        g_boat_wallet_info.network_info.node_url_ptr = BoatMalloc(strlen(node_url_ptr)+1);
        
        if( g_boat_wallet_info.network_info.node_url_ptr != NULL )
        {
            strcpy(g_boat_wallet_info.network_info.node_url_ptr, node_url_ptr);
            result = BOAT_SUCCESS;
        }
        else
        {
            BoatLog(BOAT_LOG_NORMAL, "Fail to allocate memory for Node URL string.");
            result = BOAT_ERROR;
        }
    }
    else
    {
        BoatLog(BOAT_LOG_NORMAL, "Argument cannot be NULL.");
        result = BOAT_ERROR;
    }

    return result;
}


/*!*****************************************************************************
@brief Set BoatWallet: EIP-155 Compatibility

Function: BoatWalletSetEIP155Comp()

    This function sets if the network supports EIP-155.

    If the network supports EIP-155, set it to BOAT_TRUE.
    Otherwise set it to BOAT_FALSE.

@return
    This function returns BOAT_SUCCESS if setting is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] eip155_compatibility
    BOAT_TRUE if the network supports EIP-155. Otherwise BOAT_FALSE.
        
*******************************************************************************/
BOAT_RESULT BoatWalletSetEIP155Comp(UINT8 eip155_compatibility)
{
    // Set EIP-155 Compatibility
    g_boat_wallet_info.network_info.eip155_compatibility = eip155_compatibility;
    return BOAT_SUCCESS;
}


/*!*****************************************************************************
@brief Set BoatWallet: Chain ID

Function: BoatWalletSetChainId()

    This function sets the chain ID of the network.

    If the network supports EIP-155, chain ID is part of the transaction
    message to sign.

    If the network doesn't support EIP-155, chain ID is ignored.

@return
    This function returns BOAT_SUCCESS if setting is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] chain_id
    Chain ID of the blockchain network to use.
        
*******************************************************************************/
BOAT_RESULT BoatWalletSetChainId(UINT32 chain_id)
{
    // Set Chain ID
    g_boat_wallet_info.network_info.chain_id = chain_id;
    return BOAT_SUCCESS;
}


/*!*****************************************************************************
@brief Set BoatWallet: Private Key

Function: BoatWalletSetPrivkey()

    This function sets the private key of the wallet account.

    A private key is 256 bit. If it's treated as a UINT256 in bigendian, the
    valid private key value for Ethereum is [1, n-1], where n is
    0xFFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE BAAEDCE6 AF48A03B BFD25E8C D0364141.

    This function will call BoatWalletCheckPrivkey() to check the validity of
    the private key.

    The public key is automatically derived from the private key and the
    address of the account is calculated from the public key.

    In case co-sign is used, this function set the half key shard of the
    private key. The public key is calculated by co-sign algorithm with the
    co-sign server.

    NOTE: Be very careful to PROTECT the private key.

@see
    BoatWalletCheckPrivkey()

@return
    This function returns BOAT_SUCCESS if setting is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] priv_key_array
    Private key to use.
        
*******************************************************************************/
BOAT_RESULT BoatWalletSetPrivkey(const UINT8 priv_key_array[32])
{
    UINT8 pub_key[65];
    UINT8 pub_key_digest[32];
    BOAT_RESULT result;

    if( priv_key_array == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Private key cannot be NULL.");
        return BOAT_ERROR;
    }
    
    result = BoatWalletCheckPrivkey(priv_key_array);

    if( result != BOAT_SUCCESS )
    {
        BoatLog(BOAT_LOG_NORMAL, "Private key is not valid.");
        return BOAT_ERROR;
    }
    
    
    // Set private key and calculate public key as well as address
    // PRIVATE KEY MUST BE SET BEFORE SETTING NONCE AND GASPRICE

    memcpy(g_boat_wallet_info.account_info.priv_key_array, priv_key_array, 32);
    
    // Calculate address from private key;
    ecdsa_get_public_key65(
                            &secp256k1,
                            g_boat_wallet_info.account_info.priv_key_array,
                            pub_key);

    // pub_key[] is a 65-byte array with pub_key[0] being 0x04 SECG prefix followed by 64-byte public key
    // Thus skip pub_key[0]
    memcpy(g_boat_wallet_info.account_info.pub_key_array, &pub_key[1], 64);

    keccak_256(&pub_key[1], 64, pub_key_digest);

    memcpy(g_boat_wallet_info.account_info.address, pub_key_digest+12, 20); // Address is the least significant 20 bytes of public key's hash

    return BOAT_SUCCESS;
}


/*!*****************************************************************************
@brief Generate Private Key

Function: BoatWalletGeneratePrivkey()

    This function generates the private key of the wallet account and calls
    BoatWalletSetPrivkey() to save it into wallet account.

    A private key is a 256 bit random number up to a value slightly smaller than
    all bits being 1. See BoatWalletCheckPrivkey() for the details.

    In case co-sign is used, this is the half key shard.

    NOTE: Be very careful to PROTECT the private key.

@see
    BoatWalletSetPrivkey() BoatWalletCheckPrivkey()

@return
    This function returns BOAT_SUCCESS if private key is successfully generated.\n
    Otherwise it returns BOAT_ERROR.

@param[out] priv_key_array
    A 32-byte buffer to hold the generated private key.
        
*******************************************************************************/
BOAT_RESULT BoatWalletGeneratePrivkey(BOAT_OUT UINT8 priv_key_array[32])
{
    UINT32 key_try_count;
    BOAT_RESULT result;

    if( priv_key_array == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Buffer to hold private key cannot be NULL.");
        return BOAT_ERROR;
    }
    
    // Try at most 100 times to find a random number fits for Ethereum private key
    for( key_try_count = 0; key_try_count < 100; key_try_count++ )
    {
    
        result = random_stream(priv_key_array, 32);

        if( result != BOAT_SUCCESS )
        {
            BoatLog(BOAT_LOG_CRITICAL, "Fail to generate private key.");
            break;
        }        

        result = BoatWalletCheckPrivkey(priv_key_array);

        if( result == BOAT_SUCCESS )
        {
            break;
        }            
    }

    return result;
    
}


/*!*****************************************************************************
@brief Check validity of given private key

Function: BoatWalletCheckPrivkey()

    This function checks the validity of the private key.

    A private key is 256 bit. If it's treated as a UINT256 in bigendian, the
    valid private key value for Ethereum is [1, n-1], where n is
    0xFFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE BAAEDCE6 AF48A03B BFD25E8C D0364141


@return
    This function returns BOAT_SUCCESS if the key is valid.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] priv_key_array
    Private key to check.
        
*******************************************************************************/
BOAT_RESULT BoatWalletCheckPrivkey(const UINT8 priv_key_array[32])
{
    bignum256 priv_key_bn256;
    bignum256 priv_key_max_bn256;

    // Valid private key value (as a UINT256) for Ethereum is [1, n-1], where n is
    // 0xFFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE BAAEDCE6 AF48A03B BFD25E8C D0364141
    const UINT32 priv_key_max_u256[8] = {0xD0364141, 0xBFD25E8C, 0xAF48A03B, 0xBAAEDCE6,
                                         0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};

    if( priv_key_array == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Private key cannot be NULL.");
        return BOAT_ERROR;
    }

    // Check whether private_key_array is in valid range
    
    // Convert private key from UINT256 to Bignum256 format
    bn_read_le(priv_key_array, &priv_key_bn256);

    if( bn_is_zero(&priv_key_bn256) != 0)
    {
        BoatLog(BOAT_LOG_NORMAL, "Private key cannot be all zeros. Fail to set private key.");
        return BOAT_ERROR;
    }

    // Convert priv_key_max_u256 from UINT256 to Bignum256 format
    bn_read_le((const uint8_t *)priv_key_max_u256, &priv_key_max_bn256);

    // Check whether priv_key_bn256 is less than priv_key_max_bn256
    if( bn_is_less(&priv_key_bn256, &priv_key_max_bn256) == 0 )
    {
        BoatLog(BOAT_LOG_NORMAL, "Private key does not conform to Ethereum. Fail to set private key.");
        return BOAT_ERROR;
    }

    return BOAT_SUCCESS;

}


/*!*****************************************************************************
@brief Get Balance of the wallet account

Function: BoatWalletGetBalance()

    This function gets the balance of the wallet account from network.

    If the account is not avaialable (i.e. the address of the wallet account
    never appears as a recipient in any successful non-zero-value transaction),
    the balance returned is 0. It's not possible to distinguish an unavailable
    account from a zero-balance account.


@return
    This function returns a HEX string representing the balance (Unit: wei,\n
    i.e. 1e-18 ETH) of the account.\n
    If any error occurs, it returns NULL.
    

@param This function doesn't take any argument.
        
*******************************************************************************/
CHAR * BoatWalletGetBalance(void)
{
    CHAR account_address_str[43];
    Param_eth_getBalance param_eth_getBalance;
    CHAR *tx_balance_str;


    // PRIVATE KEY MUST BE SET BEFORE SETTING NONCE, BECAUSE GETTING BALANCE FROM
    // NETWORK NEEDS ETHEREUM ADDRESS, WHICH IS COMPUTED FROM KEY


    // Get balance from network
    // Return value of web3_eth_getBalance() is balance in wei
    
    UtilityBin2Hex(
        account_address_str,
        g_boat_wallet_info.account_info.address,
        20,
        BIN2HEX_LEFTTRIM_UFMTDATA,
        BIN2HEX_PREFIX_0x_YES,
        BOAT_FALSE
        );
        
    param_eth_getBalance.address_str = account_address_str;
    param_eth_getBalance.block_num_str = "latest";


    
    tx_balance_str = web3_eth_getBalance(
                                    g_boat_wallet_info.network_info.node_url_ptr,
                                    &param_eth_getBalance);

    if( tx_balance_str == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to get balance from network.");
        return NULL;
    }

    return tx_balance_str;
}



#define KEYSTORE_SIZE_EXCLUDE_URL \
  ( sizeof(g_boat_wallet_info.account_info.priv_key_array)\
  + sizeof(g_boat_wallet_info.account_info.pub_key_array)\
  + sizeof(g_boat_wallet_info.account_info.address)\
  + sizeof(g_boat_wallet_info.network_info.chain_id)\
  + sizeof(g_boat_wallet_info.network_info.eip155_compatibility)\
  + sizeof(UINT32) )


/*!*****************************************************************************
@brief Save specified wallet information to keystore file with AES encryption

Function: BoatWalletSaveWalletEx()

    This function saves the specified wallet account into a keystore file with
    password protected. The wallet account must be a structure of type BoatWalletInfo.

    BoatWalletSaveWallet() is a derived version of this function that specifies
    g_boat_wallet_info as the wallet account. g_boat_wallet_info is the wallet
    account internally used by BoatWallet.

    BoatWalletSaveWalletEx() is typically used to create a keystore file of a
    given wallet account other than g_boat_wallet_info.

    The fields in the wallet account are saved. Especially the node url pointer
    field is extracted as the node url string.

    The keystore file is in following format:

    ---------------------------------
    |  IH  | IL | D |     I     | P |
    ---------------------------------
    |<--Plane-->|<--- Encrypted --->|

    IH: 32 bytes keccak_256 hash of "I"
    IL: Length of "I" in byte
    D:  16 bytes dummy block for IV-independent decryption
    I:  Wallet Information
    P:  Padding for AES Block alignment, 0~15 bytes

    I field consists of following sub-fields in sequence:

    1. 32 bytes private key
    2. 64 bytes public key
    3. 20 bytes address
    4. 4 bytes chain ID, in BigEndian
    5. 1 byte EIP-155 compatibility indicator
    6. 4 bytes length of Node URL (no null-terminator following), in BigEndian
    7. Node URL string (without null-terminator)
    
    The D, I and P parts are encrypted by AES256-CBC before being saved to
    keystore file with a user specified password. Actual AES key is the keccak_256
    hash of the user specified password, and thus no matter how long the password
    is, the key is always 256 bit.

    AES is a block cipher algorithm with a block size of 16 bytes. To encrypt
    plain text of any size, some block cipher mode of operation is performed.
    AES-CBC is one of the most popular modes. It XORs every plain text block
    with the previous encrypted text block and encrypts the XORed block. For
    the first plain text block, an extra 16-byte Initial Vector (IV) is XORed.

    To decrypt an encrypted block, first decrypt the block with the same AES
    key and then XOR the decrypted text with the encrypted text of the previous
    block. This recovers the plain text block.
    
    To decrypt the first encrypted block (whose corresponding plain text block
    was XORed with IV before being encrypted), the same IV as the one at
    encryption time must be specified.

    Note that the IV only affects the decryption of the first encrypted block.
    All blocks following the first can be decrypted correctly even if IV is not
    the same as the one at encryption time.

    Thus by simply prefixing a 16-byte dummy block to the beginning of the plain
    text, which plays the role of "first block", the decryption side could
    simply omit the first block and doesn't have to know the IV at encryption
    time. That's what field D does.

    Field P is a padding of 0 to 15 bytes to meet the alignment requirement of
    AES-CBC. AES-CBC could only process the text in a manner of 16-byte blocks.
    Thus length of I + P MUST be multiple of 16 bytes.
    
    See the URL for details about block cipher mode of operation:
    https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation

    Note that if OpenSSL is used as the cipher implementation, its default
    padding rule MUST be disabled. Instead, add correct size of padding
    mannually.

    OpenSSL uses PKCS padding rule for AES by default. The rule adds 1-to-16-
    byte padding with every byte's value equal to how many bytes the padding is.
    For example, if 6 bytes are added as padding, the value of every byte is
    0x06. Especally if no padding is required, this rule always adds a 16-byte
    padding.

    When decrypting, OpenSSL will check the padding value against PKCS rule as
    a simple validition and automatically removes PKCS padding by default.
    However PKCS rule can't strictly tell whether a byte is a padding byte or
    a user data byte of the same value as a padding byte. For example, the final
    decrypted block ended with 0x03 0x03 0x03 0x03 may either be treated as a
    correct block with 3-byte padding, or an incorrect block because 4 bytes of
    0x04 are expected regarding PKCS padding rule.

    Another concern not using default PKCS padding rule is for cross-platform
    compatibility. It's possible to export the keystore from a linux based
    device to an RTOS based device which can't run OpenSSL.

    Thus OpenSSL should be configured with automatic padding disabled.
    
    This function will call BoatWalletCheckPrivkey() to check the validity of
    the private key.


@see
    BoatWalletSaveWallet() BoatWalletLoadWalletEX() BoatWalletLoadWallet() BoatWalletCheckPrivkey()

@return
    This function returns BOAT_SUCCESS if the keystore file is saved successfully.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] wallet_info_ptr
    Pointer to the wallet account structure to save to keystore file.

@param[in] passwd_ptr
    Password to encrypt the wallet info.

@param[in] passwd_len
    Length of <passwd_ptr> in byte.\n
    Note that if <passwd_ptr> is a string, either <passwd_len> includes\n
    null-terminator or not is OK. But it must be the same as BoatWalletLoadWalletEx().

@param[in] file_path_str
    Keystore file's path.

*******************************************************************************/
BOAT_RESULT BoatWalletSaveWalletEx(const BoatWalletInfo *wallet_info_ptr, const UINT8 *passwd_ptr, UINT32 passwd_len, const CHAR *file_path_str)
{
    UINT8 aes256key[32];
    UINT8 iv[16];  // Initial Vector is used for AES-CBC encryption and not for AES-ECB

    UINT8 *plain_wallet_info_array = NULL;
    UINT8 *encrypted_wallet_info_array = NULL;

    UINT8 wallet_info_hash_array[32];

#if BOAT_USE_OPENSSL != 0
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX ctx;
    EVP_CIPHER_CTX *ctx_ptr = &ctx;
#else
    EVP_CIPHER_CTX *ctx_ptr = NULL;
#endif

    int openssl_ret;
#endif

    UINT32 node_url_str_len;
    UINT32 node_url_str_len_big;
    int openssl_len = 0;
    UINT32 plain_wallet_info_len = 0;
    UINT32 plain_wallet_info_len_big;
    UINT32 encrypted_total_len = 0;
    UINT32 chain_id_big;

    size_t written_len;


    FILE *key_store_file_ptr = NULL;

    BOAT_RESULT result = BOAT_SUCCESS;
    boat_try_declare;

    if( wallet_info_ptr == NULL || passwd_ptr == NULL || file_path_str == NULL || passwd_len == 0 )
    {
        BoatLog(BOAT_LOG_NORMAL, "Arguments cannot be NULL.");
        return BOAT_ERROR;
    }

    if( wallet_info_ptr->network_info.node_url_ptr == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Node URL cannot be NULL.");
        return BOAT_ERROR;
    }

    node_url_str_len = strlen(wallet_info_ptr->network_info.node_url_ptr);
    

    // doesn't include node url's NULL Terminator
    plain_wallet_info_array = BoatMalloc(  AES_BLOCK_SIZE
                                        + ROUNDUP(KEYSTORE_SIZE_EXCLUDE_URL + node_url_str_len, AES_BLOCK_SIZE));

    // encrypted node url doesn't include the NULL terminator
    encrypted_wallet_info_array = BoatMalloc(  AES_BLOCK_SIZE
                                          + ROUNDUP(KEYSTORE_SIZE_EXCLUDE_URL + node_url_str_len, AES_BLOCK_SIZE));

    if( plain_wallet_info_array == NULL || encrypted_wallet_info_array == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to allocate memory.");
        boat_throw(BOAT_ERROR, BoatWalletSaveWallet_cleanup);
    }
    
    result = BoatWalletCheckPrivkey(wallet_info_ptr->account_info.priv_key_array);

    if( result != BOAT_SUCCESS )
    {
        BoatLog(BOAT_LOG_NORMAL, "Private key is not valid.");
        boat_throw(BOAT_ERROR, BoatWalletSaveWallet_cleanup);
    }

    key_store_file_ptr = fopen(file_path_str, "wb");

    if( key_store_file_ptr == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Unable to save keystore file: %s.", file_path_str);
        boat_throw(BOAT_ERROR, BoatWalletSaveWallet_cleanup);
    }

    // Use random number for the initial vector
    result = random_stream(iv, 16);


    // Hash the password to generate the AES key
    keccak_256(passwd_ptr, passwd_len, aes256key);


    // Initialize OpenSSL EVP context for cipher/decipher
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX_init(ctx_ptr);
#else
    ctx_ptr = EVP_CIPHER_CTX_new();
#endif

    // Specify AES256-CBC Algorithm, AES key and initial vector of CBC
    openssl_ret = EVP_EncryptInit_ex(ctx_ptr, EVP_aes_256_cbc(), NULL, aes256key, iv);
    if( openssl_ret != 1 )
    {
        BoatLog(BOAT_LOG_NORMAL, "EVP_EncryptInit_ex failed.");
        boat_throw(BOAT_ERROR, BoatWalletSaveWallet_cleanup);
    }

    // Disable default padding, because default PKCS padding cannot be distinguished
    // from effective data in some extreme case.
    EVP_CIPHER_CTX_set_padding(ctx_ptr, 0);

    // Encryption
    // Reserve beginning AES_BLOCK_SIZE bytes for IV-independent decryption

    plain_wallet_info_len = AES_BLOCK_SIZE;

    memcpy(plain_wallet_info_array + plain_wallet_info_len, &wallet_info_ptr->account_info.priv_key_array, sizeof(wallet_info_ptr->account_info.priv_key_array));
    plain_wallet_info_len += sizeof(wallet_info_ptr->account_info.priv_key_array);

    memcpy(plain_wallet_info_array + plain_wallet_info_len, &wallet_info_ptr->account_info.pub_key_array, sizeof(wallet_info_ptr->account_info.pub_key_array));
    plain_wallet_info_len += sizeof(wallet_info_ptr->account_info.pub_key_array);
    
    memcpy(plain_wallet_info_array + plain_wallet_info_len, &wallet_info_ptr->account_info.address, sizeof(wallet_info_ptr->account_info.address));
    plain_wallet_info_len += sizeof(wallet_info_ptr->account_info.address);

    chain_id_big = Utilityhtonl(wallet_info_ptr->network_info.chain_id);
    memcpy(plain_wallet_info_array + plain_wallet_info_len, &chain_id_big, sizeof(UINT32));
    plain_wallet_info_len += sizeof(UINT32);

    memcpy(plain_wallet_info_array + plain_wallet_info_len, &wallet_info_ptr->network_info.eip155_compatibility, sizeof(wallet_info_ptr->network_info.eip155_compatibility));
    plain_wallet_info_len += sizeof(wallet_info_ptr->network_info.eip155_compatibility);

    node_url_str_len_big = Utilityhtonl(node_url_str_len);
    memcpy(plain_wallet_info_array + plain_wallet_info_len, &node_url_str_len_big, sizeof(UINT32));
    plain_wallet_info_len += sizeof(UINT32);

    memcpy(plain_wallet_info_array + plain_wallet_info_len, wallet_info_ptr->network_info.node_url_ptr, node_url_str_len);
    plain_wallet_info_len += node_url_str_len;


    // Encrypt wallet info
    
    encrypted_total_len = 0;
    
    openssl_ret = EVP_EncryptUpdate(ctx_ptr, encrypted_wallet_info_array + encrypted_total_len, &openssl_len, plain_wallet_info_array, ((plain_wallet_info_len - 1)/AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE);
    if( openssl_ret != 1 )
    {
        BoatLog(BOAT_LOG_NORMAL, "EVP_EncryptUpdate failed.");
        boat_throw(BOAT_ERROR, BoatWalletSaveWallet_cleanup);
    }
    encrypted_total_len += openssl_len;

    
    // Finalize encryption
    openssl_ret = EVP_EncryptFinal_ex(ctx_ptr, encrypted_wallet_info_array + encrypted_total_len, &openssl_len);
    if( openssl_ret != 1 )
    {
        BoatLog(BOAT_LOG_NORMAL, "EVP_EncryptFinal_ex failed.");
        boat_throw(BOAT_ERROR, BoatWalletSaveWallet_cleanup);
    }
    encrypted_total_len += openssl_len;



    // Write AES encrypted Wallet Info to keystore file


    // Write wallet info hash
    keccak_256(plain_wallet_info_array + AES_BLOCK_SIZE, plain_wallet_info_len - AES_BLOCK_SIZE, wallet_info_hash_array);
    written_len = fwrite(wallet_info_hash_array, 1, sizeof(wallet_info_hash_array), key_store_file_ptr);
    if( written_len != sizeof(wallet_info_hash_array) )
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to write to keystore file.");
        boat_throw(BOAT_ERROR, BoatWalletSaveWallet_cleanup);
    }

    // Write plain wallet info length (excluding padding) in big endian
    plain_wallet_info_len_big = Utilityhtonl(plain_wallet_info_len);
    written_len = fwrite(&plain_wallet_info_len_big, 1, sizeof(UINT32), key_store_file_ptr);
    if( written_len != sizeof(UINT32) )
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to write to keystore file.");
        boat_throw(BOAT_ERROR, BoatWalletSaveWallet_cleanup);
    }

    // Write Encrypted Wallet Info
    written_len = fwrite(encrypted_wallet_info_array, 1, encrypted_total_len, key_store_file_ptr);
    if( written_len != encrypted_total_len )
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to write to keystore file.");
        boat_throw(BOAT_ERROR, BoatWalletSaveWallet_cleanup);
    }
    

    boat_catch(BoatWalletSaveWallet_cleanup)
    {
        result = boat_exception;
    }

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX_cleanup(&ctx);
#else
    if( ctx_ptr != NULL )
    {
        EVP_CIPHER_CTX_free(ctx_ptr);
    }
#endif

    // Destroy sensitive information
    memset(aes256key, 0, sizeof(aes256key));
    memset(encrypted_wallet_info_array, 0, encrypted_total_len);
    memset(plain_wallet_info_array, 0, plain_wallet_info_len);
    memset(wallet_info_hash_array, 0, sizeof(wallet_info_hash_array));

    if( key_store_file_ptr != NULL )
    {
        fclose(key_store_file_ptr);
    }
    

    if( encrypted_wallet_info_array != NULL )
    {
        BoatFree(encrypted_wallet_info_array);
    }
    
    if( plain_wallet_info_array != NULL )
    {
        BoatFree(plain_wallet_info_array);
    }


    return result;
}


/*!*****************************************************************************
@brief Save wallet information to keystore file with AES encryption

Function: BoatWalletSaveWallet()

    This function saves the wallet account of g_boat_wallet_info into a keystore
    file with password protected.

    This function is a derived version of BoatWalletSaveWalletEx()



@see
    BoatWalletSaveWalletEx() BoatWalletLoadWalletEx() BoatWalletLoadWallet() BoatWalletCheckPrivkey()

@return
    This function returns BOAT_SUCCESS if the keystore file is saved successfully.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] passwd_ptr
    Password to encrypt the wallet info.

@param[in] passwd_len
    Length of <passwd_ptr> in byte.\n
    Note that if <passwd_ptr> is a string, either <passwd_len> includes\n
    null-terminator or not is OK. But it must be the same as BoatWalletLoadWallet().

@param[in] file_path_str
    Keystore file's path.

*******************************************************************************/
BOAT_RESULT BoatWalletSaveWallet(const UINT8 *passwd_ptr, UINT32 passwd_len, const CHAR *file_path_str)
{
    return(BoatWalletSaveWalletEx(&g_boat_wallet_info, passwd_ptr, passwd_len, file_path_str));
}


/*!*****************************************************************************
@brief Load wallet information from keystore file with AES encryption to the specified wallet account

Function: BoatWalletLoadWalletEx()

    This function loads wallet information from specified keystore file to the
    specified wallet account. The wallet account must be a structure of type
    BoatWalletInfo.

    BoatWalletLoadWallet() is a derived version of this function that specifies
    g_boat_wallet_info as the wallet account. g_boat_wallet_info is the wallet
    account internally used by BoatWallet.

    BoatWalletLoadWalletEx() is typically used to load wallet information from
    a keystore file without affecting g_boat_wallet_info.

    The keystore file is protected with an AES password. See BoatWalletSaveWalletEx()
    for its format.

    This function will call BoatWalletCheckPrivkey() to check the validity of
    the private key saved in keystore file.


@see
    BoatWalletLoadWalletEx BoatWalletSaveWalletEx() BoatWalletSaveWallet() BoatWalletCheckPrivkey()

@return
    This function returns BOAT_SUCCESS if loading keystore is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] wallet_info_ptr
    Pointer to the wallet account structure to load wallet information.

@param[in] passwd_ptr
    Password to decrypt the keystore.

@param[in] passwd_len
    Length of <passwd_ptr> in byte.\n
    Note that if <passwd_ptr> is a string, either <passwd_len> includes\n
    null-terminator or not is OK. But it must be the same as BoatWalletSaveWallet().

@param[in] file_path_str
    Keystore file's path.

*******************************************************************************/
BOAT_RESULT BoatWalletLoadWalletEx(BoatWalletInfo *wallet_info_ptr, const UINT8 *passwd_ptr, UINT32 passwd_len, const CHAR *file_path_str)
{
    UINT8 aes256key[32];
    UINT8 iv[16];  // Exact value of Initial Vector is not care for IV-independent decryption

    UINT8 *plain_wallet_info_array = NULL;
    UINT8 *encrypted_wallet_info_array = NULL;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX ctx;
    EVP_CIPHER_CTX *ctx_ptr = &ctx;
#else
    EVP_CIPHER_CTX *ctx_ptr = NULL;
#endif

    int openssl_ret;

    int openssl_len;
    UINT32 plain_wallet_info_len = 0;
    UINT32 plain_wallet_info_len_no_pad;
    UINT32 plain_wallet_info_len_no_pad_big;
    UINT32 plain_wallet_info_field_index;
    UINT32 encrypted_total_len = 0;
    UINT32 node_url_str_len;
    UINT32 node_url_str_len_big;
    UINT32 chain_id_big;
    size_t read_len;


    FILE *key_store_file_ptr = NULL;
    UINT8 wallet_info_hash_array[32];
    UINT8 stored_wallet_info_hash_array[32];

    BOAT_RESULT result = BOAT_SUCCESS;
    boat_try_declare;

    if( passwd_ptr == NULL || file_path_str == NULL  || passwd_len == 0)
    {
        BoatLog(BOAT_LOG_NORMAL, "Arguments cannot be NULL.");
        return BOAT_ERROR;
    }
    

    key_store_file_ptr = fopen(file_path_str, "rb");

    if( key_store_file_ptr == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Unable to open keystore file: %s.", file_path_str);
        return BOAT_ERROR;
    }

    // Read encrypted wallet info from keystore file

    // Read wallet info hash
    read_len = fread(stored_wallet_info_hash_array, 1, sizeof(stored_wallet_info_hash_array), key_store_file_ptr);
    if( read_len != sizeof(stored_wallet_info_hash_array) )
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to read from keystore file.");
        boat_throw(BOAT_ERROR, BoatWalletLoadWallet_cleanup);
    }

    
    // Read effective wallet info length in big endian
    read_len = fread(&plain_wallet_info_len_no_pad_big, 1, sizeof(UINT32), key_store_file_ptr);
    plain_wallet_info_len_no_pad = Utilityntohl(plain_wallet_info_len_no_pad_big);
    if( read_len != sizeof(UINT32) || plain_wallet_info_len_no_pad > BOAT_REASONABLE_MAX_LEN )
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to read from keystore file.");
        boat_throw(BOAT_ERROR, BoatWalletLoadWallet_cleanup);
    }

    // Encrypted length (with padding) is round up to the nearest multiple of AES_BLOCK_SIZE
    encrypted_total_len = ROUNDUP(plain_wallet_info_len_no_pad, AES_BLOCK_SIZE);
    
    encrypted_wallet_info_array = BoatMalloc(encrypted_total_len);
    plain_wallet_info_array =  BoatMalloc(encrypted_total_len);

    if( encrypted_wallet_info_array == NULL || plain_wallet_info_array == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to allocate memory.");
        boat_throw(BOAT_ERROR, BoatWalletLoadWallet_cleanup);
    }

    // Read encrypted wallet info
    read_len = fread(encrypted_wallet_info_array, 1, encrypted_total_len, key_store_file_ptr);
    if( read_len != encrypted_total_len )
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to read from keystore file.");
        boat_throw(BOAT_ERROR, BoatWalletLoadWallet_cleanup);
    }


    // Hash the password to generate the AES key
    keccak_256(passwd_ptr, passwd_len, aes256key);


    // Initialize OpenSSL EVP context for cipher/decipher
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX_init(ctx_ptr);
#else
    ctx_ptr = EVP_CIPHER_CTX_new();
#endif

    // Specify AES256-CBC Algorithm, AES key and initial vector of CBC (IV is not care)
    openssl_ret = EVP_DecryptInit_ex(ctx_ptr, EVP_aes_256_cbc(), NULL, aes256key, iv);
    if( openssl_ret != 1 )
    {
        BoatLog(BOAT_LOG_NORMAL, "EVP_DecryptInit_ex failed.");
        boat_throw(BOAT_ERROR, BoatWalletLoadWallet_cleanup);
    }

    // Disable default padding, because default PKCS padding cannot be distinguished
    // from effective data in some extreme case.
    EVP_CIPHER_CTX_set_padding(ctx_ptr, 0);

    plain_wallet_info_len = 0;
    openssl_ret = EVP_DecryptUpdate(ctx_ptr, plain_wallet_info_array + plain_wallet_info_len, &openssl_len, encrypted_wallet_info_array, encrypted_total_len);
    if( openssl_ret != 1 )
    {
        BoatLog(BOAT_LOG_NORMAL, "EVP_DecryptUpdate failed.");
        boat_throw(BOAT_ERROR, BoatWalletLoadWallet_cleanup);
    }
    plain_wallet_info_len += openssl_len;

    openssl_ret = EVP_DecryptFinal_ex(ctx_ptr, plain_wallet_info_array + plain_wallet_info_len, &openssl_len);
    if( openssl_ret != 1 )
    {
        BoatLog(BOAT_LOG_NORMAL, "EVP_DecryptFinal_ex failed.");
        boat_throw(BOAT_ERROR, BoatWalletLoadWallet_cleanup);
    }
    plain_wallet_info_len += openssl_len;


    // Check decrypted plain wallet info's hash
    // NOTE: IV-independent decryption: ingore firest AES block
    keccak_256(plain_wallet_info_array + AES_BLOCK_SIZE, plain_wallet_info_len_no_pad - AES_BLOCK_SIZE, wallet_info_hash_array);
    if( memcmp(wallet_info_hash_array, stored_wallet_info_hash_array, sizeof(wallet_info_hash_array)) != 0 )
    {
        BoatLog(BOAT_LOG_NORMAL, "Load wallet info fails: bad checksum.");
        boat_throw(BOAT_ERROR, BoatWalletLoadWallet_cleanup);
    }


    result = BoatWalletCheckPrivkey(((BoatWalletInfo*)(plain_wallet_info_array + AES_BLOCK_SIZE))->account_info.priv_key_array);

    if( result != BOAT_SUCCESS )
    {
        BoatLog(BOAT_LOG_NORMAL, "Load wallet info fails: invalid private key.");
        boat_throw(BOAT_ERROR, BoatWalletLoadWallet_cleanup);
    }

    // copy wallet info to g_boat_wallet_info

    // Ignore the beginning 16 bytes for IV-dependent decryption
    plain_wallet_info_field_index = AES_BLOCK_SIZE;
    
    memcpy(&g_boat_wallet_info.account_info.priv_key_array, plain_wallet_info_array + plain_wallet_info_field_index, sizeof(g_boat_wallet_info.account_info.priv_key_array));
    plain_wallet_info_field_index += sizeof(g_boat_wallet_info.account_info.priv_key_array);

    memcpy(&g_boat_wallet_info.account_info.pub_key_array, plain_wallet_info_array + plain_wallet_info_field_index, sizeof(g_boat_wallet_info.account_info.pub_key_array));
    plain_wallet_info_field_index += sizeof(g_boat_wallet_info.account_info.pub_key_array);
    
    memcpy(&g_boat_wallet_info.account_info.address, plain_wallet_info_array + plain_wallet_info_field_index, sizeof(g_boat_wallet_info.account_info.address));
    plain_wallet_info_field_index += sizeof(g_boat_wallet_info.account_info.address);

    
    memcpy(&chain_id_big, plain_wallet_info_array + plain_wallet_info_field_index, sizeof(UINT32));
    g_boat_wallet_info.network_info.chain_id = Utilityntohl(chain_id_big);
    plain_wallet_info_field_index += sizeof(UINT32);

    memcpy(&g_boat_wallet_info.network_info.eip155_compatibility, plain_wallet_info_array + plain_wallet_info_field_index, sizeof(g_boat_wallet_info.network_info.eip155_compatibility));
    plain_wallet_info_field_index += sizeof(g_boat_wallet_info.network_info.eip155_compatibility);

    memcpy(&node_url_str_len_big, plain_wallet_info_array + plain_wallet_info_field_index, sizeof(UINT32));
    plain_wallet_info_field_index += sizeof(UINT32);

    node_url_str_len = Utilityhtonl(node_url_str_len_big);

    if( node_url_str_len !=
          plain_wallet_info_len_no_pad
        - AES_BLOCK_SIZE
        - sizeof(g_boat_wallet_info.account_info.priv_key_array)
        - sizeof(g_boat_wallet_info.account_info.pub_key_array)
        - sizeof(g_boat_wallet_info.account_info.address)
        - sizeof(g_boat_wallet_info.network_info.chain_id)
        - sizeof(g_boat_wallet_info.network_info.eip155_compatibility)
        - sizeof(UINT32))  // UINT32 for node url's length itself
    {
        BoatLog(BOAT_LOG_NORMAL, "Incorrect node url length");
        boat_throw(BOAT_ERROR, BoatWalletLoadWallet_cleanup);
    }
    

    if( g_boat_wallet_info.network_info.node_url_ptr != NULL )
    {
        BoatFree(g_boat_wallet_info.network_info.node_url_ptr);
    }

    // +1 for NULL Terminator
    g_boat_wallet_info.network_info.node_url_ptr = BoatMalloc(node_url_str_len + 1);

    if( g_boat_wallet_info.network_info.node_url_ptr == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to allocate memory.");
        boat_throw(BOAT_ERROR, BoatWalletLoadWallet_cleanup);
    }

    memcpy(g_boat_wallet_info.network_info.node_url_ptr, plain_wallet_info_array + plain_wallet_info_field_index, node_url_str_len);
    plain_wallet_info_field_index += node_url_str_len;

    // Add a NULL teriminator
    g_boat_wallet_info.network_info.node_url_ptr[node_url_str_len] = '\0';


    ///

    boat_catch(BoatWalletLoadWallet_cleanup)
    {
        result = boat_exception;
    }


#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX_cleanup(&ctx);
#else
    if( ctx_ptr != NULL )
    {
        EVP_CIPHER_CTX_free(ctx_ptr);
    }
#endif


    // Destroy sensitive information
    memset(aes256key, 0, sizeof(aes256key));
    memset(encrypted_wallet_info_array, 0, encrypted_total_len);
    memset(plain_wallet_info_array, 0, plain_wallet_info_len);
    memset(wallet_info_hash_array, 0, sizeof(wallet_info_hash_array));
    memset(stored_wallet_info_hash_array, 0, sizeof(stored_wallet_info_hash_array));

    if( key_store_file_ptr != NULL )
    {
        fclose(key_store_file_ptr);
    }
    

    if( encrypted_wallet_info_array != NULL )
    {
        BoatFree(encrypted_wallet_info_array);
    }
    
    if( plain_wallet_info_array != NULL )
    {
        BoatFree(plain_wallet_info_array);
    }


    return result;
}


/*!*****************************************************************************
@brief Load wallet information from keystore file with AES encryption

Function: BoatWalletLoadWallet()

    This function loads wallet information from specified keystore file to
    g_boat_wallet_info.

    This function is a derived version of BoatWalletLoadWalletEx().


@see
    BoatWalletLoadWalletEx() BoatWalletSaveWalletEx() BoatWalletSaveWallet() BoatWalletCheckPrivkey()

@return
    This function returns BOAT_SUCCESS if loading keystore is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] passwd_ptr
    Password to decrypt the keystore.

@param[in] passwd_len
    Length of <passwd_ptr> in byte.\n
    Note that if <passwd_ptr> is a string, either <passwd_len> includes\n
    null-terminator or not is OK. But it must be the same as BoatWalletSaveWallet().

@param[in] file_path_str
    Keystore file's path.

*******************************************************************************/
BOAT_RESULT BoatWalletLoadWallet(const UINT8 *passwd_ptr, UINT32 passwd_len, const CHAR *file_path_str)
{
    return(BoatWalletLoadWalletEx(&g_boat_wallet_info, passwd_ptr, passwd_len, file_path_str));
}


/*!*****************************************************************************
@brief Set Transaction Parameter: Transaction Nonce

Function: BoatTxSetNonce()

    This function sets the nonce to the transaction count of the account
    obtained from network.

    This function can be called after BoatWalletSetPrivkey() has been called.

@return
    This function returns BOAT_SUCCESS if setting is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param This function doesn't take any argument.
        
*******************************************************************************/
BOAT_RESULT BoatTxSetNonce(void)
{
    CHAR account_address_str[43];
    Param_eth_getTransactionCount param_eth_getTransactionCount;
    CHAR *tx_count_str;


    // PRIVATE KEY MUST BE SET BEFORE SETTING NONCE, BECAUSE GETTING NONCE FROM
    // NETWORK NEEDS ETHEREUM ADDRESS, WHICH IS COMPUTED FROM KEY


    // Get transaction count from network
    // Return value of web3_eth_getTransactionCount() is transaction count
    
    UtilityBin2Hex(
        account_address_str,
        g_boat_wallet_info.account_info.address,
        20,
        BIN2HEX_LEFTTRIM_UFMTDATA,
        BIN2HEX_PREFIX_0x_YES,
        BOAT_FALSE
        );
        
    param_eth_getTransactionCount.address_str = account_address_str;
    param_eth_getTransactionCount.block_num_str = "latest";


    
    tx_count_str = web3_eth_getTransactionCount(
                                    g_boat_wallet_info.network_info.node_url_ptr,
                                    &param_eth_getTransactionCount);

    if( tx_count_str == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Fail to get transaction count from network.");
        return BOAT_ERROR;
    }

    // Set nonce from transaction count
    g_tx_info.rawtx_fields.nonce.field_len =
    UtilityHex2Bin(
                    g_tx_info.rawtx_fields.nonce.field,
                    32,
                    tx_count_str,
                    TRIMBIN_LEFTTRIM,
                    BOAT_TRUE
                  );

    return BOAT_SUCCESS;
}


/*!*****************************************************************************
@brief Set Transaction Parameter: GasPrice

Function: BoatTxSetGasPrice()

    This function sets the gas price of the transaction.


@return
    This function returns BOAT_SUCCESS if setting is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] gas_price_ptr
    The gas price in wei.\n
    If <gas_price_ptr> is NULL, the gas price obtained from network is used.
        
*******************************************************************************/
BOAT_RESULT BoatTxSetGasPrice(TxFieldMax32B *gas_price_ptr)
{
    CHAR *gas_price_from_net_str;
    BOAT_RESULT result = BOAT_SUCCESS;

    // If gas price is specified, use it
    // Otherwise use gas price obtained from network
    if( gas_price_ptr != NULL )
    {
        memcpy(&g_tx_info.rawtx_fields.gasprice,
                gas_price_ptr,
                sizeof(TxFieldMax32B));
    }
    else
    {
        // Get current gas price from network
        // Return value of web3_eth_gasPrice is in wei
        
        gas_price_from_net_str = web3_eth_gasPrice(g_boat_wallet_info.network_info.node_url_ptr);

        if( gas_price_from_net_str == NULL )
        {
            BoatLog(BOAT_LOG_NORMAL, "Fail to get gasPrice from network.");
            result = BOAT_ERROR;
        }
        else
        {
            // Set transaction gasPrice with the one got from network
            g_tx_info.rawtx_fields.gasprice.field_len =
            UtilityHex2Bin(
                            g_tx_info.rawtx_fields.gasprice.field,
                            32,
                            gas_price_from_net_str,
                            TRIMBIN_LEFTTRIM,
                            BOAT_TRUE
                          );

            BoatLog(BOAT_LOG_VERBOSE, "Use gasPrice from network: %s wei.", gas_price_from_net_str);
        }
    }
    
    return result;
}


/*!*****************************************************************************
@brief Set Transaction Parameter: GasLimit

Function: BoatTxSetGasLimit()

    This function sets the gas limit of the transaction.


@return
    This function returns BOAT_SUCCESS if setting is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] gas_limit_ptr
    The gas limit
        
*******************************************************************************/
BOAT_RESULT BoatTxSetGasLimit(TxFieldMax32B *gas_limit_ptr)
{
    // Set gasLimit
    if( gas_limit_ptr != NULL )
    {
        memcpy(&g_tx_info.rawtx_fields.gaslimit,
                gas_limit_ptr,
                sizeof(TxFieldMax32B));

        return BOAT_SUCCESS;
    }
    else
    {
        BoatLog(BOAT_LOG_NORMAL, "Argument cannot be NULL.");
        return BOAT_ERROR;
    }
    
}


/*!*****************************************************************************
@brief Set Transaction Parameter: Recipient Address

Function: BoatTxSetRecipient()

    This function sets the address of the transaction recipient.


@return
    This function returns BOAT_SUCCESS if setting is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] address
    The address of the recipient
        
*******************************************************************************/
BOAT_RESULT BoatTxSetRecipient(BoatAddress address)
{
    // Set recipient's address
    memcpy(&g_tx_info.rawtx_fields.recipient,
            address,
            sizeof(BoatAddress));

    return BOAT_SUCCESS;    
}


/*!*****************************************************************************
@brief Set Transaction Parameter: Value

Function: BoatTxSetValue()

    This function sets the value of the transaction.


@return
    This function returns BOAT_SUCCESS if setting is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param[in] value_ptr
    The value of the transaction.\n
    If <value_ptr> is NULL, it's treated as no value being transfered.
        
*******************************************************************************/
BOAT_RESULT BoatTxSetValue(TxFieldMax32B *value_ptr)
{
    // Set value
    if( value_ptr != NULL )
    {
        memcpy(&g_tx_info.rawtx_fields.value,
                value_ptr,
                sizeof(TxFieldMax32B));

    }
    else
    {
        // If value_ptr is NULL, value is treated as 0.
        // NOTE: value.field_len == 0 has the same effect as
        //       value.field_len == 1 && value.field[0] == 0x00 for RLP encoding
        g_tx_info.rawtx_fields.value.field_len = 0;
    }

    return BOAT_SUCCESS;
    
}


/*!*****************************************************************************
@brief Set Transaction Parameter: Data

Function: BoatTxSetData()

    This function sets the data of the transaction.


@return
    This function returns BOAT_SUCCESS if setting is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param data_ptr[in]
    The data of the transaction. Note that data_ptr->field_ptr itself is only\n
    a pointer without any associated storage by default. The caller must\n
    allocate appropriate memory for it.\n
    If <data_ptr> is NULL, it's treated as no data being transfered.
        
*******************************************************************************/
BOAT_RESULT BoatTxSetData(TxFieldVariable *data_ptr)
{
    // Set data
    if( data_ptr != NULL )
    {
        // NOTE: g_tx_info.rawtx_fields.data.field_ptr is a pointer
        //       The caller must make sure the storage it points to is available
        //       until the transaction is sent.
        memcpy(&g_tx_info.rawtx_fields.data,
                data_ptr,
                sizeof(TxFieldVariable));

    }
    else
    {
        // If data_ptr is NULL, value is treated as 0.
        // NOTE: data.field_len == 0 has the same effect as
        //       data.field_len == 1 && data.field_ptr[0] == 0x00 for RLP encoding
        g_tx_info.rawtx_fields.data.field_len = 0;
    }

    return BOAT_SUCCESS;
    
}


/*!*****************************************************************************
@brief Sign and send a transaction. Also call a stateful contract function.

Function: BoatTxSend()

    This function sign and set a transaction.

    BoatWalletSetXXX() and BoatTxSetXXX() functions must be properly called
    before call this function.

    A transaction whose recipient may be an EOA address or a contract address.
    In latter case it's usually a contract function call.

    This function invokes the eth_sendRawTransaction RPC method.
    eth_sendRawTransaction method only applies the transaction and returns a
    transaction hash. The transaction is not verified (got mined) until the
    nodes in the network get into consensus about the transaction. This
    function will invoke eth_getTransactionReceipt method to wait for the
    transaction being mined or timeout.

    If the transaction is a contract function call, the caller cannot get its
    return value because the transaction is asynchronously executed. It's a
    good practice to save the return value in a state variable and use
    BoatCallContractFunc() to call a "read" contract function that could read
    and return the state variable.


    NOTE:

    Any contract function that may change the state of the contract shall
    be called in a transaction way. "state" is the "global variable" used in a
    contract.

    Any contract function that doesn't change the state of the contract can
    be called either in a transaction way or by BoatCallContractFunc(), which
    invokes the eth_call RPC method. However the former will consume gas and
    latter doesn't consume gas.

@see BoatCallContractFunc()
    
    
@return
    This function returns BOAT_SUCCESS if setting is successful.\n
    Otherwise it returns BOAT_ERROR.
    

@param This function doesn't take any argument.
*******************************************************************************/
BOAT_RESULT BoatTxSend(void)
{
    BOAT_RESULT result;

    result = RawtxPerform(&g_boat_wallet_info, &g_tx_info);

    return result;
}


/*!*****************************************************************************
@brief Call a state-less contract function

Function: BoatCallContractFunc()

    This function calls contract function that doesn't change the state of the
    contract. "state" is the "global variable" used in a contract.

    This function invokes the eth_call RPC method. eth_call method requests the
    blockchain node to execute the function without affecting the block chain.
    The execution runs only on the requested node thus it can return immediately
    after the execution. This function synchronously return the return value
    of the eth_call method, which is the return value of the contract function.

    To call contract function that may change the state, use BoatTxSend()
    instead.

    If call a contract function that may change the state with
    BoatCallContractFunc(), the function will be executed and return a value,
    but none of the states will change.

@see BoatTxSend()

@return
    This function returns a HEX string representing the return value of the\n
    called contract function.\n
    If any error occurs, it returns NULL.
    

@param[in] contract_addr_str
    A HEX string representing the address of th called contract.

@param[in] func_proto_str
    A string representing the prototype of the called function.\n
    Note: uint is treated as uint256.\n
    e.g.: for the contract function:\n
        function readListByIndex(uint index) public view returns (bytes32 event_)\n
    its prototype is "readListByIndex(uint256)"

@param[in] func_param_ptr
    A byte stream containing the parameters to pass to the function.\n
    The layout conforms to Ethereum ABI: https://solidity.readthedocs.io/en/develop/abi-spec.html\n
    If <func_param_ptr> is NULL, this function doesn't take any parameter.

@param[in] func_param_len
    Length of <func_param_ptr> in byte.
        
*******************************************************************************/
CHAR * BoatCallContractFunc(
                    CHAR * contract_addr_str,
                    CHAR *func_proto_str,
                    UINT8 *func_param_ptr,
                    UINT32 func_param_len)
{
    UINT8 function_selector[32];

    // +4 for function selector, *2 for bin to HEX, + 3 for "0x" prefix and NULL terminator
    CHAR data_str[(func_param_len + 4)*2 + 3]; // Compiler MUST support C99 to allow variable-size local array
    
    Param_eth_call param_eth_call;
    CHAR *retval_str;

    if(    contract_addr_str == NULL
        || func_proto_str == NULL
        || (func_param_ptr == NULL && func_param_len != 0)
       )
    {
        BoatLog(BOAT_LOG_NORMAL, "Arguments cannot be NULL.");
        return NULL;
    }

    param_eth_call.to = contract_addr_str;

    // Function call consumes zero gas but gasLimit and gasPrice must be specified.
    param_eth_call.gas = "0x1fffff";
    param_eth_call.gasPrice = "0x8250de00";

    keccak_256((UINT8*)func_proto_str, strlen(func_proto_str), function_selector);

    // Set function selector
    UtilityBin2Hex(
            data_str,
            function_selector,
            4,
            BIN2HEX_TRIM_NO,
            BIN2HEX_PREFIX_0x_YES,
            BOAT_FALSE);

    // Set function parameters
    UtilityBin2Hex(
            data_str+10,        // "0x12345678" function selector prefixed
            func_param_ptr,
            func_param_len,
            BIN2HEX_TRIM_NO,
            BIN2HEX_PREFIX_0x_NO,
            BOAT_FALSE);

    
    param_eth_call.data = data_str;

    retval_str = web3_eth_call(
                                g_boat_wallet_info.network_info.node_url_ptr,
                                &param_eth_call);


    return retval_str;

}



