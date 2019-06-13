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

/*!@brief Utility functions

@file
utility.c contains utility functions for boatwallet.
*/

#include "wallet/boattypes.h"
#include "utilities/utility.h"

//!@brief Literal representation of log level
const CHAR  * const g_log_level_name_str[] = 
{
    "LOG_CRITICAL",
    "LOG_NORMAL",
    "LOG_VERBOSE"
};



/*!*****************************************************************************
@brief Trim zeros of a binary stream

Function: UtilityTrimBin()

    This function trims zeros of a binary stream and returns the length after
    trimming. Currently only left trimming (i.e. trimming leading zeros) is
    supported.

    This function simply ignores leading zeros and copy the first non-zero byte
    til the end of the stream to the target buffer. It doesn't treat the data
    in buffer as integer, i.e. it doesn't care about the endian.

    The source and target buffer may be overlapped.
    
    For example, {0x00, 0x01, 0x02, 0x00} is trimed to {0x01, 0x02, 0x00} and
    returns 3.
    

@return
    This function returns how many bytes the trimmed binary stream is.
    

@param to_ptr
        A buffer to contain the trimmed binary stream. Its size must be at
        least <from_len> bytes. <to_ptr> and <from_ptr> may be overlapped.
        
@param from_ptr
        The binary stream to trim. <to_ptr> and <from_ptr> may be overlapped.

@param from_len
        Length of <from_ptr>.

@param trim_mode
        Only TRIMBIN_LEFTTRIM is supported, i.e. trim leading zeros.\n
        If TRIMBIN_TRIM_NO is specified, this function acts like memmove().
        
@param zero_as_null
        In case the binary stream contains only one or multiple 0x00, if\n
        <zero_as_null> is BOAT_TRUE, it will be treated as a null stream and\n
        returns 0.\n
        If <zero_as_null> is BOAT_FALSE, it will be trimed to a single 0x00 and\n
        returns 1.\n
        <zero_as_null> should be BOAT_TRUE for RLP-Encoding purpose.

*******************************************************************************/
UINT32 UtilityTrimBin(
                BOAT_OUT UINT8 *to_ptr,
                const UINT8 *from_ptr,
                UINT32 from_len,
                TRIMBIN_TRIM_MODE trim_mode,
                BOATBOOL zero_as_null
                )
{
    UINT32 from_offset = 0;
    
        
    if( to_ptr == NULL || from_ptr == NULL)
    {
        BoatLog(BOAT_LOG_NORMAL, "<to_ptr> and <from_ptr> cannot be NULL.");
        return 0;
    }


    if( trim_mode == TRIMBIN_TRIM_NO)
    {
        // Duplicate byte array
        if( to_ptr != from_ptr )
        {
            memmove(to_ptr, from_ptr, from_len);
        }
    }
    else  // if( trim_mode == TRIMBIN_LEFTTRIM )
    {
        for( from_offset = 0; from_offset < from_len; from_offset++ )
        {
            // Trim leading 0x00, i.e. {0x00, 0x01, 0x00 0xAB} => {0x01, 0x00 0xAB}
            if( from_ptr[from_offset] == 0 )
            {
                continue;
            }
            else
            {
                memmove(to_ptr, from_ptr + from_offset, from_len - from_offset);
                break;
            }
        }
    }

    // Special process for all zero byte array
    
    if( trim_mode != TRIMBIN_TRIM_NO && from_offset == from_len )
    {
        if( zero_as_null == BOAT_FALSE )
        {
            to_ptr[0] = 0;
            return 1;
        }
        else
        {
            return 0;
        }
    }

    return from_len - from_offset;
}


/*!****************************************************************************
@brief Convert a binary stream to HEX string with optional leading zeros trimming and "0x" prefix

Function: UtilityBin2Hex()

    This function converts a binary stream to a null-terminated HEX string.
    Optionally leading zeros can be trimmed during conversion.
    Optionally "0x" prefix can be prepended to the HEX string.

    There is no space to delimit every HEX code.
    The case of "a" through "f" is always lower case.
    
    There are 3 ways to trim leading zeros. For example, a binary stream
    {0x00, 0x01, 0x00, 0xAB} will be converted to string:
    
    "000100ab": if <trim_mode> = BIN2HEX_TRIM_NO
    "100ab"   : if <trim_mode> = BIN2HEX_LEFTTRIM_QUANTITY
    "0100ab"  : if <trim_mode> = BIN2HEX_LEFTTRIM_UFMTDATA

    Note that this function doesn't treat the binary stream as an integer and thus
    it doens't make any endian conversion.


@return
    This function returns the length of the converted HEX string excluding NULL\n
    terminator. It equals to strlen of the string.\n
    If <to_str> is NULL, this function does nothing and returns 0.\n
    If <from_len> is 0 or <from_ptr> is NULL, and <to_str> is NOT NULL, this\n
    function writes a '\0' to <to_str>, i.e. a NULL string and returns 1.
    

@param[out] to_str
        The buffer to hold the converted HEX string.\n
        Its size should be least <from_len>*2+1 bytes if <prefix_0x_mode>=BIN2HEX_PREFIX_0x_NO,\n
        or <from_len>*2+3 bytes if <prefix_0x_mode>=BIN2HEX_PREFIX_0x_YES.\n
        If <to_str> is NULL, this function does nothing and returns 0.
        
@param[in] from_ptr
        The binary stream to convert from.\n
        If <from_ptr> is NULL and <to_str> is NOT NULL, this function writes a\n
        '\0' to <to_str>, i.e. a NULL string and returns 1.

@param[in] from_len
        The length of <from_ptr> in byte.\n
        If <from_len> is 0 and <to_str> is NOT NULL, this function writes a\n
        '\0' to <to_str>, i.e. a NULL string and returns 1.

@parmm[in] trim_mode
        BIN2HEX_TRIM_NO if no trimming is required;\n
        BIN2HEX_LEFTTRIM_QUANTITY to trim as a quantity;\n
        BIN2HEX_LEFTTRIM_UFMTDATA to trim as unformatted data

@param[in] prefix_0x_mode
        BIN2HEX_PREFIX_0x_YES: Prepend a "0x" prefix at the beginning of the HEX string;\n
        BIN2HEX_PREFIX_0x_NO:  Don't prepend "0x" prefix.
        
@param zero_as_null
        In case the binary stream contains only one or multiple 0x00, if\n
        <zero_as_null> is BOAT_TRUE, it will be converted to a null sting and\n
        returns 0.\n
        If <zero_as_null> is BOAT_FALSE, it will be converted to "0" or "00"\n
        according to <trim_mode>.\n
        <zero_as_null> should be BOAT_TRUE for RLP-Encoding purpose.

*******************************************************************************/
UINT32 UtilityBin2Hex(
                BOAT_OUT CHAR *to_str,
                const UINT8 *from_ptr,
                UINT32 from_len,
                BIN2HEX_TRIM_MODE trim_mode,
                BIN2HEX_PREFIX_0x_MODE prefix_0x_mode,
                BOATBOOL zero_as_null
                )
{
    UINT32 to_offset;
    unsigned char halfbyte;
    unsigned char octet;
    char halfbytechar;
    int i, j;
    BOATBOOL trim_done;
    
        
    if( to_str == NULL )
    {
        return 0;
    }

    if( from_ptr == NULL || from_len == 0 )
    {
        to_str[0] = '\0';
        return 1;
    }

    to_offset = 0;

    if( prefix_0x_mode == BIN2HEX_PREFIX_0x_YES)
    {
        to_str[to_offset++] = '0';  // "0x"
        to_str[to_offset++] = 'x';
    }


    if( trim_mode == BIN2HEX_TRIM_NO )
    {
        trim_done = BOAT_TRUE;
    }
    else
    {
        trim_done = BOAT_FALSE;
    }
    
    for( i = 0; i < from_len; i++ )
    {
        octet = from_ptr[i];
        
        // Trim leading double zeroes, i.e. {0x00, 0x01, 0x00 0xAB} => "0100AB"
        if( trim_done == BOAT_FALSE && octet == 0 )
        {
            continue;
        }
        
        for( j = 0; j < 2; j++)
        {
            halfbyte = (octet>>(4-j*4))&0x0F;

            // Trim all leading zeroes, i.e. {0x00, 0x01, 0x00 0xAB} => "100AB"
            if(    trim_done == BOAT_FALSE
                && trim_mode == BIN2HEX_LEFTTRIM_QUANTITY
                && halfbyte == 0)
            {
                continue;
            }
            
            if( halfbyte >= 0x0 && halfbyte <= 0x9)
            {
                halfbytechar = halfbyte + '0';
            }
            else
            {
                halfbytechar = halfbyte - 10 + 'a';
            }

            to_str[to_offset++] = halfbytechar;
            
            trim_done = BOAT_TRUE;
            
        }
    }


    // Special process for all zero byte array
    
    if(   (prefix_0x_mode == BIN2HEX_PREFIX_0x_YES && to_offset == 2)  // HEX contains only "0x"
       || (prefix_0x_mode == BIN2HEX_PREFIX_0x_NO  && to_offset == 0)) // HEX contains nothing
    {
        if( zero_as_null == BOAT_FALSE )
        {
            if( trim_mode == BIN2HEX_LEFTTRIM_QUANTITY )
            {
                to_str[to_offset++] = '0';  // "0"
            }
            else // if( trim_mode == BIN2HEX_LEFTTRIM_UFMTDATA )
            {
                to_str[to_offset++] = '0';  // "00"
                to_str[to_offset++] = '0';
            }
        }
        else
        {
            to_offset = 0;
        }
    }

    to_str[to_offset] = '\0';

    return to_offset;
}


/*!*****************************************************************************
@brief Convert a HEX string to binary stream with optional leading zeros trimming

Function: UtilityHex2Bin()

Description:
    This function converts a HEX string to a binary stream.

    Optionally leading zeros can be trimmed during conversion.
    
    If there is "0x" prefix at the beginning of the HEX string, it's ignored.

    There should be no space between HEX codes.

    If <to_size> is too small to hold the converted binary stream, only the
    beginning <to_size> bytes are converted and written to <to_ptr>.

    The trim_mode is the same as UtilityTrimBin(). Odd length of HEX string is
    allowed as if it were left filled with a "0". For example, a HEX string
    "0x00123ab" is treated as "0x000123ab" and converted to:
    {0x01, 0x23, 0xab}: if <trim_mode> = TRIMBIN_LEFTTRIM;
    {0x00, 0x01, 0x23, 0xab}: if <trim_mode> = TRIMBIN_TRIM_NO

    Note that this function doesn't treat the HEX string as an integer and thus
    it doens't make any endian conversion.
    
@see UtilityTrimBin()
    

@return
    This function returns how many bytes the converted binary stream is if the\n
    conversion completes successfully. If the output buffer is too small to\n
    hold the converted binary stream, it returns <to_size>.\n
    If any non-HEX character is encountered, or <from_str> is an all-zero HEX\n
    string and <trim_mode> is TRIMBIN_LEFTTRIM, it returns 0.
    

@param[out] to_ptr
        The buffer to hold the converted binary stream.

@param[in] to_size
        The size of <to_ptr> in bytes.\n
        It's safe to ensure <to_size> >= (strlen(from_str)+1)/2 bytes.\n
        
@param[in] from_str
        The null-terminated HEX string to convert from. "0x" prefix is optional\n
        and ignored. There should be no space between HEX codes.
        
@param[in] trim_mode
        TRIMBIN_LEFTTRIM: Trim leading zeros;\n
        TRIMBIN_TRIM_NO:  Don't trim.

@param[in] zero_as_null
        In case the HEX string contains only one or multiple "00" and <trim_mode>\n
        is TRIMBIN_LEFTTRIM, if <zero_as_null> is BOAT_TRUE, nothing will be\n
        written to <to_ptr> and it returns 0.\n
        If <zero_as_null> is BOAT_FALSE, it will be converted to a single 0x00\n
        and returns 1.\n
        <zero_as_null> should be BOAT_TRUE for RLP-Encoding purpose.

*******************************************************************************/
UINT32 UtilityHex2Bin(
                        BOAT_OUT UINT8 *to_ptr,
                        UINT32 to_size,
                        const CHAR *from_str,
                        TRIMBIN_TRIM_MODE trim_mode,
                        BOATBOOL zero_as_null
                      )
{
    UINT32 from_offset;
    UINT32 from_len;
    UINT32 to_offset;
    UINT32 odd_flag;

    unsigned char octet;
    unsigned char halfbyte;
    char halfbytechar;
    BOATBOOL bool_trim_done;
     
    if( to_ptr == NULL || to_size == 0 || from_str == NULL)
    {
        BoatLog(BOAT_LOG_NORMAL, "<to_ptr>, <to_size> and <from_str> cannot be 0 or NULL.");
        return 0;
    }

    octet = 0;

    from_len = strlen(from_str);

    from_offset = 0;
    to_offset = 0;

    // Skip leading "0x" or "0X" if there be.
    // Note: If strlen(from_ptr) <= 2 or <from_up_to_len> <= 2, no "0x" prefix
    //       is allowed in HEX string.
    if( from_len > 2 )
    {
        if(     from_str[0] == '0' 
            && (from_str[1] == 'x' || from_str[1] == 'X')
           )
        {
            from_offset += 2;
        }
    }

            
    // if HEX length is odd, treat as if it were left filled with one more '0'
    if( (from_len&0x01) != 0 )
    {
        // length is odd 
        odd_flag = 1;
    }
    else
    {
        // length is even
        odd_flag = 0;
    }

    if( trim_mode == TRIMBIN_TRIM_NO)
    {
        bool_trim_done = BOAT_TRUE;
    }
    else
    {
        bool_trim_done = BOAT_FALSE;
    }
    

    while( from_offset < from_len )
    {
        halfbytechar = from_str[from_offset];
        
        if( halfbytechar >= '0' && halfbytechar <= '9')
        {
            halfbyte = halfbytechar - '0';
        }
        else if( halfbytechar >= 'A' && halfbytechar <= 'F')
        {
            halfbyte = halfbytechar - 'A' + 0x0A;
        }
        else if( halfbytechar >= 'a' && halfbytechar <= 'f')
        {
            halfbyte = halfbytechar - 'a' + 0x0A;
        }
        else
        {
            BoatLog(BOAT_LOG_NORMAL, "<from_str> contains non-HEX character 0x%02x (%c) at Position %d of \"%s\".\n", halfbytechar, halfbytechar, from_offset, from_str);
            if( halfbytechar == ' ' || halfbytechar == '\t' )
            {
                BoatLog(BOAT_LOG_NORMAL, "There should be no space between HEX codes.");
            }
            return 0;
        }

        // If from_len is even, pack 2 half bytes to a byte when from_offset is odd.
        // For example, "0x012345" is packed when from_offset points to '1', '3' and '5'.
        //
        // If from_len is odd, pack 2 half bytes to a byte when from_offset is even.
        // For example, "0x12345" is packed when from_offset points to '1', '3' and '5'.
        if( (from_offset&0x01) == odd_flag )
        {
            octet = halfbyte << 4;
            from_offset++;
            continue;
        }
        else
        {
            octet |= halfbyte;
        
            if( bool_trim_done == BOAT_FALSE && octet == 0x00 )
            {
                from_offset++;
                continue;  // Trim leading zeros.
            }
            else
            {
                bool_trim_done = BOAT_TRUE;
            }
            
            to_ptr[to_offset++] = octet;

            from_offset++;

            // Check capacity of output buffer
            if( to_offset >= to_size )
            {
                break;
            }
         }
    }

    // Special process for trimed all zero HEX string
    if( to_offset == 0 && zero_as_null == BOAT_FALSE)
    {
        to_ptr[0] = 0x00;
        to_offset = 1;
    }

    return to_offset;
    
}



/*!*****************************************************************************
@brief Convert a host endian UINT32 to bigendian with optional MSB zeros trimming

Function: UtilityUint32ToBigend()

    This function converts a host endian (typically littleendian) UINT32 integer
    to bigendian.

    Optionally most siganificant zeros can be trimmed during conversion.
    
    The trim_mode is the same as UtilityTrimBin(). For example, a UINT32 integer
    0x000123ab is converted to:
    {0x01, 0x23, 0xab}: if <trim_mode> = TRIMBIN_LEFTTRIM;
    {0x00, 0x01, 0x23, 0xab}: if <trim_mode> = TRIMBIN_TRIM_NO

    
@see UtilityTrimBin()
    

@return
    This function returns how many bytes the converted bigendian integer is.\n
    If <trim_mode> = TRIMBIN_TRIM_NO, it always returns 4.
    

@param[out] to_big_ptr
        The buffer to hold the converted binary stream.
        
@param[in] from_host_integer
        The host endian UINT32 integer to convert.

@param[in] trim_mode
        TRIMBIN_LEFTTRIM: Trim MSB zeros;\n
        TRIMBIN_TRIM_NO:  Don't trim.

*******************************************************************************/
UINT8 UtilityUint32ToBigend(
                BOAT_OUT UINT8 *to_big_ptr,
                UINT32 from_host_integer,
                TRIMBIN_TRIM_MODE trim_mode
                )
{
    BOATBOOL bool_trim_done;
    SINT8 i;
    SINT8 binary_index;
    UINT8 octet;
    
    binary_index = 0;
    
    if( trim_mode == TRIMBIN_TRIM_NO )
    {
        bool_trim_done = BOAT_TRUE;
    }
    else // trim_mode == TRIMBIN_TRIM_LEFTTRIM
    {
        bool_trim_done = BOAT_FALSE;
    }

    
    for( i = sizeof(from_host_integer)-1; i>=0; i-- )
    {
        octet = (UINT8)(from_host_integer >> i*8);
        
        if( bool_trim_done == BOAT_FALSE )
        {
            if( octet == 0x00 )
            {
                continue;  // Trim MSB zeros.
            }
            else
            {
                bool_trim_done = BOAT_TRUE;
            }
        }

        to_big_ptr[binary_index++] = octet;

    }

    
    if( binary_index == 0 )
    {
        to_big_ptr[0] = 0x00;
        binary_index = 1;
    }
    
   
    return binary_index;
}


/*!*****************************************************************************
@brief Convert a host endian UINT64 to bigendian with optional MSB zeros trimming

Function: UtilityUint64ToBigend()

    This function converts a host endian (typical littleendian) UINT64 integer
    to bigendian.

    Optionally most siganificant zeros can be trimmed during conversion.
    
    It's a 64-bit version of UtilityUint32ToBigend(). 

    
@see UtilityUint32ToBigend()  UtilityTrimBin()
    

@return
    This function returns how many bytes the converted bigendian integer is.\n
    If <trim_mode> = TRIMBIN_TRIM_NO, it always returns 8.
    

@param[out] to_big_ptr
        The buffer to hold the converted binary stream.
        
@param[in] from_host_integer
        The host endian UINT64 integer to convert.

@param[in] trim_mode
        TRIMBIN_LEFTTRIM: Trim MSB zeros;\n
        TRIMBIN_TRIM_NO:  Don't trim.

*******************************************************************************/
UINT8 UtilityUint64ToBigend(
                BOAT_OUT UINT8 *to_big_ptr,
                UINT64 from_host_integer,
                TRIMBIN_TRIM_MODE trim_mode
                )
{
    BOATBOOL bool_trim_done;
    SINT8 i;
    SINT8 binary_index;
    UINT8 octet;
    
    binary_index = 0;
    
    if( trim_mode == TRIMBIN_TRIM_NO )
    {
        bool_trim_done = BOAT_TRUE;
    }
    else // trim_mode == TRIMBIN_TRIM_LEFTTRIM
    {
        bool_trim_done = BOAT_FALSE;
    }

    
    for( i = sizeof(from_host_integer)-1; i>=0; i-- )
    {
        octet = (UINT8)(from_host_integer >> i*8);
        
        if( bool_trim_done == BOAT_FALSE )
        {
            if( octet == 0x00 )
            {
                continue;  // Trim MSB zeros.
            }
            else
            {
                bool_trim_done = BOAT_TRUE;
            }
        }

        to_big_ptr[binary_index++] = octet;

    }

    
    if( binary_index == 0 )
    {
        to_big_ptr[0] = 0x00;
        binary_index = 1;
    }
    
   
    return binary_index;
}


/*!*****************************************************************************
@brief Convert a host endian UINT32 to bigendian

Function: Utilityhtonl()

    This function converts a host endian (typically littleendian) UINT32 integer
    to bigendian.

@see UtilityUint32ToBigend()
    

@return
    This function returns the converted bigendian UINT32 integer.
    

@param[in] from_host_integer
        The host endian UINT32 integer to convert.

*******************************************************************************/
UINT32 Utilityhtonl(UINT32 from_host_integer)
{
    UINT32 to_big_integer;

    UtilityUint32ToBigend((UINT8 *)&to_big_integer, from_host_integer, TRIMBIN_TRIM_NO);

    return to_big_integer;
}


/*!*****************************************************************************
@brief Convert a bigendian UINT32 to host endian

Function: Utilityhtonl()

    This function converts a bigendian UINT32 integer to host endian (typically
    littleendian).

@see Utilityhtonl()
    

@return
    This function returns the converted host endian UINT32 integer.
    

@param[in] from_big_integer
        The big endian UINT32 integer to convert.

*******************************************************************************/
UINT32 Utilityntohl(UINT32 from_big_integer)
{
    SINT8 i;
    UINT32 to_host_integer;

    to_host_integer = 0;
    for( i = 0; i < 4; i++ )
    {
        *((UINT8 *)&to_host_integer + i) |= (from_big_integer>>((3-i)*8))&0xFF;
    }
    return to_host_integer;
}


/*!*****************************************************************************
@brief Convert a string representing wei in HEX to ether in double float

Function: UtilityWeiStrToEthDouble()

    This function converts a string representing wei in HEX to ether in double
    float.

    1 ether is 1e18 wei. Note that in Ethereum an integer type is up to 256 bits
    while most C compilers natively support up to 64-bit integer. A 64-bit
    unsigned interger can represent up to 2^64 - 1, which is roughly 1.845e19.
    Thus it's not possible to convert more than 1.845e19 wei (i.e. 18.45 ether)
    to native integer type.

    Converting integer type wei to double float type ether loses pricision.
    Fortunately the unit of ether is usually for friendly human-reading only
    and slight pricision loss is not a problem.
    


@return
    This function returns the converted ether in double float.
    

@param[in] wei_str
        The HEX-represented string of wei, either in "0xabcd..." or "abcd..." format.

*******************************************************************************/
double UtilityWeiStrToEthDouble(const CHAR *wei_str)
{
    UINT32 wei_bin_len;
    UINT8 wei_bin[(strlen(wei_str)+1)/2];
    UINT64 wei_int64;
    UINT8 *wei_int64_addr_ptr;
    UINT32 shift_bits;
    double scale_double;
    double wei_double;
    double ether_double;
    UINT32 i;

    // Conver wei from HEX to binary with leading zeros trimmed
    wei_bin_len = UtilityHex2Bin(
                        wei_bin,
                        sizeof(wei_bin),
                        wei_str,
                        TRIMBIN_LEFTTRIM,
                        BOAT_FALSE
                      );

    // Above binary representation of wei is in bigendian and it's possibly
    // larger than UINT64 (8 bytes).
    // To convert it to littleendian, every byte in wei_bin is copied to wei_int64
    // reversely.
    //
    // If wei_bin is more than 8 bytes, only most significant 8 bytes are copied,
    // as if the integer were right shifted by some bits until its value is
    // no more than UINT64. After it's converted to double, 2^shift_bits is
    // multiplied by to recover its value.
    //
    // If wei_bin is no more than 8 bytes, only effective bytes are copied to
    // wei_int64.
        
    wei_int64 = 0;
    wei_int64_addr_ptr = (UINT8*)&wei_int64;

    if( wei_bin_len > sizeof(UINT64) )
    {
        // If wei_bin is more than 8 bytes, copy 8 bytes only
        for( i = 0; i< sizeof(UINT64); i++ )
        {
            // bigendian to littleendian
            wei_int64_addr_ptr[i] =  wei_bin[sizeof(UINT64) - 1 - i];
        }
        
        shift_bits = (wei_bin_len - sizeof(UINT64))*8;
    }
    else
    {
        // If wei_bin is no more than 8 bytes, copy effective bytes
        for( i = 0; i< wei_bin_len; i++ )
        {
            // bigendian to littleendian
            wei_int64_addr_ptr[i] =  wei_bin[wei_bin_len - 1 - i];
        }
        shift_bits = 0;
    }
    
    // Convert UINT64 to double
    wei_double = (double)wei_int64;

    // Recover shifted bits if any.
    // DO NOT calculate 2^shift_bits by (0x1 << shift_bits), because shift_bits
    // may be more than 64 bits. Calculate it in double instead.
    scale_double = 1.0;
    
    for( i = 0; i < shift_bits; i++ )
    {
        scale_double *= 2.0;
    }
    
    // Convert wei to ether by division by 1e18
    ether_double = wei_double * scale_double / 1e18;

    BoatLog(BOAT_LOG_VERBOSE, "%s wei converted to %f ether", wei_str, ether_double);
    
    return ether_double;
}


/*!*****************************************************************************
@brief Wrapper function for memory allocation

Function: BoatMalloc()

    This function is a wrapper for dynamic memory allocation.

    It typically wraps malloc() in a linux or Windows system.
    For RTOS it depends on the specification of the RTOS.


@return
    This function returns the address of the allocated memory. If allocation\n
    fails, it returns NULL.
    

@param[in] size
        How many bytes to allocate.

*******************************************************************************/
void *BoatMalloc(UINT32 size)
{
    return(malloc(size));
}


/*!*****************************************************************************
@brief Wrapper function for memory allocation

Function: BoatFree()

    This function is a wrapper for dynamic memory de-allocation.

    It typically wraps free() in a linux or Windows system.
    For RTOS it depends on the specification of the RTOS.


@see BoatMalloc()

@return
    This function doesn't return anything.
    

@param[in] mem_ptr
    The address to free. The address must be the one returned by BoatMalloc().

*******************************************************************************/
void BoatFree(void *mem_ptr)
{
    free(mem_ptr);
    return;
}




