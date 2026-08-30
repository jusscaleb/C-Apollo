/*===================================================================
                          apl-io.h

                    (c)2026 SCXRPIUS.dev

              The Apollo Input/Output runtime library.
     Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)

----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/

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
#define _DEBUG(prompt)                                                         \
  do {                                                                         \
    if (_DB)                                                                   \
      printf("%s\n", prompt);                                                  \
  } while (0);

#define _BUFFER_SIZE 4096
#define _TEMP_BUFFER_SIZE 16
extern char out_buf[_BUFFER_SIZE];
extern int buf_idx;
//extern char str_arena[_BUFFER_SIZE];
//extern int arena_offset;

#define TYPEOF(x)                                                              \
  _Generic((x),                                                                \
      int: "int",                                                              \
      float: "float",                                                          \
      double: "double",                                                        \
      char: "char",                                                            \
      char *: "string",                                                        \
      default: "unknown")

typedef struct String {
  const char *_str_;
  int _length_;
} String;

void _apl_flush();

void _apl_print_char(char c);

void _apl_print_string(const char *str);

void _apl_print_bool(int val);

void _apl_print_int(int val);

void _apl_print_newline();

void _apl_print_float(float val);

void _apl_clear_();

void __apl_input__(const char *__prompt__, String *__input__);

void __apl_print_ptr(const void *ptr);

void _apl_panic_out_of_bounds(int index, int length);

#endif