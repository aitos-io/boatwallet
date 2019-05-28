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
#include <string.h>
#include <signal.h>


// Uncomment following line if it's on target board
// #define ON_TARGET

#ifdef ON_TARGET

// APIs defined on target board
extern int DemoEnableGPS(void);
extern int DemoDisableGPS(void);
extern char * DemoGetGPSLocation(void);

#else

// If not on target, simulate +CGPSINFO
int DemoEnableGPS(void) { return 0; }
int DemoDisableGPS(void) { return 0; }
char * DemoGetGPSLocation(void)
{
    static CHAR *g_locaton_string = "+CGPSINFO: 3109.991971,N,12122.945494,E,240519,025335.0,-10.3,8.0,337.5";
    static CHAR *g_no_locaton_string = "+CGPSINFO: ,,,,,,,,";
    static UINT8 n = 0;
    
    n++;
    
    if( n % 2 == 1 )
        return g_locaton_string;
    else
        return g_no_locaton_string;
}
#endif


static int exit_signal = 0;

// Before test this case, deploy following smart contract and replace
// contract_address with the actual deployed contract address.
// See Truffle Suite's documents for how to deploy a smart contract.
CHAR *contract_address = "0xcfeb869f69431e42cdb54a4f4f105c19c080a601";

// Smart Contract GpsTraceContract (in solidity)
/*
pragma solidity >=0.4.16 <0.6.0;

contract GpsTraceContract {
    address public organizer;

    bytes32[] eventList;

    constructor () public {
        organizer = msg.sender;
    }
    
    function saveList(bytes32 newEvent) public {
        eventList.push(newEvent);
    }
    
    function readListLength() public view returns (uint length_) {
        // ...
        length_ = eventList.length;
    }

    function readListByIndex(uint index) public view returns (bytes32 event_) { 
        // ...
        if(eventList.length > index) {
            event_ = eventList[index];
        }
    }


    function destroy() public {
        if (msg.sender == organizer) { 
            selfdestruct(organizer);
        }
    }
}
*/

BOAT_RESULT CallSaveListSol(CHAR * contract_addr_str, CHAR * string_to_save)
{
    BoatAddress recipient;
    CHAR *function_prototye_str;
    UINT8 function_selector[32];
    TxFieldVariable data;
    UINT8 data_array[36];
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
    
    // Truncate input string up to 31 characters plus '\0'
    memset(data.field_ptr+4, 0x00, 32);
    strncpy((CHAR*)data.field_ptr+4, string_to_save, 31);
   
    data.field_len = 36;
     
    result = BoatTxSetData(&data);
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;

    
    // Perform the transaction
    // NOTE: Field v,r,s are calculated automatically
    result = BoatTxSend();
    if( result != BOAT_SUCCESS ) return BOAT_ERROR;

    return BOAT_SUCCESS;
}


UINT32 CallReadListLength(CHAR * contract_addr_str)
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


BOAT_RESULT CallReadListByIndex(CHAR * contract_addr_str, UINT32 list_len)
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
            CHAR event_string[32];
            UtilityHex2Bin(
                        (UINT8*)event_string,
                        32,
                        retval_str,
                        TRIMBIN_TRIM_NO,
                        BOAT_FALSE
                      );
            BoatLog(BOAT_LOG_NORMAL, "%s", event_string);
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


//+CGPSINFO:[<lat>],[<N/S>],[<log>],[<E/W>],[<date>],[<UTCtime>],[<alt>],[<speed>],[<course>]
typedef struct TStringWithLen
{
    CHAR *string_ptr;
    UINT32 string_len;
}StringWithLen;

typedef struct TCgpsinfo
{
    StringWithLen lat;
    StringWithLen ns;
    StringWithLen log;
    StringWithLen ew;
    StringWithLen date;
    StringWithLen utctime;
    StringWithLen alt;
    StringWithLen speed;
    StringWithLen course;
}Cgpsinfo;

BOAT_RESULT ParseCGPSINFO(const CHAR *cgpsinfo_str, BOAT_OUT Cgpsinfo *parsed_gpsinfo_ptr)
{
    CHAR *cgpsinfo_pos_ptr;
    CHAR *field_start_pos_ptr;
    CHAR c;
    UINT32 field_len;
    
    if( cgpsinfo_str == NULL || parsed_gpsinfo_ptr == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Arguments cannot be NULL.");
        return BOAT_ERROR;
    }

    // Check if the string contains "+CGPSINFO:"
    cgpsinfo_pos_ptr = strstr(cgpsinfo_str, "+CGPSINFO:");
    if( cgpsinfo_pos_ptr == NULL )
    {
        BoatLog(BOAT_LOG_NORMAL, "Unable to find \"+CGPSINFO:\" in string.");
        return BOAT_ERROR;
    }
    
    cgpsinfo_pos_ptr = cgpsinfo_pos_ptr + strlen("+CGPSINFO:");
    
    // Skip space
    while( (c = *cgpsinfo_pos_ptr) == ' ' )
    {
        cgpsinfo_pos_ptr++;
    }

   
    // Extract every field

    // Extract <lat>
    field_start_pos_ptr = cgpsinfo_pos_ptr;
    
    while( (c = *cgpsinfo_pos_ptr) != '\0' )
    {
        if( c == ',' )     break;  // Note: cgpsinfo_pos_ptr points to ',' when loop exits
        cgpsinfo_pos_ptr++;
    }
    
    field_len = (UINT32)(cgpsinfo_pos_ptr - field_start_pos_ptr);
    if( field_len != 0 )
    {
        parsed_gpsinfo_ptr->lat.string_ptr = field_start_pos_ptr;
        parsed_gpsinfo_ptr->lat.string_len = field_len;
        cgpsinfo_pos_ptr++;
    }
    else
    {
        parsed_gpsinfo_ptr->lat.string_ptr = NULL;
        parsed_gpsinfo_ptr->lat.string_len = 0;
        
        if( c != '\0' )     cgpsinfo_pos_ptr++;
    }


    // Extract <N/S>
    field_start_pos_ptr = cgpsinfo_pos_ptr;
    
    while( (c = *cgpsinfo_pos_ptr) != '\0' )
    {
        if( c == ',' )     break;  // Note: cgpsinfo_pos_ptr points to ',' when loop exits
        cgpsinfo_pos_ptr++;
    }
    
    field_len = (UINT32)(cgpsinfo_pos_ptr - field_start_pos_ptr);
    if( field_len != 0 )
    {
        parsed_gpsinfo_ptr->ns.string_ptr = field_start_pos_ptr;
        parsed_gpsinfo_ptr->ns.string_len = field_len;
        cgpsinfo_pos_ptr++;
    }
    else
    {
        parsed_gpsinfo_ptr->ns.string_ptr = NULL;
        parsed_gpsinfo_ptr->ns.string_len = 0;
        
        if( c != '\0' )     cgpsinfo_pos_ptr++;
    }

    
    // Extract <log>
    field_start_pos_ptr = cgpsinfo_pos_ptr;
    
    while( (c = *cgpsinfo_pos_ptr) != '\0' )
    {
        if( c == ',' )     break;  // Note: cgpsinfo_pos_ptr points to ',' when loop exits
        cgpsinfo_pos_ptr++;
    }
    
    field_len = (UINT32)(cgpsinfo_pos_ptr - field_start_pos_ptr);
    if( field_len != 0 )
    {
        parsed_gpsinfo_ptr->log.string_ptr = field_start_pos_ptr;
        parsed_gpsinfo_ptr->log.string_len = field_len;
        cgpsinfo_pos_ptr++;
    }
    else
    {
        parsed_gpsinfo_ptr->log.string_ptr = NULL;
        parsed_gpsinfo_ptr->log.string_len = 0;
        
        if( c != '\0' )     cgpsinfo_pos_ptr++;
    }


    // Extract <E/W>
    field_start_pos_ptr = cgpsinfo_pos_ptr;
    
    while( (c = *cgpsinfo_pos_ptr) != '\0' )
    {
        if( c == ',' )     break;  // Note: cgpsinfo_pos_ptr points to ',' when loop exits
        cgpsinfo_pos_ptr++;
    }
    
    field_len = (UINT32)(cgpsinfo_pos_ptr - field_start_pos_ptr);
    if( field_len != 0 )
    {
        parsed_gpsinfo_ptr->ew.string_ptr = field_start_pos_ptr;
        parsed_gpsinfo_ptr->ew.string_len = field_len;
        cgpsinfo_pos_ptr++;
    }
    else
    {
        parsed_gpsinfo_ptr->ew.string_ptr = NULL;
        parsed_gpsinfo_ptr->ew.string_len = 0;
        
        if( c != '\0' )     cgpsinfo_pos_ptr++;
    }


    // Extract <date>
    field_start_pos_ptr = cgpsinfo_pos_ptr;
    
    while( (c = *cgpsinfo_pos_ptr) != '\0' )
    {
        if( c == ',' )     break;  // Note: cgpsinfo_pos_ptr points to ',' when loop exits
        cgpsinfo_pos_ptr++;
    }
    
    field_len = (UINT32)(cgpsinfo_pos_ptr - field_start_pos_ptr);
    if( field_len != 0 )
    {
        parsed_gpsinfo_ptr->date.string_ptr = field_start_pos_ptr;
        parsed_gpsinfo_ptr->date.string_len = field_len;
        cgpsinfo_pos_ptr++;
    }
    else
    {
        parsed_gpsinfo_ptr->date.string_ptr = NULL;
        parsed_gpsinfo_ptr->date.string_len = 0;
        
        if( c != '\0' )     cgpsinfo_pos_ptr++;
    }


    // Extract <UTCtime>
    field_start_pos_ptr = cgpsinfo_pos_ptr;
    
    while( (c = *cgpsinfo_pos_ptr) != '\0' )
    {
        if( c == ',' )     break;  // Note: cgpsinfo_pos_ptr points to ',' when loop exits
        cgpsinfo_pos_ptr++;
    }
    
    field_len = (UINT32)(cgpsinfo_pos_ptr - field_start_pos_ptr);
    if( field_len != 0 )
    {
        parsed_gpsinfo_ptr->utctime.string_ptr = field_start_pos_ptr;
        parsed_gpsinfo_ptr->utctime.string_len = field_len;
        cgpsinfo_pos_ptr++;
    }
    else
    {
        parsed_gpsinfo_ptr->utctime.string_ptr = NULL;
        parsed_gpsinfo_ptr->utctime.string_len = 0;
        
        if( c != '\0' )     cgpsinfo_pos_ptr++;
    }


    // Extract <alt>
    field_start_pos_ptr = cgpsinfo_pos_ptr;
    
    while( (c = *cgpsinfo_pos_ptr) != '\0' )
    {
        if( c == ',' )     break;  // Note: cgpsinfo_pos_ptr points to ',' when loop exits
        cgpsinfo_pos_ptr++;
    }
    
    field_len = (UINT32)(cgpsinfo_pos_ptr - field_start_pos_ptr);
    if( field_len != 0 )
    {
        parsed_gpsinfo_ptr->alt.string_ptr = field_start_pos_ptr;
        parsed_gpsinfo_ptr->alt.string_len = field_len;
        cgpsinfo_pos_ptr++;
    }
    else
    {
        parsed_gpsinfo_ptr->alt.string_ptr = NULL;
        parsed_gpsinfo_ptr->alt.string_len = 0;
        
        if( c != '\0' )     cgpsinfo_pos_ptr++;
    }


    // Extract <speed>
    field_start_pos_ptr = cgpsinfo_pos_ptr;
    
    while( (c = *cgpsinfo_pos_ptr) != '\0' )
    {
        if( c == ',' )     break;  // Note: cgpsinfo_pos_ptr points to ',' when loop exits
        cgpsinfo_pos_ptr++;
    }
    
    field_len = (UINT32)(cgpsinfo_pos_ptr - field_start_pos_ptr);
    if( field_len != 0 )
    {
        parsed_gpsinfo_ptr->speed.string_ptr = field_start_pos_ptr;
        parsed_gpsinfo_ptr->speed.string_len = field_len;
        cgpsinfo_pos_ptr++;
    }
    else
    {
        parsed_gpsinfo_ptr->speed.string_ptr = NULL;
        parsed_gpsinfo_ptr->speed.string_len = 0;
        
        if( c != '\0' )     cgpsinfo_pos_ptr++;
    }


    // Extract <course>
    field_start_pos_ptr = cgpsinfo_pos_ptr;
    
    while( (c = *cgpsinfo_pos_ptr) != '\0' )
    {
        if( c == ',' )     break;  // Note: cgpsinfo_pos_ptr points to ',' when loop exits
        cgpsinfo_pos_ptr++;
    }
    
    field_len = (UINT32)(cgpsinfo_pos_ptr - field_start_pos_ptr);
    if( field_len != 0 )
    {
        parsed_gpsinfo_ptr->course.string_ptr = field_start_pos_ptr;
        parsed_gpsinfo_ptr->course.string_len = field_len;
        cgpsinfo_pos_ptr++;
    }
    else
    {
        parsed_gpsinfo_ptr->course.string_ptr = NULL;
        parsed_gpsinfo_ptr->course.string_len = 0;
        
        if( c != '\0' )     cgpsinfo_pos_ptr++;
    }
    
    return BOAT_SUCCESS;
}





//CTRL-C:exit main process.
static void handle_signal(int signum)
{
    exit_signal = 1;
}



BOAT_RESULT CaseGpsTraceMain(void)
{
    UINT32 list_len;
    BOAT_RESULT result;
    UINT32 n;
    CHAR *gps_location_ptr;
    CHAR truncated_gps_location_str[32];
    

    //signal-CTRL-C:exit main process.
    exit_signal = 0;
    signal(SIGINT, handle_signal);  

    DemoEnableGPS();
   

    BoatLog(BOAT_LOG_NORMAL, "====== Testing CaseGpsTrace ======");

    // Capture 10 location records
    for( n = 0; n < 10; n++ )
    {

        UINT32 k;
        Cgpsinfo parsed_gpsinfo;
        UINT32 location_string_len;
        
        for( k = 0; k < 30; k++ )
        {
            if( exit_signal == 1 )  goto CaseGpsTraceMain_destruct;
            #ifdef ON_TARGET
            sleep(1);
            #endif
        }
        
        // DemoGetGPSLocation() returns a string current GPS information, either
        // something like "+CGPSINFO: 3109.991971,N,12122.945494,E,240519,025335.0,-10.3,8.0,337.5"
        // or "+CGPSINFO: ,,,,,,,," if GPS is out of coverage
        gps_location_ptr = DemoGetGPSLocation();
        if( gps_location_ptr == NULL ) goto CaseGpsTraceMain_destruct;

        result = ParseCGPSINFO(gps_location_ptr, &parsed_gpsinfo);
        if( result != BOAT_SUCCESS ) goto CaseGpsTraceMain_destruct;

        // Save the location (first 4 fields in GPS information) to contract
        if(   parsed_gpsinfo.lat.string_len
            + parsed_gpsinfo.ns.string_len
            + parsed_gpsinfo.log.string_len
            + parsed_gpsinfo.ew.string_len
            + 3 < 32 )  // +3 for three ','s
        {
            location_string_len = 0;
            
            // <lat>
            memcpy( truncated_gps_location_str + location_string_len,
                    parsed_gpsinfo.lat.string_ptr,
                    parsed_gpsinfo.lat.string_len);

            location_string_len += parsed_gpsinfo.lat.string_len;
            truncated_gps_location_str[location_string_len++] = ',';
            
            // <N/S>
            memcpy( truncated_gps_location_str + location_string_len,
                    parsed_gpsinfo.ns.string_ptr,
                    parsed_gpsinfo.ns.string_len);

            location_string_len += parsed_gpsinfo.ns.string_len;
            truncated_gps_location_str[location_string_len++] = ',';

            // <log>
            memcpy( truncated_gps_location_str + location_string_len,
                    parsed_gpsinfo.log.string_ptr,
                    parsed_gpsinfo.log.string_len);

            location_string_len += parsed_gpsinfo.log.string_len;
            truncated_gps_location_str[location_string_len++] = ',';

            // <E/W>
            memcpy( truncated_gps_location_str + location_string_len,
                    parsed_gpsinfo.ew.string_ptr,
                    parsed_gpsinfo.ew.string_len);

            location_string_len += parsed_gpsinfo.ew.string_len;
            truncated_gps_location_str[location_string_len++] = '\0';
                        
        }
        
        // Check for "+CGPSINFO: ,,,,,,,,", i.e. unable to obtain location due
        // to loss of GPS coverage, ignore it
        if(   parsed_gpsinfo.lat.string_len
            + parsed_gpsinfo.ns.string_len
            + parsed_gpsinfo.log.string_len
            + parsed_gpsinfo.ew.string_len
              == 0 )
        {
            BoatLog(BOAT_LOG_NORMAL, "Out of GPS coverage, ignore.");
            continue;
        }
      
        result = CallSaveListSol(contract_address, truncated_gps_location_str);
        if( result != BOAT_SUCCESS ) goto CaseGpsTraceMain_destruct;
    }

        
    // Read how many records are there in the contract
    list_len = CallReadListLength(contract_address);
    if( list_len == 0 ) goto CaseGpsTraceMain_destruct;

    // Read all records out
    result = CallReadListByIndex(contract_address, list_len);
    if( result != BOAT_SUCCESS ) goto CaseGpsTraceMain_destruct;   

CaseGpsTraceMain_destruct:

    DemoDisableGPS();
    
    return BOAT_SUCCESS;
}
