#include "dils.h"
#include <fcntl.h>

using namespace std;

int main(int argc, char **argv)
{
  /* declare a buffer that is the same size as a filesystem block */
  char raw_superblock[128];

  /* Check command line args, should receive disk image and optional
     -l for "long" listing. */
  if(!(argc == 2 || (argc == 3 && strstr(argv[2], "-l") )))
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
  
  /* search the first 10 blocks for the superblock */
  for(int i=0; i<10; i++)
  {
    driver_read(super,i);

    /* is it the filesystem superblock? */
    if(super->fsmagic == VMLARIX_SFS_MAGIC && !strcmp(super->fstypestr,VMLARIX_SFS_TYPESTR))
    {
      printf("superblock found at block %d!\n", i);
      PrintSuperblock(super); //FIXME: remove after testing
      if(argc == 2)
        ShortListing(super);
      else
        LongListing(super);
      i=20;
    }
    
    /* Give up after 10 blocks checked. */
    if(i == 9)
      printf("superblock is not found in first 10 blocks.\nI quit!\n");
  }
  
  /* close the disk image */
  driver_detach_disk_image();

  return 0;
}


void LongListing(sfs_superblock *super)
{
  char raw_block[2][128];
  sfs_inode *inode = (sfs_inode*)raw_block[0]; // Grabs second inode from block 
  sfs_dirent *dir = (sfs_dirent *)raw_block[1];
  sfs_dirent dirents[4];
  char *name = "add name";

  // int fo = open("test.txt", O_RDWR);
  // dup2(fo,1);

  /* Print all Inodes. 2 inodes per block, check for odd number 
     FIXME: skipping initial dir at . for some reason (does it have inode?)
   */
  for(int i=0; i<super->num_inode_blocks - super->inodes_free/2-1; i++)
  {
    driver_read(inode, super->inodes+i);
    PrintInodeLongList(inode[0], name);
    PrintInodeLongList(inode[1], name);
  }
  driver_read(inode, super->inodes + super->num_inode_blocks - super->inodes_free/2 - 1 );
  PrintInodeLongList(inode[0], name);
  if((super->num_inodes - super->inodes_free) % 2 != 1)
    PrintInodeLongList(inode[1], name);
}

void PrintInodeLongList(sfs_inode inode, char* f_name)
{
  string perms = "drwxrwxrwx";
  uint16_t mask = 0b100000000;
  time_t t = (time_t)inode.atime;
  string atime = (string)ctime(&t);
  
  if(inode.type == FT_DIR)
    cout << "d";
  else 
    cout << "-";

  for(int i=1; i<10; i++)
  {
    if((mask & inode.perm) == 0)
      cout << '-';
    else
      cout << perms[i];
    mask = mask >> 1;
  }
  cout << " " << right << setw(2) << inode.refcount;
  cout << setw(7) << inode.owner;
  cout << setw(7) << inode.group;
  cout << setw(8) << inode.size;
  cout << " " << atime.substr(4,12) << atime.substr(19,5);
  cout << " " << left << f_name << endl;
}

void GetFileBlock(sfs_inode_t * node, size_t blockNumber, void* data)
{
    uint32_t ptrs[128];

    // direct
    if (blockNumber < 5)
    {
        //printf("pass\n");
        driver_read(data, node->direct[blockNumber]);
    }
    // indirect
    else if (blockNumber < (5 + 32))
    {
        driver_read(ptrs, node->indirect);
        driver_read(data, ptrs[blockNumber - 5]);
    }
    // double indirect
    else if (blockNumber < (5 + 32 + (32 * 32)))
    {
        driver_read(ptrs, node->dindirect);
        int tmp = (blockNumber - 5 - 32) / 32;
        driver_read(ptrs, ptrs[tmp]);
        tmp = (blockNumber - 5 - 32) % 32;
        driver_read(data, ptrs[tmp]);
    }
    // triple indirect
    else if (blockNumber < (5 + 32 + (32*32) + (32*32*32)))
    {
        driver_read(ptrs, node->tindirect);
        int tmp = (blockNumber - 5 - 32 - (32*32)) / (32*32);
        driver_read(ptrs, ptrs[tmp]);
        tmp = ((blockNumber - 5 - 32 - (32* 32)) / 32) % 32;
        driver_read(ptrs, ptrs[tmp]);
        tmp = (blockNumber - 5 - 32 - (32 * 32)) % 32;
        driver_read(data, ptrs[tmp]);
    }
    else
    {
        printf("Error in block fetch, out of range\n");
        exit(1);
    }
}

void ShortListing(sfs_superblock *super)
{
  char raw_block[128];
  bool new_entry = true;
  uint32_t block = super->rootdir;
  sfs_dirent dirents[4];
  sfs_inode *inode = (sfs_inode*)raw_block; // Grabs second inode from block 

  // Read a block into 4 sfs_dirents 
  for(int i=0; i<4; i++)
  {
    driver_read(raw_block, block);//+j);
    for(int j=0; j<28; j++)
      dirents[i].name[j] = raw_block[i*32+j];
    dirents[i].inode = raw_block[i*32+31]<<24 | raw_block[i*32+30] << 16 | raw_block[i*32+29] << 8 | raw_block[i*32+28];
    PrintDir(dirents[i]);
    driver_read(inode, super->inodes + dirents[i].inode);
    PrintInode(inode[0]);
  }
}

void ReadDirectories(uint32_t node, sfs_dirent* dirents)
{
  char raw_block[128];
  int j=0, k=0;
  
  // FIXME: make this thing work, having issues with null spaces
  driver_read(raw_block, node);
  for(int i=0; i<4;i++)
  {
    k=j;
    // Load nonNULL chars
    while(!CheckNULL(raw_block[j]))
    {
      dirents[i].name[j-k] = raw_block[j];
      j++;
    }
    // add NULL terminator & inode number
    dirents[i].name[j-k] = NULL;
    j++;
    dirents[i].inode = raw_block[j];
    j++;

    // Cycle through to 
    while(CheckNULL(raw_block[j]))
    {
      j++;
      if(j>64)
        i=5;
    }
  }
}

bool CheckNULL(char c)
{
  for(uint8_t i= 0;i<31; i++)
  {
    if(c == i)
      return true;
  }
  return false;
}

void PrintBitMap(bitmap_t *bitmap)
{
  uint32_t temp;
  cout << BITMAP_BITS << endl;
  for(int i=0; i<BITMAP_BITS; i++)
  {
    temp = get_bit(bitmap, i);
    cout << temp;
  }
}
/* Print the contents of a single block*/
void PrintBlock(uint32_t block)
{
  char raw_block[128];
  bool new_line = true;

  driver_read(raw_block, block);
  for(int i=0; i<128; i++)
  {
    cout << raw_block[i];
  }
  cout << endl;
}
/* This function outputs the super blocks info, purely for testing */
void PrintSuperblock(sfs_superblock *super)
{
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
  cout << "num_inode_blocks: " << super->num_inode_blocks << endl;
  cout << "inodes: " << super->inodes << endl;
  cout << "rootdir: " << super->rootdir << endl;
  cout << "open_count: " << super->open_count << endl << endl;
}
void PrintInode(sfs_inode inode)
{
  cout << "owner: " << inode.owner << endl;
  cout << "group: " << inode.group << endl;
  cout << "ctime: " << inode.ctime << endl;
  cout << "mtime: " << inode.mtime << endl;
  cout << "atime: " << inode.atime << endl;
  cout << "perm: " << inode.perm << endl;
  cout << "type: " << inode.type << endl;
  cout << "refcount: " << inode.refcount << endl;
  cout << "size: " << inode.size << endl;
  cout << "direct: ";
  for(int i=0; i<5; i++)
   cout << inode.direct[i] << " ";
  cout << endl;
  cout << "indirect: " << inode.indirect << endl;
  cout << "dindirect: " << inode.dindirect << endl;
  cout << "tindirect: " << inode.tindirect << endl << endl;
}

void PrintDir(sfs_dirent dir)
{
  cout << "name: " << dir.name << endl;
  cout << "inode: " << dir.inode << endl << endl;
}