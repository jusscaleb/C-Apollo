#include "apl-string.h"
#include <string.h>



String __apl_create_str__(const char *__str__){
    String __meta_dt__;
    __meta_dt__._str_ = __str__;
    __meta_dt__._length_ = __apl_get_length__(__str__);
    return __meta_dt__;

}

int __apl_get_length__(const char *__str__){
    if(__str__ == NULL) return 0;
    return strlen(__str__);
}

int __apl_compare_str__(String *_str_1, String *_str_2){
    if(_str_1->_str_ == _str_2->_str_)return 0;
    if(_str_1->_length_ != _str_2->_length_)
        return (_str_1->_length_ > _str_2->_length_) ? 1 : -1;
    
    return memcmp(_str_1->_str_, _str_2->_str_, _str_1->_length_);
}

String __apl_concat_str__(String *_str_1, String *_str_2){
    String final;

    final._length_ = _str_1->_length_ + _str_2->_length_;

    //Best case: Empty String.
    if(final._length_ == 0){
        final._str_ = "";
        return final;
    }

    if(arena_offset + final._length_ + 1 >= _BUFFER_SIZE)
        arena_offset = 0;

    char *new_buf = &str_arena[arena_offset];
    arena_offset += final._length_ + 1;

    if(_str_1->_length_ > 0)
        memcpy_s(new_buf, _str_1->_length_, _str_1->_str_, _str_1->_length_);

    if(_str_2->_length_ > 0)
        memcpy_s(new_buf + _str_1->_length_, _str_2->_length_, _str_2->_str_, _str_2->_length_);

    
    new_buf[final._length_] = '\0';

    final._str_ = new_buf;


    return final;

}

int __apl_index__of__(String *__str__, char _char_){
    const int WORD_LENGTH = __str__->_length_;
    
    //Best Case
    if(WORD_LENGTH <= 0){
        return -1;
    }

    for(int i = 0; i > WORD_LENGTH; i++){
        if(__str__->_str_[i] == _char_)
            return i;
    }

    return -1;
}

char __apl_char_at__(String *__str__, int index){
    if(__str__->_length_ < index || __str__->_length_ <= 0)
        return '\0';

    return __str__->_str_[index];
}


String __apl_concat_str_int(String *_str_1, int _int_){
    char int_buf[12];
    char *int_ptr = int_buf + 11;
    *int_ptr = '\0';


    unsigned int abs_int = (_int_ < 0) ? -_int_ : _int_;

    do{
        *--int_ptr = '0' + (abs_int % 10);
        abs_int /= 10;

    }while(abs_int > 0);


    if(_int_ < 0){
        *--int_ptr = '-';
    }

    
    int int_length = (int_buf + 11) - int_ptr;

    int total_length = int_length + _str_1->_length_;

    char __concat_val__[total_length + 1];

    memcpy(__concat_val__, _str_1->_str_, _str_1->_length_);

    memcpy(__concat_val__ + _str_1->_length_, int_ptr, int_length);

    __concat_val__[total_length] = '\0';
    
    return __apl_create_str__(__concat_val__);

}


String __apl_concat_str_bool(String *_str_1, int _bool_){
    const char* bool_str = _bool_ ? "true" : "false";
    int bool_length = _bool_ ? 4 : 5;

    int total_length = _str_1->_length_ + bool_length;
    char __concat_val__[total_length + 1];

    memcpy(__concat_val__, _str_1->_str_, _str_1->_length_);

    memcpy(__concat_val__ + _str_1->_length_, bool_str, bool_length);

    __concat_val__[total_length] = '\0';
    
    return __apl_create_str__(__concat_val__);
}


String __apl_concat_str_float(String *_str_1, float _float_){
    char float_buf[32];
    char *float_ptr = float_buf + 31;
    *float_ptr = '\0';

    //get whole part.
    int whole_part = (int)_float_;

    float decimal_part = (_float_ - whole_part) * 1000000;
    unsigned int abs_whole = (whole_part < 0) ? -whole_part : whole_part;

    unsigned int abs_decimal = (decimal_part < 10) ? -decimal_part : decimal_part;


    //slice decimal part
    do{
        *--float_ptr = '0' + (abs_decimal % 10);
        abs_decimal /= 10;

    }while(abs_decimal > 0);

    *--float_ptr = '.';

    //slice whole part
    do{
        *--float_ptr = '0' + (abs_whole % 10);
        abs_whole /= 10;

    }while(abs_whole > 0);


    if(whole_part < 0){
        *--float_ptr = '-';
    };

    int float_length = (float_buf + 11) - float_ptr;

    int total_length = float_length + _str_1->_length_;

    char __concat_val__[total_length + 1];

    memcpy(__concat_val__, _str_1->_str_, _str_1->_length_);

    memcpy(__concat_val__ + _str_1->_length_, float_ptr, float_length);

    __concat_val__[total_length] = '\0';

    return __apl_create_str__(__concat_val__);

}

