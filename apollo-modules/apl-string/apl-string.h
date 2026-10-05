/*===================================================================
                    apl-string.h(v2.0.0)

                    (c)2026 SCXRPIUS.dev

              The Apollo string runtime library.
     Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)
     
----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/

#ifndef APL_STRING_H
#define APL_STRING_H

#include <stdio.h>
#include <string.h> 

#ifdef _WIN32 //Running on Windows.
    #include <io.h>
    #define write _write
#else
    #include <unistd.h>
#endif

#define _DB 0
#define _DEBUG(prompt) do{ if(_DB) printf("%s\n", prompt); }while(0);

#define _BUFFER_SIZE 65536
char str_arena[_BUFFER_SIZE];
int arena_offset = 0;

typedef struct String{
    const char* _str_;
    int _length_;
}String;


void __apl_create_str__(String *__meta_dt__, const char *__str__);
int __apl_get_length__(const char *__str__);


int __apl_compare_str__(String *_str_1, String *_str_2);

void __apl_concat_str__(String *out, String *_str_1, String *__str_2);

void __apl_copy_str__(String *out, String *_str_1, String *__str_2);

int __apl_index__of__(String *__str__, char _char_);

char __apl_char_at__(String *__str__, int _index_);


void __apl_concat_str_int(String *out, String *_str_1, int _int_);

void __apl_concat_str_bool(String *out, String *_str_1, int _bool_);

void __apl_concat_str_float(String *out, String *_str_1, float _float_);


#endif