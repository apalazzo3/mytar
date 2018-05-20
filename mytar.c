/*#define _GNU_SOURCE*/
#define _BSD_SOURCE
#define USAGE "usage: ./mytar [ctxSp[f tarfile]] [file1 [ file 2 [...] ] ]\n"
#define C_FLAG 0
#define T_FLAG 1
#define X_FLAG 2
#define V_FLAG 3

#include "mytar.h"

/* Helper function to check the command line arguments */

void check_args(int argc, char *argv[], int *flags){
   if(argc == 1){
      fprintf(stderr, "./mytar: you must specify at least one of the 'ctx' options.\n");
      exit(EXIT_FAILURE);
   }
   
   if(argc != 4){
      fprintf(stderr, USAGE);
      exit(EXIT_FAILURE);
   }
   /*not our job to worry about spaces between flags*/

   if(strchr(argv[1], 'c') != NULL){
      flags[C_FLAG] = 1;
   }
   if(strchr(argv[1], 't') != NULL){
      flags[T_FLAG] = 1;
   }
   if(strchr(argv[1], 'x') != NULL){
      flags[X_FLAG] = 1;
   }
   if(strchr(argv[1], 'v') != NULL){
      flags[V_FLAG] = 1;
   }

   /* now check present flags (ex. cant have c and x and t) */
   if((flags[C_FLAG] == 1 && flags[T_FLAG] == 1) || (flags[C_FLAG] == 1 && flags[X_FLAG] == 1) || (flags[T_FLAG] == 1 && flags [X_FLAG] == 1)){
      fprintf(stderr, "./mytar: you may only choose one of the ctx options.\n");
      fprintf(stderr, USAGE);
      exit(EXIT_FAILURE);
   }

}

/* Helper function to get the given file from a string */
FILE* get_fp(char path[256]) {
   FILE *fp = fopen(path, "r");
   if (fp == NULL) {
      fprintf(stderr, USAGE);
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

/* Returns a pointer to a complete posix_header struct from tar.h */
Header* get_header(FILE* fp, char path[256]) {
   Header *header = NULL;
   struct stat st;
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
   if (lstat(path, &st) < 0) {
      fprintf(stderr, "lstat failed\n");
      exit(EXIT_FAILURE);
   }

   /* mode[8]; offset: 100 */
   sprintf(header -> mode, "%07o", st.st_mode);

   /* uid[8];           offset: 108 */
   sprintf(header -> uid, "%07o", st.st_uid);

   /* gid[8];           offset: 116 */
   sprintf(header -> gid, "%07o", st.st_gid);

   /* size[12];         offset: 124 */
   sprintf(header -> size, "%011o", (int)st.st_size);

   /* mtime[12];        offset: 136 */
   sprintf(header -> size, "%011o", (int)st.st_mtime);

   /* chksum[8];        offset: 148 */
   /* typeflag;         offset: 156 */
   /* linkname[100];    offset: 157 */
   /* magic[6];         offset: 257 */
   /* version[2];       offset: 263 */
   /* uname[32];        offset: 265 */
   /* gname[32];        offset: 297 */

   /*wouldnt these just default to 0?*/
   /* devmajor[8];      offset: 329 */
   /* devminor[8];      offset: 337 */
   return NULL;
}

int main (int argc, char *argv[])
{
   int flags [4];
   int i;
   FILE *fp;
   char path[256];
   Header *header;
  
   /* init flags to zero */
   for(i = 0; i < 4; i++)
   {
      flags[i] = 0;
   }

   /* check args for errors and which flags are present */
   check_args(argc, argv, flags);


   /* get file pointer from path in argv */
   strcpy(path, argv[3]);
   fp = get_fp(path);
  
   /* after this point there probably needs to be a loop of some */
   /* sort in order to deal with all the files specified in the*/
   /* tree */

   /* create struct header */
   header = get_header(fp, path);

   return 0;
}

/*creating*/
/*get posix struct*/
/*write info from struct to tar file*/
/*write file to tar file*/
/*done*/
/*this is just the process of writing one file,omitting directories*/

/*extracting*/
/*create file*/
/*write from info in tar*/
/*done*/
/*again this doesnt tke into account directoories or huge paths*/
