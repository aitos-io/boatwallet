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

/*!@brief Random Number Generator for Qualcomm 9x07

@file
randgenerate.c is the random number generator.
*/

#include "wallet/boatwallet.h"

#if BOAT_USE_OPENSSL == 1

#include <stdio.h>
#include <openssl/rand.h>
#include "randgenerator.h"

#else

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "randgenerator.h"
#include "sha3.h"

#endif


#if BOAT_USE_OPENSSL == 1

// If OpenSSL is used, seed is automatically initialized by OpenSSL
void rand_seed_init(void)
{
    return;
}

BOAT_RESULT random_stream(UINT8 *rand_buf, UINT16 len)
{
    int rand_status;
    BOAT_RESULT result = BOAT_ERROR;
    
    rand_status = RAND_status();
    
    if( rand_status == 1 )
    {
        rand_status = RAND_bytes(rand_buf, len);
        
        if( rand_status == 1 )
        {
            result = BOAT_SUCCESS;
        }
    }
    
    
    if( result == BOAT_ERROR )
    {
        BoatLog(BOAT_LOG_CRITICAL, "Fail to generate random number.");
    }
    else
    {
        #ifdef DEBUG_LOG
        do
        {
            UINT32 i;
            printf("Rand: ");
            for( i = 0; i < len; i++ )
            {
                printf("%02x", rand_buf[i]);
            }
            putchar('\n');
        }while(0);
        #endif
    }
    
    return result;
}

UINT32 random32(void)
{
    BOAT_RESULT result;
    UINT32 rand_bytes;
    
    result = random_stream((UINT8 *)&rand_bytes, 4);
    
    if( result != BOAT_SUCCESS )
    {
        rand_bytes = 0;
    }
    
    return rand_bytes;
}


UINT64 random64(void)
{
    BOAT_RESULT result;
    UINT64 rand_bytes;
    
    result = random_stream((UINT8 *)&rand_bytes, 8);
    
    if( result != BOAT_SUCCESS )
    {
        rand_bytes = 0;
    }
    
    return rand_bytes;
}


#else // else of #if BOAT_USE_OPENSSL == 1


UINT8 * read_random_sdram(UINT32 len)
{
#ifdef DEBUG_LOG
    UINT32 i;
#endif
    size_t current_time;
    UINT8 *random_sdram_ptr;
    UINT8 *ret_val;
    UINT8 time_digest[32];
    
    current_time = time(NULL);
    keccak_256((unsigned char *)&current_time, sizeof(size_t), time_digest);
    
    // Allocate extra 256 bytes so an random offset can be given by current time
    random_sdram_ptr = malloc(len+256);

    if( random_sdram_ptr != NULL )
    {
        // Return a freed buffer increase the randomness
        free(random_sdram_ptr);
        
        // Give an offset related to current time to improve randomness
        ret_val = random_sdram_ptr + time_digest[0];
    }
    else
    {
        // If malloc() fails (possibly due to out of memory), return an address
        // in the stack. The content of stack is typically random.
        ret_val = (UINT8*)&ret_val  + time_digest[0];
    }
    
#ifdef DEBUG_LOG
        printf("read_random_sdram() returns:\n");
        for( i = 0; i < len; i++ )
        {
            printf("%02x", ret_val[i]);
        }
        putchar('\n');
#endif

        return(ret_val);

}


void rand_seed_init(void)
{
    size_t current_time;
    UINT8 time_digest[32];
    UINT8 sdram_digest[32];
    volatile UINT32 seed;
    
    // Random Seed Source 1: Current Time
    // NOTE: return type of time() is 64-bit or 32-bit depending on the system
    current_time = time(NULL);
    keccak_256((UINT8 *)&current_time, sizeof(size_t), time_digest);
    
    // Random Seed Source 2: Content of random address in SDRAM
    keccak_256(read_random_sdram(256),
               256,
               sdram_digest);
    
    // Seed = uninitialized "seed" xor first 4 bytes of time_digest xor first 4 bytes of sdram_digest
    seed ^= *(UINT32 *)time_digest ^ *(UINT32 *)sdram_digest;
    
    #ifdef DEBUG_LOG
        printf("rand_seed_init(): Seed is: %08x.\n", seed);
    #endif
    
	srand(seed);
	
    return;
}


UINT32 random32(void)
{
	return ((rand() & 0xFF) | ((rand() & 0xFF) << 8) | ((rand() & 0xFF) << 16) | ((UINT32) (rand() & 0xFF) << 24));
}


UINT64 random64(void)
{
    UINT64 rand64;
    
    *(UINT32 *)&rand64 = random32();
    *(((UINT32 *)&rand64) + 1) = random32();
    
	return (rand64);
}


BOAT_RESULT random_stream(UINT8 *buf, UINT16 len)
{
	UINT32 r = 0;
	for (UINT32 i = 0; i < len; i++)
	{
		if (i % 4 == 0)
		{
			r = random32();
		}
		buf[i] = (r >> ((i % 4) * 8)) & 0xFF;
	}

	return BOAT_SUCCESS;
}


#endif // else of #if BOAT_USE_OPENSSL == 1