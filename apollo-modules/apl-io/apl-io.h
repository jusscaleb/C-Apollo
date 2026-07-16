/*===================================================================
                          apl-ion.h

                    (c)2026 SCXRPIUS.dev

              The Apollo Input/Output runtime library.
     Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)
     
----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/

#pragma  once
#ifndef APL_IO_H
#define APL_IO_H

#include <stdio.h>

#ifdef _WIN32 
    #include <io.h>
    #define write _write
#else
    #include <unistd.h>
#endif

#define _DB 0
#define _DEBUG(prompt) do{ if(_DB) printf("%s\n", prompt); }while(0);

#define _BUFFER_SIZE 4096
#define _TEMP_BUFFER_SIZE 16
static char out_buf[_BUFFER_SIZE];
static int buf_idx = 0;
static char str_arena[_BUFFER_SIZE];
static int arena_offset = 0;


typedef struct String{
    const char* _str_;
    int _length_;
}String;

__attribute__((always_inline)) static void _apl_flush();

__attribute__((always_inline)) static void _apl_print_char(char c);

void _apl_print_string(const char* str);

void _apl_print_bool(int val);

void _apl_print_int(int val);

void _apl_print_float(double val);

void _apl_print_newline();
void _apl_print_float(double val);

__attribute__((always_inline)) static void _apl_clear_();

String __apl_input__(const char* __prompt__);


#endif