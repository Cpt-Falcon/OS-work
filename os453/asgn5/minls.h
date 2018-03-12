/*
 * Authors Aubrey Russel & Chris Voncina
 * Header file for minls and minget
 */
#include <sys/types.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdint.h>
#define DIRECT_ZONES 7
#define Size_directory 60
#define  File_type_mask 0xF000
#define  Regular_f 0x8000
#define  Directory 0x4000
#define Owner_read_permission 0x100
#define Owner_write_permission  0x80
#define  Owner_execute_permission 0x40
#define Group_read_permission   0x20
#define Group_write_permission   0x10
#define Group_execute_permission 0x8
#define Other_read_permission  0x4
#define Other_write_permission 0x2
#define Other_execute_permission 0x1
#define DIR_SIZE 64
#define PART_TABLE 0x1BE
#define SECTOR_SIZE 512
#define SUPER_OFFSET 1024
#define INDENT 9
#define PART_OFFSET 510
#define PART2_OFFSET 511
#define BOOT_PART1 0x55
#define BOOT_PART2 0xAA
#define MAGIC_NUM 0x4d5a
#define LITTLE_MAGIC_NUM 0x5a4d
#define PERMISSION_SIZE 11
#define MINIX_TYPE 0x81
#define BUFF_TIME 80

typedef struct Dir
{
   uint32_t inode;
   unsigned char name[60];
} Dir;

typedef struct inode
{
   uint16_t mode;
   uint16_t links;
   uint16_t uid;
   uint16_t gid;
   uint32_t size;
   int32_t atime;
   int32_t mtime;
   int32_t ctime;
   uint32_t zone[DIRECT_ZONES];
   uint32_t indirect;
   uint32_t two_indirect;
   uint32_t unused;
} inode;

/*super block structures that
allows us to get important
information about the files
we're interested in*/
typedef struct Superblock
{
   uint32_t ninodes;  
   uint16_t pad1;
   int16_t i_blocks;
   int16_t z_blocks;
   uint16_t firstdata;
   int16_t log_zone_size;
   int16_t pad2;
   uint32_t max_file;
   uint32_t zones;
   int16_t magic;
   int16_t pad3;
   uint16_t blocksize;
   uint8_t subversion;
} Superblock;

/*partition structure
that defines all the important
aspects of the partition we're
interested in*/
typedef struct Part
{
   uint8_t bootind;
   uint8_t start_head;
   uint8_t start_sec;
   uint8_t start_cyl;
   uint8_t type;
   uint8_t end_head;
   uint8_t end_sec;
   uint8_t end_cyl;
   uint32_t lFirst;
   uint32_t size;
} Part;

/*struct defines the way in which
we keep track of the various
argv parameters*/
typedef struct Parameters
{
   int verbose; /*verbose check if is an argv */
   int part;   /* check if partition is an argv */
   int sub;    /* checking the sub partition is in argv */
   char *imgPath; /*path to the image for filesystem */
   char *filePath; /*path inside the image. */
   char *destPath; /*path trying to find in minget */
} Parameters;

/*struct defines the way in which
we keep track of partitions,
super blocks, etc*/
typedef struct DataStore
{
   FILE *fp; /*image file we want to seek and read from */
   int inodeRootOffset; 
   int superOffset; /* offset of superblock */
   int partOffset; /* the partition offset */
   int dataOffset; /* offset of the zones */
   int hasPath;   /*a check for valid path */
   int hasDest;  /* path you want to write to in minget */
   uint32_t zoneSize;  /* size of the zone */
   struct Parameters pList;/*give datastore access to parameter */
   struct Part primPart; /* gives datastore access to Partitiion table */
   struct Superblock blk; /*give datastore access to superblock */
   struct inode node;  /* gives datastore access to inode */
} DataStore;


