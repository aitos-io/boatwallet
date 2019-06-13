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

/*!@brief Header file for utility functions

@file
utility.h is header file for boatwallet utility functions.
*/

#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "wallet/boattypes.h"

//!@brief Argument type for UtilityTrimBin(), UtilityHex2Bin() and UtilityUint32ToBigend()
typedef enum
{
    TRIMBIN_TRIM_NO,    //!< Don't trim zeros
    TRIMBIN_LEFTTRIM    //!< Trim leading or MSB zeros
}TRIMBIN_TRIM_MODE;

//!@brief Argument type for UtilityHex2Bin()
typedef enum
{
    BIN2HEX_TRIM_NO = 0,        //!< Don't trim zeros
    BIN2HEX_LEFTTRIM_QUANTITY,  //!< Trim {0x00, 0x01, 0x00 0xAB} => "0x100AB" or "100AB"
    BIN2HEX_LEFTTRIM_UFMTDATA   //!< Trim {0x00, 0x01, 0x00 0xAB} => "0x0100AB" or "0100AB"
}BIN2HEX_TRIM_MODE;

//!@brief Argument type for UtilityBin2Hex()
typedef enum
{
    BIN2HEX_PREFIX_0x_NO = 0,   //<! Prepend "0x" to converted HEX string
    BIN2HEX_PREFIX_0x_YES       //<! Don't prepend "0x" to converted HEX string
}BIN2HEX_PREFIX_0x_MODE;



extern const CHAR * const g_log_level_name_str[];

#if BOAT_LOG_LEVEL == BOAT_LOG_NONE
/*!@brief Log Output

@param level
    Log priority level. One of BOAT_LOG_CRITICAL, BOAT_LOG_NORMAL or BOAT_LOG_VERBOSE.

@param format
    Similar to that in printf().
*/
#define BoatLog(level, format,...)
#else
#define BoatLog(level, format,...)\
    do{\
        if( level <= BOAT_LOG_LEVEL ) {printf("%s: "__FILE__":%d, %s(): "format"\n", g_log_level_name_str[level-1], __LINE__, __func__, ##__VA_ARGS__);}\
    }while(0)
#endif


/*!@brief Round value up to the nearest multiple of step

@param value
    The value to round up

@param step
    The step of round
*/
#define ROUNDUP(value, step) ((((value) - 1)/(step) + 1) * (step))


#ifdef __cplusplus
extern "C" {
#endif


UINT32 UtilityTrimBin(
                BOAT_OUT UINT8 *to_ptr,
                const UINT8 *from_ptr,
                UINT32 from_len,
                TRIMBIN_TRIM_MODE trim_mode,
                BOATBOOL zero_as_null
                );

UINT32 UtilityBin2Hex(
                BOAT_OUT CHAR *to_str,
                const UINT8 *from_ptr,
                UINT32 from_len,
                BIN2HEX_TRIM_MODE trim_mode,
                BIN2HEX_PREFIX_0x_MODE prefix_0x_mode,
                BOATBOOL zero_as_null
                );

UINT32 UtilityHex2Bin(
                        BOAT_OUT UINT8 *to_ptr,
                        UINT32 to_size,
                        const CHAR *from_str,
                        TRIMBIN_TRIM_MODE trim_mode,
                        BOATBOOL zero_as_null
                      );

UINT8 UtilityUint32ToBigend(
                BOAT_OUT UINT8 *to_big_ptr,
                UINT32 from_host_integer,
                TRIMBIN_TRIM_MODE trim_mode
                );

UINT8 UtilityUint64ToBigend(
                BOAT_OUT UINT8 *to_big_ptr,
                UINT64 from_host_integer,
                TRIMBIN_TRIM_MODE trim_mode
                );

UINT32 Utilityhtonl(UINT32 from_host_integer);

UINT32 Utilityntohl(UINT32 from_host_integer);

double UtilityWeiStrToEthDouble(const CHAR *wei_str);

void *BoatMalloc(UINT32 size);
void BoatFree(void *mem_ptr);




#ifdef __cplusplus
}
#endif /* end of __cplusplus */

#endif
