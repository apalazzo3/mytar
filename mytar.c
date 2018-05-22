/*#define _GNU_SOURCE*/
#define _BSD_SOURCE
#define USAGE "usage: ./mytar [ctxSp[f tarfile]] [file1 [ file 2 [...] ] ]\n"
#define C_FLAG 0
#define T_FLAG 1
#define X_FLAG 2
#define V_FLAG 3
#define UID_PACKING_LEN 8

#include "mytar.h"

/* Helper function to check the command line arguments */

void check_args(int argc, char *argv[], int *flags){
   if(argc == 1){
      fprintf(stderr, "./mytar: you must specify at least one of the 'ctx' options.\n");
      exit(EXIT_FAILURE);
   }
   
   if(argc != 4 && argc != 3){
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
         fprintf(stderr, "get_type(3)\n");
         exit(EXIT_FAILURE);
   }

   return type;
}

int header_insert_special_int(char* where, size_t size, int32_t val)
{
    int err = 0;
    if (val < 0 || size < sizeof(val))
    {
        err++;
    }
    else
    {
        memset(where, 0, size);
        *(int32_t *)(where+size-sizeof(val)) = htonl(val);
        *where |= 0x80;
        fprintf(stderr, "In else\n");
    }
    fprintf(stderr, "err: %d\n", err);
    return err;
}

int header_extract_special_int(char* where, int len)
{
    int32_t val = -1;
    if ( (len>=sizeof(val)) && (where[0] & 0x80))
    {
        val = *(int32_t *)(where + len - sizeof(val));
        val = ntohl(val);
    }
    return val;
}


void header_set_uid_bigsafe(char* buf, int32_t uid)
{
    int toobig = 07777777;
    fprintf(stderr, "toobig: %o\n", toobig);
    /* Tests if bitpacking is needed */
    if(uid > toobig)
    {
        fprintf(stderr, "uid: %o\n", uid);
        header_insert_special_int(buf,UID_PACKING_LEN,uid);
        fprintf(stderr, "Here 1\n");
    }
    else
    {
        /* Do normal formatting as octal */
        fprintf(stderr, "Here 2\n");
    }
}

void header_parse_uid_bigsafe(char* uidstring, uid_t* uid)
{   

    /* Tests if bitpacking was used */
    if(uidstring[0] == '\0')
    {
        *uid = header_extract_special_int(uidstring, UID_PACKING_LEN);
    }
    else
    {
        /* Do normal parsing from octal */
    }
}

/* Returns a pointer to a complete posix_header struct from tar.h */
Header* get_header(FILE* fp, char path[256]) {
   Header *header = malloc(sizeof(*header));
   struct stat st;
   int i;
   unsigned char temp[] = "       "; /* empty checksum first calc */
   char type;
   char *buf;
   char *buf2;
   char *strng;
   long unsigned int decimal_long;
   int decimal;
   char final;
   unsigned int empty = 0;
   char zeroes[100];

   for(i = 0; i < 100; i++) {
      zeroes[i] = 0;
   }
   i = 0;

   buf = malloc(sizeof(char) * 100);
   buf2 = malloc(sizeof(char) * 2);
   strng = malloc(sizeof(char) * 8);

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
   header -> mode[0] = '0';
   header -> mode[1] = '0';
   header -> mode[2] = '0';

   /* uid[8];           offset: 108 */
   fprintf(stderr, "buf: %s\n", buf);
   fprintf(stderr, "st_uid before (octal): %07o\n", st.st_uid);

   /*header_set_uid_bigsafe(buf, st.st_uid);*/

   fprintf(stderr, "buf: %s\n", buf);
   fprintf(stderr, "uid before (string): %s\n", header -> uid);

   /*sprintf(header -> uid, "%07o", (unsigned int)strtol(buf, 0L, 8));*/
   sprintf(header -> uid, "%07x", st.st_uid);

   for (i = 0; i < 8; i++) {
      if (i % 2 == 0 && i < 6 && i != 2) {
         buf2[0] = header -> uid[i + 1];
	 buf2[1] = header -> uid[i + 2];
         fprintf(stderr, "buf2: %s\n", buf2);
	 decimal_long = strtol(buf2, 0, 16);
         fprintf(stderr, "decimal_long: %ld\n", decimal_long);
         decimal = (int)decimal_long;
         fprintf(stderr, "decimal: %d\n", decimal);
	 final = decimal;	 
         fprintf(stderr, "final: %c\n", final);
	 strng[i] = final;
         fprintf(stderr, "strng: %s\n", strng);
      }
      else{
      }
   }
   for(i = 0; i < 8; i++){
      fprintf(stderr, "%c", strng[i]);
   }
   sprintf(header -> uid, "%s", strng);



   fprintf(stderr, "uid after (string): %s\n", header -> uid);
   fprintf(stderr, "st_uid after (octal): %07o\n", st.st_uid);
   fprintf(stderr, "buf: %s\n", buf);

   /* gid[8];           offset: 116 */
   sprintf(header -> gid, "%07o", st.st_gid);

   /* size[12];         offset: 124 */
   sprintf(header -> size, "%011o", (int)st.st_size);

   /* mtime[12];        offset: 136 */
   sprintf(header -> mtime, "%011o", (int)st.st_mtime);

   /* given checksum of 7 spaces */
   /* chksum[8];        offset: 148 */
/*   fprintf(stderr, "init chksum (string): %s\n", header->chksum);*/
   sprintf(header -> chksum, "%07o", chksum(temp, 7));
/*   fprintf(stderr, "chksum (string): %s\n", header->chksum);*/
/*   fprintf(stderr, "chksum (octal): %07o\n", chksum(temp, 7));*/

   /* typeflag[1];         offset: 156 */
   type = get_type(st);
   buf[0] = type;
/*   sprintf(header -> typeflag, "%s", buf);*/
   strcpy(header -> typeflag, buf);

   /* check first if file is even symlink type  */
   /* linkname[100];    offset: 157 */
   if(type == '2'){
      if(readlink(path, header -> linkname, strlen(path)) < 0){
         fprintf(stderr,"usage: could not read link %s\n", path);
         exit(EXIT_FAILURE);
      }
   }
   else{ /* not symlink */
      /* note: possibly leave uninitialized */
      sprintf(header -> linkname, zeroes);
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
   sprintf(header -> chksum, "%07o", chksum_2(header));
   return header;
}


/* check if tar exists for multiple files */
void make_tar(Header *header, char path[256], FILE *fp, char * tar_name, int fd){
   int i = 0;
   char *file;
   char c;
   int num_chars = 0;
   int limit = 1024;
   char empty[1536];

   for(i = 0; i < 1536; i++){
      empty[i] = 0;
   }

   /* write header to tar file */

   write(fd, header -> name, 100);
/*   write(fd, "/Mode/", 6);*/
   write(fd, header -> mode, 8);
/*   write(fd, "/UID/", 5);*/
   write(fd, header -> uid, 8);
/*   write(fd, "/GID/", 5);*/
   write(fd, header -> gid, 8);
/*   write(fd, "/Size/", 6);*/
   write(fd, header -> size, 12);
/*   write(fd, "/Mtime/", 7);*/
   write(fd, header -> mtime, 12);
/*   write(fd, "/Chksum/", 8);*/
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

   free(header);
   free(file);
}

int main (int argc, char *argv[]){
   int flags [4];
   int i, fd;
   int created = 0;
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
   if(flags[C_FLAG] == 1){
      strcpy(path, argv[3]);
      fp = get_fp(path);
   }
  
   /* after this point there probably needs to be a loop of some */
   /* sort in order to deal with all the files specified in the*/
   /* tree */
   
   if(flags[C_FLAG] == 1){
      /* create struct header */
      header = get_header(fp, path);

      /* create and write to tar file */
      if(created == 0){
         if((fopen(argv[2], "r")) != NULL){
            remove(argv[2]);
         }
         fd = open(argv[2], O_CREAT | O_RDWR, S_IRUSR, S_IWUSR);
         created = 1;
      }
      make_tar(header, path, fp, argv[2], fd);
   }
   else if(flags[T_FLAG] == 1){

   }
   else if(flags[X_FLAG] == 1){

   }
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
