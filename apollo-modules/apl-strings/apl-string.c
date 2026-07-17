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
