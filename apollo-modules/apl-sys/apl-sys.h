/*===================================================================
                          apl-sys.h

                    (c)2026 SCXRPIUS.dev

              The Apollo Systems runtime library.
     Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)
     
----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/

//clang -emit-llvm -o3 -flto  -ffunction-sections -fdata-sections -DNDEBUG -c apl-sys.c -o apl-sys.bc

#ifndef APL_SYS_H
#define APL_SYS_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdint.h>
#include <stdio.h>


//++++++++++++++++++++++ PERMISSIONS ++++++++++++++++++++++++++++

#define O_RDONLY    0x0000          //Read-only.
#define O_WRONLY    0x0001          //Write-only.
#define O_RDWR      0x0002          //Read-Write.



#define O_CREAT     0x0100          //Create if exists.
#define O_TRUNC     0x0200          //Wipe file if exists.
#define O_APPEND    0x0400          //Write at end of file.
#define O_EXCL      0x0080          //Fail if file exists.


int __apl_sys_mkdir__   (const char* __path__);

int __apl_sys_delete__  (const char* __path__);

int __apl__sys_rmdir__  (const char* __path__);

int __apl_sys_rename__  (const char* _old_path_, const char* _new_path_);

void __apl_sys_exit__   (int __code__);

int __apl_sys_chdir__   (const char* __path__);

int __apl_sys_getcwd__  (char* __buf__, int size);

int __apl_sys_open__    (const char* __path__, int flags, int mode);

int __apl_sys_read__    (int fd, void* __buf__, int size);

int __apl_sys_write__   (int fd, const void* __buf__, uint32_t size);

#endif