#include "apl-sys.h"







//++++++++++++++++++++++++++++++++++++++++++++ FILE SYSTEM +++++++++++++++++++++++++++++++++++++++++++++
int __apl_sys_mkdir__(const char* __path__){
    if(CreateDirectoryA(__path__, NULL)){
        return 0;
    }

    return 1;
}

int __apl__sys_rmdir__(const char *__path__){
    return RemoveDirectoryA(__path__) ? 0:-1;
}

int __apl_sys_delete__(const char *__path__){
    return (DeleteFileA(__path__)) ? 0 : -1;
}


int __apl_sys_rename__(const char *_old_path_, const char *_new_path_){
    return (rename(_old_path_, _new_path_)) ? 0 : -1;
}


//++++++++++++++++++++++++++++++++++++++++++++ Process/Control +++++++++++++++++++++++++++++++++++++++++++++
void __apl_sys_exit__(int __code__){
    ExitProcess(__code__);
    
}

int __apl_sys_chdir__(const char* __path__){
    return SetCurrentDirectory(__path__) ? 0 : -1;
}


int __apl_sys_getcwd__(char *__buf__, int size){
        uint32_t len = GetCurrentDirectory(size, __buf__);
        return (len > 0 && len < (uint32_t)size) ? 0 : -1;
}


//++++++++++++++++++++++++++++++++++++++++++++ File I/O +++++++++++++++++++++++++++++++++++++++++++++
int __apl_sys_open__(const char *__path__, int flags, int mode){
   uint32_t access = 0;
   uint32_t creation = OPEN_EXISTING;
   uint32_t attrs = FILE_ATTRIBUTE_NORMAL;


   if(flags & O_RDONLY) access |= GENERIC_READ;
   if(flags & O_WRONLY) access |= GENERIC_WRITE;
   if(flags & O_RDWR  ) access |= GENERIC_READ | GENERIC_WRITE;
   if(flags & O_CREAT ) access |= OPEN_ALWAYS;
   if(flags & O_TRUNC ) access |= CREATE_ALWAYS;
   if(flags & O_APPEND) access |= FILE_APPEND_DATA;

   HANDLE h = CreateFileA(__path__, access, FILE_SHARE_READ, NULL, creation, attrs, NULL);

   return (h == INVALID_HANDLE_VALUE) ? -1 : (int)(intptr_t) h;
}


int __apl_sys_read__(int fd, void *__buf__, int size){
    HANDLE h = (HANDLE)(int)fd;
    DWORD read_bytes = 0;

    if(!ReadFile(h, __buf__, size, &read_bytes, NULL)) return -1;

    return (int)read_bytes; //0 ==> EOF.
}

int __apl_sys_write__(int fd, const void *__buf__, uint32_t size){
    HANDLE h = (HANDLE)(int)fd;

    DWORD bytes_written = 0;

    if(!WriteFile(h, __buf__, size, &bytes_written, NULL)) return -1;

    return (int)bytes_written;
}