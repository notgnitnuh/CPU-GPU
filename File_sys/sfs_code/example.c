#include <driver.h>
#include <sfs_superblock.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main()
{
  /* declare a buffer that is the same size as a filesystem block */
  char raw_superblock[128];

  /* Create a pointer to the buffer so that we can treat it as a
     superblock struct. The superblock struct is smaller than a block,
     so we have to do it this way so that the buffer that we read
     blocks into is big enough to hold a complete block. Otherwise the
     driver_read function will overwrite something that should not be
     overwritten. */
  sfs_superblock *super = (sfs_superblock *)raw_superblock;

  /* open the disk image and get it ready to read/write blocks */
  driver_attach_disk_image("initrd", 128);

  /* CHANGE THE FOLLOWING CODE SO THAT IT SEARCHES FOR THE SUPERBLOCK */
  
  /* read block 10 from the disk image */
  driver_read(super,10);
  /* is it the filesystem superblock? */
  if(super->fsmagic == VMLARIX_SFS_MAGIC &&
     ! strcmp(super->fstypestr,VMLARIX_SFS_TYPESTR))
    {
      printf("superblock found at block 10!\n");
    }
  else
    {
      /* read block 0 from the disk image */
      driver_read(super,0);
      if(super->fsmagic == VMLARIX_SFS_MAGIC &&
	 ! strcmp(super->fstypestr,VMLARIX_SFS_TYPESTR))
	{
	  printf("superblock found at block 0!\n");
	}
      else
	printf("superblock is not at block 10 or block 0\nI quit!\n");
    }
  
  /* close the disk image */
  driver_detach_disk_image();

  return 0;
}
