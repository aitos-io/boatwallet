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

/*!@brief Boatwallet options

@file
boatoptions.h defines options for compiling.
*/

#ifndef __BOATOPTIONS_H__
#define __BOATOPTIONS_H__



// BOAT LOG LEVEL DEFINITION
// Log level is used to control the detail of log output.
// 3 types of detail level can be specified in BoatLog():
// "CRITICAL" is for critical exceptions that may halt the wallet from going on.
// "NORMAL" is for warnings that may imply some error but could be held.
// "VERBOSE" is for detail information that is only useful for debug.
#define BOAT_LOG_NONE        0
#define BOAT_LOG_CRITICAL    1
#define BOAT_LOG_NORMAL      2
#define BOAT_LOG_VERBOSE     3

// BOAT_LOG_LEVEL is a macro that limits the log detail up to that level.
// Seting it to BOAT_LOG_NONE means outputing nothing.
#define BOAT_LOG_LEVEL BOAT_LOG_NORMAL


// OpenSSL OPTION: Use OpenSSL for key generation
#define BOAT_USE_OPENSSL 1



// RPC USE OPTION: One and only one RPC_USE option shall be set to 1
#define RPC_USE_LIBCURL 1
#define RPC_USE_NOTHING 0

#define RPC_USE_COUNT (RPC_USE_LIBCURL + RPC_USE_NOTHING)
#if RPC_USE_COUNT != 1
#error "One and only one RPC_USE option shall be set to 1"
#endif
#undef RPC_USE_COUNT


// Mining interval and Pending transaction timeout
#define BOAT_MINE_INTERVAL 3  // Mining Interval of the blockchain, in seconds
#define BOAT_WAIT_PENDING_TX_TIMEOUT 30  // Timeout waiting for a transaction being mined, in seconds

#endif
