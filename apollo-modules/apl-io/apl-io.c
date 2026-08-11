#include "apl-io.h"

__attribute__((always_inline)) static void _apl_flush() {
  _DEBUG("API_FLUSH WORKS");
  if (buf_idx > 0) {
    write(1, out_buf, buf_idx);
  }

  _apl_clear_();
}

__attribute__((always_inline)) static void _apl_print_char(char c) {
  if (buf_idx >= _BUFFER_SIZE) {
    _apl_flush();
  }
  out_buf[buf_idx++] = c;
}

__attribute__((always_inline)) static void _apl_clear_() { buf_idx = 0; }

void _apl_print_string(const char *str) {
  if (!str)
    return;
  while (*str) {
    _apl_print_char(*str++);
  }
}

__attribute__((always_inline)) void _apl_print_bool(int val) {
  if (val) {
    _apl_print_string("true");
  } else {
    _apl_print_string("false");
  }

  _apl_flush();
}

__attribute__((always_inline)) void _apl_print_newline() {
  _apl_print_char('\n');
  _apl_flush();
}

__attribute__((always_inline)) void _apl_print_int(int val) {
  _DEBUG("INT PRINT WORKS.")

  if (val == 0) {
    _apl_print_char('0');
    return;
  }

  if (val < 0) {
    _apl_print_char('-');
    val = -val;
  }
  char temp[_TEMP_BUFFER_SIZE];
  int temp_idx = 0;
  while (val > 0) {
    temp[temp_idx++] = (val % 10) + '0';
    val /= 10;
  }

  for (int i = temp_idx - 1; i >= 0; i--) {
    _DEBUG("THIS FOR LOOP WORKS")
    _apl_print_char(temp[i]);
  }
}

__attribute__((always_inline)) void _apl_print_float(float val) {
  if (val < 0) {
    _apl_print_char('-');
    val = -val;
  }

  // Getting the integer part.
  const long long _INT_PART = (long long)val;
  _apl_print_int((int)_INT_PART);

  _apl_print_char('.');

  // Extracting decimal part.
  const float _DEC_PART = val - (float)_INT_PART;
  const long long _FRAC_PART =
      (long long)(_DEC_PART * 1000000.0 + 0.5); // 0.5 to round off.

  long long _temp = _FRAC_PART;

  int _digits = 0;

  while (_temp > 0) {
    _digits++;
    _temp /= 10;
  }

  int _padding = 6 - _digits;

  if (_FRAC_PART == 0)
    _padding = 5;

  for (int i = 0; i < _padding; i++)
    _apl_print_char('0');

  if (_FRAC_PART > 0) {
    _apl_print_int((int)_FRAC_PART);
  } else {
    _apl_print_char('0');
  }

  _apl_flush();
}

String __apl_input__(const char *__prompt__) {
  String __input__;

  _apl_print_string(__prompt__);

  char *new_buf = &str_arena[arena_offset];

  int max_readable = _BUFFER_SIZE;

  if (max_readable <= 0) {
    arena_offset = 0;
    new_buf = &str_arena[0];
    max_readable = _BUFFER_SIZE;
  }

  int length = 0;
  int c;

  while (length < max_readable) {
    c = getchar();

    if (c == EOF || c == '\n') {
      break;
    }
    new_buf[length] = (char)c;
    length++;
  }
  new_buf[length] = '\0';

  __input__._str_ = new_buf;
  __input__._length_ = length;

  arena_offset += length + 1;

  return __input__;
}
