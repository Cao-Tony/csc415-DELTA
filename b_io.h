/**************************************************************
* Class:  CSC-415 
* Name: Tony Cao
* Student ID: 920171613
* GitHub Handle:  Cao-Tony
* Project: File System
*
* File: b_io.h
*
* Description: Interface of I/O functions
**************************************************************/
#ifndef _B_IO_H
#define _B_IO_H
#include <fcntl.h>

typedef int b_io_fd;

int b_open (char * filename, int flags);
int b_read (int fd, char * buffer, int count);
int b_write (int fd, char * buffer, int count);
int b_seek (int fd, off_t offset, int whence);
void b_close (int fd);

#endif