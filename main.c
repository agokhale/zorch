#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/disk.h>

#define controlflag_CLEAR 0x01
#define controlflag_TRIM 0x02

#define damage_zone ( 1024 * 1024 * 10  )  //  cut a 10M clear path 

void usage ( ) { 
	printf ( " zorch ( anything else )  - this message \n" ); 
	printf ( " zorch -l /dev/disk_to_clear_labels \n"); 
	printf ( " zorch -t /dev/disk_to_trim \n"); 
	printf ( " zorch -lt /dev/disk_to_clear_then_trim \n"); 
}

void write_damage_zone ( int fd ) {
  char padding = 0xa5;
  char nonce[1024]; 
  size_t cursor = 0 ; 
  memset ( nonce, 0x5a, 1024); 
  while ( cursor < damage_zone ) { 
    write ( fd, (const void* ) (nonce) ,  1024); 
    if ( cursor % (1<<20) == 0 ) printf ( "."); 
    cursor += 1024;
  }
}

void trim (int fd , off_t len) {
  off_t arg[2];
  arg[0] = 0;
  arg[1] = (off_t)len;
  size_t sectorsize; 
  ioctl(fd, DIOCGSECTORSIZE, &sectorsize);
  printf ( "sectorsize %lu  offset %ld len %ld\n ", sectorsize, arg[0],  len); 
  if (ioctl(fd, DIOCGDELETE, arg) == -1) {
    perror ("trim failed"); 
    exit ( -3 ); 
  }
}

int zorch ( char * target, int controlflags ) {
  int retval = 'f';
  printf ( "target %s, flags: %d\n" ,target, controlflags); 
  int fd ; 
  off_t  disksize; 
  fd = open ( target , O_RDWR | O_DIRECT); 
  disksize = lseek ( fd, 0 , SEEK_END); 
  if ( controlflags & controlflag_CLEAR ) {
    fflush (0);
    printf (" size %ld\n", disksize); 
    lseek ( fd,  disksize-(damage_zone) , SEEK_SET); 
    write_damage_zone  ( fd );
    lseek ( fd,  0  , SEEK_SET); 
    write_damage_zone  ( fd );
  }
  if ( controlflags & controlflag_TRIM ) {
    trim (fd, disksize);
  }
  disksize = lseek ( fd, 0 , SEEK_END); 
  close ( fd ); 
  return ( fd ); 
}

int main  ( int argc , char * argv[] )  {
  int controlflags = 0x00;
  int ch = -1; 
  if (argc < 2) {usage();  exit (-33); }
  while ((ch = getopt(argc, argv, "lt,")) != -1) {
    switch (ch) {
      case 't':  controlflags += controlflag_TRIM ; break;
      case 'l':  controlflags += controlflag_CLEAR;  break;
      default: usage ();  exit (-1); 
    }
 
  }
zorch ( argv[2], controlflags ) ;
printf ("\n"); 
exit (0);
}
