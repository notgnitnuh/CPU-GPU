
extern "C"{
#include "sfs_code/driver.h"
#include "sfs_code/sfs_superblock.h"
}
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <unistd.h>

using namespace std;

int main(int argc, char **argv)
{
  /* declare a buffer that is the same size as a filesystem block */
  char raw_superblock[128];

  /* Check command line args, should receive disk image and optional
     -l for "long" listing. */
  if(!(argc == 2 || (argc == 3 && argv[2] == "-l") ))
  {
    cout << "Usage \n  ./dils <disk.img>\n  ./dils <disk.img> -l\n";
    exit(1);
  }

  /* Create a pointer to the buffer so that we can treat it as a
     superblock struct. The superblock struct is smaller than a block,
     so we have to do it this way so that the buffer that we read
     blocks into is big enough to hold a complete block. Otherwise the
     driver_read function will overwrite something that should not be
     overwritten. */
  sfs_superblock *super = (sfs_superblock *)raw_superblock;

  /* open the disk image and get it ready to read/write blocks */
  driver_attach_disk_image(argv[1], 128);

  /* CHANGE THE FOLLOWING CODE SO THAT IT SEARCHES FOR THE SUPERBLOCK */
  
  /* search the first 10 blocks for the superblock */
  for(int i=0; i<10; i++)
  {
    driver_read(super,i);
    /* is it the filesystem superblock? */
    if(super->fsmagic == VMLARIX_SFS_MAGIC && !strcmp(super->fstypestr,VMLARIX_SFS_TYPESTR))
    {
      printf("superblock found at block %d!\n", i);
      i=20;
    }
    
    if(i == 9)
    {
      /* no superblock found*/
      printf("superblock is not found in first 10 blocks.\nI quit!\n");
    }
  }
  cout << "fsmagic: " << super->fsmagic << endl;
  cout << "fstypestr: " << super->fstypestr << endl;
  cout << "block_size: " << super->block_size << endl;
  cout << "sectorsperblock: " << super->sectorsperblock << endl;
  cout << "superblock: " << super->superblock << endl;
  cout << "num_blocks: " << super->num_blocks << endl;
  cout << "fb_bitmap: " << super->fb_bitmap << endl;
  cout << "fb_bitmapblocks: " << super->fb_bitmapblocks << endl;
  cout << "blocks_free: " << super->blocks_free << endl;
  cout << "num_inodes: " << super->num_inodes << endl;
  cout << "fi_bitmap: " << super->fi_bitmap << endl;
  cout << "fi_bitmapblocks: " << super->fi_bitmapblocks << endl;
  cout << "inodes_free: " << super->inodes_free << endl;
  cout << "inodes: " << super->inodes << endl;
  cout << "rootdir: " << super->rootdir << endl;
  cout << "open_count: " << super->open_count << endl;
  
  /* close the disk image */
  driver_detach_disk_image();

  return 0;
}
