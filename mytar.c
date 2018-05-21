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

/* Helper function to calculate the chksum */
unsigned char chksum(unsigned char *c, size_t s){
   unsigned char num = 0;
   while(s-- != 0){
      num -= *c++;
   }
   return num;
}

/* Helper function (uses chksum) re-calculates header sum */
unsigned char chksum_2(Header *header){
   unsigned char sum = 0;

   /* note: keep track of length of things, whether or not to */
   /* include the ending null chars or not */

   /* compiler loses its shit since things arent stored as uchars */
   /* note: fixed this ^ but this can be a problem so keep an */
   /* eye on it */
   
   sum += chksum((unsigned char*)(&header -> name), 100);
   sum += chksum((unsigned char*)(&header -> mode), 8);
   sum += chksum((unsigned char*)(&header -> uid), 8);
   sum += chksum((unsigned char*)(&header -> gid), 8);
   sum += chksum((unsigned char*)(&header -> size), 12);
   sum += chksum((unsigned char*)(&header -> mtime), 12);
   sum += chksum((unsigned char*)(&header -> chksum), 8);
   sum += chksum((unsigned char*)(&header -> typeflag), 1);
   sum += chksum((unsigned char*)(&header -> linkname), 100);
   sum += chksum((unsigned char*)(&header -> magic), 6);
   sum += chksum((unsigned char*)(&header -> version), 2);
   sum += chksum((unsigned char*)(&header -> uname), 32);
   sum += chksum((unsigned char*)(&header -> gname), 32);
   sum += chksum((unsigned char*)(&header -> devmajor), 8);
   sum += chksum((unsigned char*)(&header -> devminor), 8);
   sum += chksum((unsigned char*)(&header -> prefix), 155);
   
   return sum;
}

/* Helper function to find type of file */
char get_type(struct stat st){
   char type = 0;

   switch(st.st_mode & S_IFMT){
      case S_IFREG:
         type = '0';
         break;
      case S_IFLNK:
         type = '1';
         break;
      case S_IFDIR:
         type = '2';
         break;
      default:
         type = -1;
         exit(EXIT_FAILURE);
   }

   return type;
}

/* Returns a pointer to a complete posix_header struct from tar.h */
Header* get_header(FILE* fp, char path[256]) {
   Header *header = malloc(sizeof(*header));
   struct stat st;
   int i;
   unsigned char temp[] = "       "; /* empty checksum first calc */
   char type = -1;
   unsigned int empty = 0;

   /* name[100], offset: 0; prefix[155], offset: 345 */
   /* null-terminated char string */
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

   /* might need to add NULL terminators to these (?) */

   /* mode[8]; offset: 100 */
   sprintf(header -> mode, "%07o", st.st_mode);

   /* uid[8];           offset: 108 */
   sprintf(header -> uid, "%07o", st.st_uid);

   /* gid[8];           offset: 116 */
   sprintf(header -> gid, "%07o", st.st_gid);

   /* size[12];         offset: 124 */
   sprintf(header -> size, "%011o", (int)st.st_size);

   /* mtime[12];        offset: 136 */
   sprintf(header -> mtime, "%011o", (int)st.st_mtime);

   /* given checksum of 7 spaces */
   /* chksum[8];        offset: 148 */
   sprintf(header -> chksum, "%07o", chksum(temp, 7));

   /* typeflag[1];         offset: 156 */
   type = get_type(st);
   sprintf(header -> typeflag, "%01o", type);

   /* check first if file is even symlink type  */
   /* linkname[100];    offset: 157 */
   printf("%c", type);
   if(type == '2'){
      if(readlink(path, header -> linkname, strlen(path)) < 0){
         fprintf(stderr,"usage: could not read link %s\n", path);
         exit(EXIT_FAILURE);
      }
   }
   else{ /* not symlink */
      /* note: possibly leave uninitialized */
   }
   
   /* magic[6];         offset: 257 */
   /* null-terminated char string */
   strncpy(header->magic, "ustar", 6);

   /* version[2];       offset: 263 */

   sprintf(header -> version, "%02o", empty);


   /* uname[32];        offset: 265 */
   /* file's owner name */
   /* null-terminated char string */
   sprintf(header -> uname, "%s", (getpwuid(st.st_uid)) -> pw_name);

   /* gname[32];        offset: 297 */
   /* file's group name */
   /* null-terminated char string */
   sprintf(header -> gname, "%s", (getgrgid(st.st_gid)) -> gr_name);

   /*wouldnt these just default to 0?*/
   /* devmajor[8];      offset: 329 */
   /*sprintf(header -> devmajor, "");*/

   /* devminor[8];      offset: 337 */
   /*sprintf(header -> devminor, "");*/

   /* checksum recalculation */
   /* this requires going through every component of the header */
   /* lets use a helper for this, gonna be a bit big*/
   sprintf(header -> chksum, "%70o", chksum_2(header));

   return header;
}


/* check if tar exists for multiple files */
void make_tar(Header *header, char path[256], FILE *fp, char * tar_name){
   int fd;
   int i = 0;
   char *file;
   char c;
   int num_chars = 0;
   int limit = 1024;
   char empty[1536];

   for(i = 0; i < 1536; i++){
      empty[i] = 0;
   }

   /* check if file exists then delete it */
   if((fopen(tar_name, "r")) != NULL){
      remove(tar_name);
   }

   fd = open(tar_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
   
   /* write header to tar file */

   write(fd, header -> name, 100);
   write(fd, header -> mode, 8);
   write(fd, header -> uid, 8);
   write(fd, header -> gid, 8);
   write(fd, header -> size, 12);
   write(fd, header -> mtime, 12);
   write(fd, header -> chksum, 8);
   write(fd, header -> typeflag, 1);
   write(fd, header -> linkname, 100); /* possile segfault */
   write(fd, header -> magic, 6);
   write(fd, header -> version, 2);
   write(fd, header -> uname, 32);
   write(fd, header -> gname, 32);
   write(fd, header -> devmajor, 8);
   write(fd, header -> devminor, 8);
   write(fd, header -> prefix, 155);
  
   write(fd, empty, 12);

   /* write file to tar file */
   /*while((c = getc(fp)) != EOF){
      write(fd, c, sizeof(char));
   }*/


   file = malloc(sizeof(char) * limit);
   while((c = getc(fp)) != EOF){
      if(num_chars + 1 == limit){
          file = realloc(file, limit *= 2);
      }
      file[num_chars] = c;
      num_chars++;
   }

   write(fd, file, (num_chars)); 
   write(fd, empty, 1524);

   fclose(fp);
   free(header);
   free(file);
}

int main (int argc, char *argv[]){
   int flags [4];
   int i;
   FILE *fp;
   char path[256];
   Header *header;
  
   /* init flags to zero */
   for(i = 0; i < 4; i++){
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

   /* create and write to tar file */
   make_tar(header, path, fp, argv[2]);

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
