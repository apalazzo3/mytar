/*#define _GNU_SOURCE*/
#define _BSD_SOURCE

#include "mytar.h"

/* Helper function to get the given file from a string */
FILE* get_fp(char path[256]) {
   FILE *fp = fopen(path, "r");
   if (fp == NULL) {
      fprintf(stderr, "usage");
      exit(EXIT_FAILURE);
   }
   
   return fp;
}

/* Helper function to seperate the prefix and the name for a header if too long */
int sep_prefix_name(char path[256]) {
   int i;
   char c;
   for (i = 0; path[i]; i++) {
      c = path[i];
      if (((strlen(path) - i) <= 100) && c == '/') {
         return i;
      }
   }
   fprintf(stderr, "%s: unable to properly divide name. Skipping.\n", path);
   exit(EXIT_FAILURE);
}

/* Returns a pointer to a complete posix_header from tar.h */
struct Header* get_header(FILE* fp, char path[256]) {
   Header *header = NULL;
   struct stat *stat = NULL;
   int i;

   /* name[100], offset: 0; prefix[155], offset: 345 */
   if (strlen(path) > 100) {
      i = sep_prefix_name(path);
      strncpy(header->name, (path + i + 1), (strlen(path) - i + 1));
      strncpy(header->prefix, path, i - 1);
   }
   else {
      strncpy(header->name, path, strlen(path));
   }

   /* Need lstat for the remaining fields */
   if (lstat(path, stat) < 0) {
      fprintf(stderr, "lstat failed\n");
      exit(EXIT_FAILURE);
   }

   /* mode[8]; offset: 100 */
   strmode(stat->st_mode, header->mode);
   /* uid[8]; offset: 108 */
   /* gid[8]; offset: 116 */
   /* size[12]; offset: 124 */
   /* mtime[12]; offset: 136 */
   /* chksum[8]; offset: 148 */
   /* typeflag; offset: 156 */
   /* linkname[100]; offset: 157 */
   /* magic[6]; offset: 257 */
   /* version[2]; offset: 263 */
   /* uname[32]; offset: 265 */
   /* gname[32]; offset: 297 */
   /* devmajor[8]; offset: 329 */
   /* devminor[8]; offset: 337 */
   return NULL;
}

int main (int argc, char *argv[])
{
   return 0;
}
