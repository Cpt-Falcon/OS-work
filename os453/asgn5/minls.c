/*
 * @Author: Aubrey Russell, Chris Voncina
 * @Date: 3/3/2016
 * @Description: minls.c is code that attempts to read
 * and list files from the minix file system. It goes through
 * partition, then sub partitions if applicable, then goes to
 * the super block, then goes to the root inode, then goes
 * through the inode to get the path, then gathers the zones
 * from either the inode or the indirect block or the double
 * indirect block and uses that to print the directory.
 *
 */
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "minls.h"

/*initialize the struct values
  so there isn't any garbage
  values in the datastore*/
void initDs(DataStore *mainStruct)
{
   mainStruct->pList.verbose = -1;
   mainStruct->pList.part = -1;
   mainStruct->pList.sub = -1;
   mainStruct->pList.imgPath = "";
   mainStruct->pList.filePath = "";
   mainStruct->pList.destPath = "";
   mainStruct->hasDest = -1;
   mainStruct->hasPath = -1;
   mainStruct->partOffset = 0;
   mainStruct->superOffset = 0;
}

/*
 * This function is used to check what the permission is of file 
 * that one is trying to read. Initialize all of permissions to nothing
 * then check the specific position and update the indices.
 */
void checkPermissions(uint16_t mode, char *permission)
{
   int i;
   /*initialize the param to be a blankslate */
   for(i = 0; i < PERMISSION_SIZE-1; i++ )
   {
      permission[i] = '-';
   }
   /*check if what trying to read is directory */
   if ((mode & Directory) == Directory)
   {
      permission[0] = 'd';
   }

   /*the following three if statements check the 
    * permissions the owner has with file */  
   if ((mode & Owner_read_permission)
         == Owner_read_permission)
   {
      permission[1] = 'r';
   }

   if ((mode & Owner_write_permission) ==
         Owner_write_permission)
   {
      permission[2] = 'w';
   }

   if ((mode & Owner_execute_permission) ==
         Owner_execute_permission)
   {
      permission[3] = 'x';
   }

   /* the following three if statements check 
    * the permissions the group has with file*/ 
   if ((mode & Group_read_permission) ==
         Group_read_permission)
   {
      permission[4] = 'r';
   }

   if ((mode & Group_write_permission) ==
         Group_write_permission)
   {
      permission[5] = 'w';
   }

   if ((mode & Group_execute_permission) ==
         Group_execute_permission )
   {
      permission[6] = 'x';
   }

   /* the following three if statements check 
    * the permission the rest have with file*/ 
   if ((mode & Other_read_permission) ==
         Other_read_permission)
   {
      permission[7] = 'r';
   }

   if ((mode & Other_write_permission) ==
         Other_write_permission)
   {
      permission[8] = 'w';
   }

   if ((mode & Other_execute_permission) ==
         Other_execute_permission)
   {
      permission[9] = 'x';
   }

   permission[10] = '\0';
}

/*helper function for read to assist with error handling
  and to therefore help keep the code clean*/
void freadHelp(void *ptr, size_t size, size_t nmemb, FILE *stream, char *msg)
{
   int err;
   /*check for error*/
   if ((err = fread(ptr, size, nmemb, stream)) < 0)
   {
      perror(msg);
      exit(err);
   }
}

/*helper function for seek to assist with error handling
  and to therefore help keep the code clean*/
void fseekHelp(FILE *fp, int offset, int whence, char *msg)
{
   int err;
   /*check for error*/
   if ((err = fseek(fp, offset, whence)) < 0)
   {
      perror(msg);
      exit(err);
   }

}

/*calculate the zone size by bit shifting
to the left blocks by the log_zone_size*/
void findZoneSize(DataStore *mainStruct)
{
   mainStruct->zoneSize = mainStruct->blk.blocksize
      << ((uint32_t)mainStruct->blk.log_zone_size);
}

/* This function handles reading of the super block. 
   It uses our helper functions to do 
   error checking. Also prints out contents of superblock
   when verbose has been set. Copies to our data struct.
   */
void getSprBlk(DataStore *mainStruct)
{
   mainStruct->superOffset = mainStruct->partOffset + SUPER_OFFSET;
   fseekHelp(mainStruct->fp, mainStruct->superOffset, SEEK_SET,
         "super block seek error");
   freadHelp(&(mainStruct->blk), sizeof(Superblock), 1, mainStruct->fp,
         "super block read error");
   findZoneSize(mainStruct);
   /*check the magic number if it isnt valid throw error */
   if(mainStruct->blk.magic != MAGIC_NUM &&
        mainStruct->blk.magic != LITTLE_MAGIC_NUM )
   {
      printf("Bad magic number. (0x%04x)\n", mainStruct->blk.magic);  
      printf("This doesn't look like a MINIX filesystem. \n");
      exit(EXIT_FAILURE);
   }
   /*verbose print*/
   if (mainStruct->pList.verbose == 1)
   {
      printf("Superblock Contents:\n");
      printf("Stored Fields:\n");
      printf("ninodes         %d\n", mainStruct->blk.ninodes);
      printf("i_blocks        %d\n", mainStruct->blk.i_blocks);
      printf("z_blocks        %d\n", mainStruct->blk.z_blocks);
      printf("firstdata       %d\n", mainStruct->blk.firstdata);
      printf("log_zone_size   %d (zone size: %d)\n",
            mainStruct->blk.log_zone_size, mainStruct->zoneSize);
      printf("max_file        %u\n", mainStruct->blk.max_file);
      printf("magic           %04X\n", mainStruct->blk.magic);
      printf("zones           %d\n", mainStruct->blk.zones);
      printf("blocksize       %d\n", mainStruct->blk.blocksize);
      printf("subversion      %d\n", mainStruct->blk.subversion);  
   }
}
/*
 * This is a helper function that checks 
 * the partition table and sees if
 * the bootsector's bytes 510 and 511
 * contain valid values.
 */
void checkValid(DataStore *mainStruct)
{
   char valid;
   fseekHelp(mainStruct->fp, mainStruct->partOffset + PART_OFFSET, SEEK_SET,
         "checkValid seek error");
   freadHelp(&valid, sizeof(char), 1, mainStruct->fp,
         "checkValid read error");
   /*check if the byte 510 of bootsector is 0x55 */
   if((valid & BOOT_PART1) != BOOT_PART1)
   {
      printf("Byte 510 of bootsector must be 0x55\n");
      exit(EXIT_FAILURE);
   }

   fseekHelp(mainStruct->fp, mainStruct->partOffset + PART2_OFFSET, SEEK_SET,
         "checkValid seek error");
   freadHelp(&valid, sizeof(char), 1, mainStruct->fp,
         "checkValid read error");
   /*check if the partition type is minix or not */
   if((valid & BOOT_PART2) != BOOT_PART2)
   {
      printf("Byte 511 of bootsector must be 0xAA\n");
      exit(EXIT_FAILURE);
   }
}
/*
 * Check if valid partition then
 * seek and read from the offset to get partition info
 * for both a partition and sub partition if specified.
 * Function is not used if partition is not specified
 */
void getPtable(DataStore *mainStruct)
{  
   int initalSublk;
   checkValid(mainStruct);
   /*set the offset and then seek and read*/
   initalSublk = mainStruct->partOffset = PART_TABLE +
      mainStruct->pList.part * sizeof(Part);
   fseekHelp(mainStruct->fp, mainStruct->partOffset, SEEK_SET,
         "getPtable seek error");
   freadHelp(&(mainStruct->primPart), sizeof(Part), 1, mainStruct->fp,
         "getPtable read error");

   mainStruct->partOffset = mainStruct->primPart.lFirst * SECTOR_SIZE;
   /*check valid minix type*/
   if (mainStruct->primPart.type != MINIX_TYPE)
   {
      printf("This is not a MINIX filesystem\n");
      exit(EXIT_FAILURE);
   }
   /*verbose output*/
   if (mainStruct->pList.verbose == 1)
   {
      printf("Partition Contents:\n");
      printf("Stored Fields:\n");
      printf("bootind         %u\n", mainStruct->primPart.bootind);
      printf("start_head      %u\n", mainStruct->primPart.start_head);
      printf("start_sec       %u\n", mainStruct->primPart.start_sec);
      printf("start_cyl       %u\n", mainStruct->primPart.start_cyl);
      printf("type            %x\n", mainStruct->primPart.type);
      printf("end_head           %u\n", mainStruct->primPart.end_head);
      printf("end_sec         %u\n", mainStruct->primPart.end_sec);
      printf("end_cyl         %u\n", mainStruct->primPart.end_cyl);
      printf("lFirst          %u\n", mainStruct->primPart.lFirst);
      printf("size            %u\n", mainStruct->primPart.size);
   }
   /*check if there is a sub partition to read*/
   if (mainStruct->pList.sub != -1)
   {
      checkValid(mainStruct);
      /*set offset and seek to sub partition then read*/
      mainStruct->partOffset += PART_TABLE +
         (mainStruct->pList.sub) * sizeof(Part);
      fseekHelp(mainStruct->fp, mainStruct->partOffset, SEEK_SET,
            "getPtable seek error");
      freadHelp(&(mainStruct->primPart), sizeof(Part), 1, mainStruct->fp,
            "getPtable read error");
      mainStruct->partOffset = mainStruct->primPart.lFirst * SECTOR_SIZE;

      if (mainStruct->primPart.type != MINIX_TYPE)
      {
         printf("This is not a MINIX filesystem\n");
         exit(EXIT_FAILURE);
      }
      if (mainStruct->pList.verbose == 1)
      {
         printf("Sub Partition Contents:\n");
         printf("Stored Fields:\n");
         printf("bootind         %u\n", mainStruct->primPart.bootind);
         printf("start_head      %u\n", mainStruct->primPart.start_head);
         printf("start_sec       %u\n", mainStruct->primPart.start_sec);
         printf("start_cyl       %u\n", mainStruct->primPart.start_cyl);
         printf("type            %x\n", mainStruct->primPart.type);
         printf("end_head           %u\n", mainStruct->primPart.end_head);
         printf("end_sec         %u\n", mainStruct->primPart.end_sec);
         printf("end_cyl         %u\n", mainStruct->primPart.end_cyl);
         printf("lFirst          %u\n", mainStruct->primPart.lFirst);
         printf("size            %u\n", mainStruct->primPart.size);
      }
   }
}
/*
 *Set and print out all the information in inode.
 */
void inodeVerbose(DataStore *mainStruct)
{
   char timeBuff[BUFF_TIME];
   time_t newTime;
   char permission[PERMISSION_SIZE];
   /*more verbose print stuff*/
   if (mainStruct->pList.verbose == 1)
   {
      checkPermissions(mainStruct->node.mode, permission);
      printf("File inode:\n");
      printf("uint16_t mode   %x (%s)\n", mainStruct->node.mode, permission);
      printf("uint16_t links  %d\n", mainStruct->node.links);
      printf("uint16_t uid    %d\n", mainStruct->node.uid);
      printf("uint16_t gid    %d\n", mainStruct->node.gid);
      printf("uint32_t size   %d\n", mainStruct->node.size);

      newTime = mainStruct->node.atime;
      strftime(timeBuff, BUFF_TIME, "%c", localtime(&newTime));
      printf("uint32_t atime  %d --- %s\n", mainStruct->node.atime, timeBuff);

      newTime = (time_t)mainStruct->node.mtime;
      strftime(timeBuff, BUFF_TIME,"%c", localtime(&newTime));
      printf("uint32_t mtime  %d --- %s\n", mainStruct->node.mtime, timeBuff);

      newTime = (time_t)mainStruct->node.ctime;
      strftime(timeBuff, BUFF_TIME ,"%c", localtime(&newTime));
      printf("uint32_t ctime  %d --- %s\n", mainStruct->node.ctime, timeBuff);
   }
}

/*
 * Set the inode offset, and then seek and
 * read the first inode, then store in our
 * datastore struct for later usage.
 */
void getRootInode(DataStore *mainStruct)
{
   /*set the correct offset*/
   mainStruct->inodeRootOffset = mainStruct->partOffset +
      mainStruct->blk.blocksize * (mainStruct->blk.i_blocks +
            mainStruct->blk.z_blocks + 2);
   mainStruct->dataOffset = sizeof(inode) *
      mainStruct->blk.ninodes + mainStruct->inodeRootOffset;
   /*seek and read the root inode*/
   fseekHelp(mainStruct->fp, mainStruct->inodeRootOffset, SEEK_SET,
         "getRootInode seek error");
   freadHelp(&(mainStruct->node), sizeof(inode), 1, mainStruct->fp,
         "getRootInode read error");
   inodeVerbose(mainStruct);
}

/*
 * Seek to an inode and read from it. 
 */
void getInode(DataStore *mainStruct, inode *out, int inodeNum)
{
   fseekHelp(mainStruct->fp, mainStruct->inodeRootOffset + inodeNum *
         sizeof(inode), SEEK_SET, "getInode seek error");
   freadHelp(out, sizeof(inode), 1, mainStruct->fp, "getInode read error");
}

/*read in inode zone blocks, indirect zone blocks,
and double indirect blocks combine them into a 
single block of memory for various purposes*/
char *getZoneBlocks(DataStore *mainStruct)
{
   int seeker, doubleSeeker, doubleSize, indirectSize;
   uint32_t zoneInc = 0, indirectInc = 0, curZone, doubleInc = 0;
   char *block = NULL;
   uint32_t tempSize = mainStruct->node.size;
   uint32_t curPos = 0;

   /*iterate through all the inode zones*/
   while (zoneInc < DIRECT_ZONES)
   {
      if (mainStruct->node.zone[zoneInc] > 0)
      {
         /*seek to the zone blocks*/
         fseekHelp(mainStruct->fp, mainStruct->partOffset + 
            mainStruct->node.zone[zoneInc] * mainStruct->zoneSize, 
            SEEK_SET, "getZoneBlock seek error");
         /*check that the current position doesn't exceed the current size*/
         if (curPos <= mainStruct->node.size)
         {
            block = realloc(block, curPos + mainStruct->zoneSize);
            freadHelp(block + curPos, mainStruct->zoneSize, 1, mainStruct->fp, 
               "getZoneBlock read error");
            curPos += mainStruct->zoneSize;
         }
         else
         {
            block = realloc(block, curPos + tempSize - curPos);
            freadHelp(block + curPos, tempSize - curPos, 1, mainStruct->fp, 
               "getZoneBlock read error");
            curPos += tempSize - curPos;

            return block;
         }
      }
      zoneInc++;
   }

   /*handles the indirect zones*/
   zoneInc = 0;
   if (mainStruct->node.indirect > 0)
   {
      /*set the offset and then go through the indirect zone seek pointers*/
      indirectSize = (int)(mainStruct->zoneSize / sizeof(uint32_t));
      seeker = mainStruct->partOffset + 
         mainStruct->node.indirect * mainStruct->zoneSize;
      /*read all of the zone data pointed to by the indirect zone*/
      while (curPos <= mainStruct->node.size && indirectInc < indirectSize)
      {
         fseekHelp(mainStruct->fp, seeker + indirectInc * sizeof(uint32_t), 
            SEEK_SET, "getZoneBlock seek error");         
         freadHelp(&curZone, sizeof(uint32_t), 1, mainStruct->fp, 
            "getZoneBlock read error");
         /*if not a hole*/
         if (curZone > 0)
         {
            fseekHelp(mainStruct->fp, mainStruct->partOffset + curZone * 
               mainStruct->zoneSize, SEEK_SET, "getZoneBlock seek error");
            /*verify the size*/
            if (curPos <= mainStruct->node.size)
            {
               block = realloc(block, curPos + mainStruct->zoneSize);
               freadHelp(block + curPos, mainStruct->zoneSize, 1, 
                  mainStruct->fp, "getZoneBlock read error");
               curPos += mainStruct->zoneSize;
            }
            else
            {

               block = realloc(block, curPos + tempSize - curPos);
               freadHelp(block + curPos, tempSize - curPos, 1, 
                  mainStruct->fp, "getZoneBlock read error");
               curPos += tempSize - curPos;
            return block;
            }
         }
         indirectInc++;
      }
   }
   /*check the double indirect zones*/
   if (mainStruct->node.two_indirect > 0)
   {
      doubleSize = (int)(mainStruct->zoneSize / sizeof(uint32_t));
      seeker = mainStruct->partOffset + 
         mainStruct->node.two_indirect * mainStruct->zoneSize;
      /*iterate through all the indirect zones*/
      while (curPos <= mainStruct->node.size)
      {
         doubleSeeker = seeker + doubleInc * sizeof(uint32_t);
         zoneInc = 0;
         indirectInc = 0;
         /*go through all the zones pointed to be the indirect zones*/
         while (indirectInc <= doubleSize)
         {
            fseekHelp(mainStruct->fp, doubleSeeker + indirectInc * 
               sizeof(uint32_t), SEEK_SET, "getZoneBlock seek error");
            freadHelp(&curZone, sizeof(uint32_t), 1, mainStruct->fp, 
               "getZoneBlock read error");
            /*check if a hole*/
            if (curZone > 0)
            {
               fseekHelp(mainStruct->fp, mainStruct->partOffset + curZone * 
                  mainStruct->zoneSize, SEEK_SET, "getZoneBlock seek error");
               /*verify size*/
               if (curPos <= mainStruct->node.size)
               {
                  block = realloc(block, curPos + mainStruct->zoneSize);
                  freadHelp(block + curPos, mainStruct->zoneSize, 1, 
                     mainStruct->fp, "getZoneBlock read error");
                  curPos += mainStruct->zoneSize;
               }
               else
               {

                  block = realloc(block, curPos + tempSize - curPos);
                  freadHelp(block + curPos, tempSize - curPos, 1, 
                     mainStruct->fp, "getZoneBlock read error");
                  curPos += tempSize - curPos;
               return block;
               }
            }
            indirectInc++;
         }
         doubleInc++;
      }
   }
   return block; /*return pointer to memory block containing zone data*/
}

/*get the data of the zone and output it*/
void getZone(DataStore *mainStruct)
{
   int i = 0;
   Dir curDir;
   inode newInode;
   char permission[11];
   char *block;

   if (mainStruct->pList.verbose == 1)
   {
      printf("Direct zones:\n");
      printf("zone[0]  =  %d\n", mainStruct->node.zone[0]);
      printf("zone[1]  =  %d\n", mainStruct->node.zone[1]);
      printf("zone[2]  =  %d\n", mainStruct->node.zone[2]);
      printf("zone[3]  =  %d\n", mainStruct->node.zone[3]);
      printf("zone[4]  =  %d\n", mainStruct->node.zone[4]);
      printf("zone[5]  =  %d\n", mainStruct->node.zone[5]);
      printf("zone[6]  =  %d\n", mainStruct->node.zone[6]);
      printf("uint32_t indirect  %d\n", mainStruct->node.indirect);
      printf("uint32_t double  %d\n", mainStruct->node.two_indirect);
   }

   block = getZoneBlocks(mainStruct);
   printf("%s:\n", mainStruct->pList.filePath);
   /*iterate through all the directory entries*/
   while (i < mainStruct->node.size)
   {

      memcpy(&curDir, block + i, sizeof(Dir));
      getInode(mainStruct, &newInode, curDir.inode - 1);
      /*make sure valid inode*/
      if (curDir.inode != 0)
      {
         checkPermissions(newInode.mode, permission);
         printf("%s %*d %s\n", permission, 9, newInode.size, curDir.name);
      }
      i += DIR_SIZE;
   }
   free(block);
}

/*traverse to the specified path using inodes
and strtoken to parse*/
void goToPath(DataStore *mainStruct)
{
   int i = 0;
   char permission[11];
   Dir curDir;
   int found;
   const char s[2] = "/";
   char *tempFile, *token, *block;
   inode newInode;
   tempFile = malloc(sizeof(char) * strlen(mainStruct->pList.filePath) + 1);
   strcpy(tempFile, mainStruct->pList.filePath);
   token = strtok(tempFile, s);
   /*iterate through the parsed file path tokens*/
   while (token != NULL)
   {
      i = 0;
      found = 0;
      block = getZoneBlocks(mainStruct);
      /*get the zone blocks and loop through the 
      data corresponding to a directory by
      going through all directory entries*/
      while (i <= mainStruct->node.size)
      {

         memcpy(&curDir, block + i, sizeof(Dir));
         getInode(mainStruct, &newInode, curDir.inode - 1);
         /*check that the string of the token matches the inode*/
         if (strcmp(token, (char *)(curDir.name)) == 0)
         {
            /*check if the inode was deleted, error*/
            if (curDir.inode == 0)
            {
               printf("The directory or file is deleted\n");
               exit(EXIT_FAILURE);
            }
            mainStruct->node = newInode;
            found = 1;
            break;
         }
         i += DIR_SIZE;
      }
      /*if not found, error*/
      if (!found)
      {
         printf("Path or file not found\n");
         exit(EXIT_FAILURE);
      }
      else
      {
         free(block); /*free the allocated block made from the zones*/
      }
      token = strtok(NULL, s);   
   }

   free(tempFile);
   checkPermissions(newInode.mode, permission);
   inodeVerbose(mainStruct);
   /*if not a directory print and end*/
   if (permission[0] != 'd')
   {
      printf("%s  %d %s\n", permission, mainStruct->node.size, 
         mainStruct->pList.filePath);
      exit(EXIT_SUCCESS);
   }
}

/*convenience function for usage output*/
void printUsage()
{
   printf("usage: minls [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n");
   printf("Options:\n");
   printf("-p part  --- select partition for filesystem (default: none)\n");
   printf("-s  sub  --- select subpartition");
   printf(" for filesystem (default: none)\n");
   printf("-h help --- print usage information and exit\n");
   printf("-v verbose --- increase verbosity level\n");
}

/*main program driver*/
int main(int argc, char *argv[]) {
   DataStore mainStruct;
   initDs(&mainStruct); /*initialize the function*/
   int c;
   char *endPtr;
   int numArgs = argc;
   /*parse command line args*/
   while ((c = getopt(argc, argv, "v:p:s:h:")) != -1) {
      switch (c) {
         /*verbose setting*/
         case 'v': 
            mainStruct.pList.verbose = 1;
            numArgs--;
            break;
         /*partition*/
         case 'p':
            mainStruct.pList.part = strtol(optarg, &endPtr, 10);
            numArgs -= 2;
            break;
         /*sub partition*/
         case 's':
            mainStruct.pList.sub = strtol(optarg, &endPtr, 10);
            numArgs -= 2;
            break;
         /*help*/
         case 'h':
            numArgs--;
            printUsage();
            exit(EXIT_SUCCESS);
            break;
         case '?':
            printf("Thing not found\n");
            break;
         default:
            break;
      }
   }
   numArgs--;
   /*insufficient arguments*/
   if (numArgs == 0)
   {
      printUsage();
      exit(EXIT_FAILURE);
   }
   /*load image path with root dir*/
   else if (numArgs == 1)
   {
      mainStruct.pList.imgPath = argv[argc - 1];
      mainStruct.pList.filePath = "/";
   }
   /*load image path and specific path*/
   else
   {

      mainStruct.pList.imgPath = argv[argc - 2];
      mainStruct.pList.filePath = argv[argc - 1];
      /*check the last thing for a path*/
      if (strcmp(mainStruct.pList.filePath, "/") != 0)
      {
         mainStruct.hasPath = 1;
      }

   }
   /*open file and check for error*/
   mainStruct.fp = fopen(mainStruct.pList.imgPath, "r");
   if (mainStruct.fp == NULL)
   {
      perror("Unable to open file");
      exit(EXIT_FAILURE);
   }

   /*if there is a partition get it*/
   if (mainStruct.pList.part != -1)
   {
      getPtable(&mainStruct);      
   }

   getSprBlk(&mainStruct);

   getRootInode(&mainStruct);

   /*if there is a path, iterate through inodes*/
   if(mainStruct.hasPath == 1)
   {
      goToPath(&mainStruct);
   }

   getZone(&mainStruct);
   fclose(mainStruct.fp);
   return 0;
}





