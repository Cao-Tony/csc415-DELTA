/**************************************************************
* Class:  CSC-415 
* Name: Tony Cao
* Student ID: 920171613
* GitHub Handle:  Cao-Tony
* Project: File System
*
* File: mfs.h
*
* Description: file system interface needed by the driver to 
* interact with your filesystem.
**************************************************************/
#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "b_io.h"

#include <dirent.h>
#define FT_REGFILE DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK DT_LNK

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

#define MAXFILEQUE 10
#define NUMBEROFENTRIESALLOWED 20
#define FREE 0
#define DIR 1
#define FILE 2

struct fs_diriteminfo
{
	unsigned short d_reclen; 
	unsigned char fileType;
	char d_name[256]; 
};

typedef struct fdDir
{
	char name[20];	 				
	int fileType;					 
	int size;						
	uint64_t directoryStartLocation; 
	unsigned short d_reclen;		 
	unsigned short dirEntryPosition; 
} fdDir;

int fs_mkdir(const char *pathName, mode_t mode);
int fs_rmdir(const char *pathname);
fdDir *fs_opendir(const char *name);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

char *fs_getcwd(char *buf, size_t size);
int fs_setcwd(char *buf);	   
int fs_isFile(char *path);	   
int fs_isDir(char *path);	   
int fs_delete(char *filename); 

int fs_initRoot();
int fs_loadRoot();
int mfs_shutdown();

int fs_openFile(char*, int);
int fs_closeFile(int);
int fs_writeFile(int, char*, int);
int fs_readFile(int, char*, int);
int fs_lSeek(int fd, int locationInBytes);
int fs_moveDirEntry(char *name, char *path); 
struct fs_stat
{
	off_t st_size;		  /* total size, in bytes */
	blksize_t st_blksize; /* blocksize for file system I/O */
	blkcnt_t st_blocks;	  /* number of 512B blocks allocated */
	time_t st_accesstime; /* time of last access */
	time_t st_modtime;	  /* time of last modification */
	time_t st_createtime; /* time of last status change */

	uint64_t EntryArray[NUMBEROFENTRIESALLOWED];
};

typedef struct fileQue
{
	struct fs_stat fileStats;
	struct fdDir fileDir;
	int currentLocationInFile; 
	int buffIndex;
	int ocupado;
	char* buff;
	int flag;
	uint64_t parentLBA; 
} fileQue;

int fs_stat(const char *path, struct fs_stat *buf);

#endif