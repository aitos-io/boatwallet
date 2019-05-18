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

/*!@brief Error catching macros and error codes

@file
boatexception.h defines error catching mechanism and error codes.
*/

#ifndef __BOATEXCEPTION_H__
#define __BOATEXCEPTION_H__

//!@{
//!@brief Declare variables used for throw...catch
#define boat_try_declare BOAT_RESULT boat_exception = BOAT_SUCCESS

//!@brief Throw an exception
//!@param e Error code.
//!@param func_catch_point The catch block name to catch the exception.
#define boat_throw(e, func_catch_point) {boat_exception = e; goto func_catch_point;}

//!@brief Catch an exception
//!@param func_catch_point The catch block name matching the one passed to boat_throw().
#define boat_catch(func_catch_point) func_catch_point: if( boat_exception != BOAT_SUCCESS )
//!@}


#define BOAT_SUCCESS 0
#define BOAT_ERROR (-1)

#define BOAT_ERROR_NULL_POINTER (-100)
#define BOAT_ERROR_OUT_OF_MEMORY (-101)
#define BOAT_ERROR_INVALID_LENGTH (-102)
#define BOAT_ERROR_INCOMPATIBLE_ARGUMENTS (-103)
#define BOAT_ERROR_RLP_ENCODING_FAIL (-104)
#define BOAT_ERROR_EXT_MODULE_OPERATION_FAIL (-105)
#define BOAT_ERROR_JSON_PARSE_FAIL (-106)
#define BOAT_ERROR_RPC_FAIL (-107)


#endif
