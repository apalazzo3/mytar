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
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <stdint.h>
#include <arpa/inet.h>

/* Copy of the posix_header struct found in spec */
typedef struct {
   char name[100];
   char mode[8];
   char uid[8];
   char gid[8];
   char size[12];
   char mtime[12];
   char chksum[8];
   char typeflag[1];
   char linkname[100];
   char magic[6];
   char version[2];
   char uname[32];
   char gname[32];
   char devmajor[8];
   char devminor[8];
   char prefix[155];
} Header;

typedef struct {
   Header *header;
   FILE *file;
   struct Node *left;
   struct Node *right;
} Node;

typedef struct {
   Node *root;
} Tree;

void check_args(int argc, char *argv[], int *flags);
FILE* get_fp(char path[256]);
int sep_prefix_name(char path[256]);
int header_insert_special_int(char* where, size_t size, int32_t val);
int header_extract_special_int(char* where, int len);
void header_set_uid_bigsafe(char* buf, int32_t uid);
void header_parse_uid_bigsafe(char* uidstring, uid_t* uid);
Header* get_header(FILE* fp, char path[256]);
unsigned char chksum(unsigned char *c, size_t s);
unsigned char chksum_2(Header *header);
char get_type(struct stat st);
void make_tar(Header *header, char path[256], FILE *fp, char * tar_name, int fd);

#endif
