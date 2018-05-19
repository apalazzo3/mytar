#ifndef MYTAR_H
#define MYTAR_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <tar.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

/* Copy of the posix_header struct found in spec */
typedef struct {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char chksum[8];
  char typeflag;
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
} Header;

FILE* get_fp(char path[256]);
int sep_prefix_name(char path[256]);
struct Header* get_header(FILE* fp, char path[256]);

#endif
