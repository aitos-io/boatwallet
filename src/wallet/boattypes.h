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

/*!@brief Define Basic Boatwallet types and include external header files

@file
boattypes.h defines types used in boatwallet.
*/

#ifndef __BOATTYPES_H__
#define __BOATTYPES_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#include "sha3.h"
#include "secp256k1.h"

#include "wallet/boatoptions.h"
#include "wallet/boatexception.h"


#define BOAT_OUT
#define BOAT_IN
#define BOAT_INOUT

#define BOAT_TRUE 1
#define BOAT_FALSE 0

//! Define a resonable max length in bytes for general checking for memory
//! allocation related validation. The lower end the embedded system is, the
//! smaller the length is.
#define BOAT_REASONABLE_MAX_LEN  8192u

typedef bool BOATBOOL;
typedef char CHAR;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long int UINT64;
typedef signed char SINT8;
typedef signed short SINT16;
typedef signed int SINT32;
typedef signed long long int SINT64;

typedef UINT8 UINT256ARRAY[32];

typedef SINT32 BOAT_RESULT;

typedef UINT8 BoatAddress[20];

//!@brief Account information

//! An account's only identifier is its private key. An address is calculated
//! from the public key and the public key is calculated from the private key.
typedef struct TAccountInfo
{
    UINT8 priv_key_array[32]; //!< Private key of the account. In case co-sign is used, it's the half key shard.
    UINT8 pub_key_array[64];  //!< Public key of the account
    BoatAddress address;       //!< Account address calculated from prublic key
}AccountInfo;


//!@brief Blockchain network information

//! EIP-155 (https://github.com/ethereum/EIPs/blob/master/EIPS/eip-155.md) requires
//! chain ID of the network being part of the transaction before it's signed.
//! If the network is NOT EIP-155 compatible, <eip155_compatibility> must be FALSE
//! and <chain_id> is ignored. Otherwise the chain ID must be set.\n
//! <node_url_ptr> must include the protocol descriptor, IP address or URL name and 
//! port. For example, http://a.b.com:8545
typedef struct TNetworkInfo
{
    UINT32 chain_id;    //!< Chain ID (in host endian) of the blockchain network if the network is EIP-155 compatible

    /*!@brief Network EIP-155 compatibility

    If the network is EIP-155 compabible <eip155_compatibility> must be set to TRUE and <chain_id>
    must be set.\n
    Otherwise set it to FALSE and <chain_id> is ignored.*/
    UINT8 eip155_compatibility;

    CHAR *node_url_ptr; //!< URL of the blockchain node, e.g. "http://a.b.com:8545"
}NetworkInfo;


//!@brief Wallet information

//! Wallet information consists of account and block chain network information.
//! Currently only one account per wallet is supported.
typedef struct TBoatWalletInfo
{
    AccountInfo account_info; //!< Account information
    NetworkInfo network_info; //!< Network information
}BoatWalletInfo;

//!@brief common struct for variable length transaction fields
typedef struct TTxFieldVariable
{
    UINT8 *field_ptr;  //!< The address of the field storage. The storage MUST be allocated before use.
    UINT32 field_len;  //!< The length of the field in byte
}TxFieldVariable;

//!@brief common struct for 4-byte (32-bit) length transaction fields
typedef struct TTxFieldMax4B
{
    UINT8 field[4];     //!< Field storage
    UINT32 field_len;   //!< The effective length of the field in byte
}TxFieldMax4B;

//!@brief common struct for 32-byte (256-bit) length transaction fields
typedef struct TTxFieldMax32B
{
    UINT8 field[32];    //!< Field storage
    UINT32 field_len;   //!< The effective length of the field in byte
}TxFieldMax32B;

//!@brief ECDSA signature struct
typedef struct TTxFieldSig
{
    union
    {
        struct
        {
            UINT8 r32B[32]; //!< r part of the signature
            UINT8 s32B[32]; //!< s part of the signature
        };
        UINT8 sig64B[64];   //!< consecutive signature composed of r+s
    };
    UINT8 r_len;            //!< Effective length of r, either 0 for unsigned tx and 32 for signed tx
    UINT8 s_len;            //!< Effective length of s, either 0 for unsigned tx and 32 for signed tx
}TxFieldSig;

//!@brief RAW transaction fields
typedef struct TRawtxFields
{
    TxFieldMax32B nonce;        //!< nonce, uint256 in bigendian, equal to the transaction count of the sender's account address
    TxFieldMax32B gasprice;     //!< gasprice in wei, uint256 in bigendian
    TxFieldMax32B gaslimit;     //!< gaslimit, uint256 in bigendian
    BoatAddress recipient;       //!< recipient's address, 160 bits
    TxFieldMax32B value;        //!< value to transfer, uint256 in bigendian
    TxFieldVariable data;       //!< data to transfer, unformatted stream
    TxFieldMax4B v;             //!< chain id or recovery identifier, @see RawtxPerform()
    TxFieldSig sig;             //!< ECDSA signature, including r and s parts
}RawtxFields;

//!@brief Transaction information
typedef struct TTxInfo
{
    struct TRawtxFields rawtx_fields;       //!< RAW transaction fields
    TxFieldMax32B tx_hash;                  //!< Transaction hash returned from network
}TxInfo;




#endif
