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
  sfs_inode *inode = (sfs_inode*)raw_block[0]; // Grabs two inodes from block
  sfs_dirent *dir = (sfs_dirent *)raw_block[1]; // Grab 4 dirs from block, no used, but removing causes seg fault?!?

  // int fo = open("test.txt", O_RDWR);
  // dup2(fo,1);

  /* Print all Inodes. 2 inodes per block, check for odd number 
     FIXME: skipping initial dir at . for some reason (does it have inode?)
   */
  for(int i=0; i<super->num_inode_blocks - super->inodes_free/2-1; i++)
  {
    driver_read(inode, super->inodes+i);
    PrintInodeLongList(inode[0], super, i*2);
    PrintInodeLongList(inode[1], super, i*2+1);
  }
  driver_read(inode, super->inodes + super->num_inode_blocks - super->inodes_free/2 - 1 );
  if((super->num_inodes - super->inodes_free) % 2 != 1)
  {
    PrintInodeLongList(inode[0], super, super->num_inodes - super->inodes_free-2);
    PrintInodeLongList(inode[1], super, super->num_inodes - super->inodes_free-1);
  }
  else
    PrintInodeLongList(inode[0], super, super->num_inodes - super->inodes_free-1);
    
}

void PrintInodeLongList(sfs_inode inode, sfs_superblock *super, uint32_t inode_num)
{
  char* f_name;
  string perms = "drwxrwxrwx";
  uint16_t mask = 0b100000000;
  time_t t = (time_t)inode.atime;
  string atime = (string)ctime(&t);
  
  GetFileName(f_name, inode_num, super, &super->rootdir);

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
  int num_files = super->num_inodes - super->inodes_free;
  uint32_t start_block=super->rootdir;
  char fname[28];
  sfs_inode *inode = (sfs_inode*)raw_block; // Grabs second inode from block 
  // bitmap_t *test = (bitmap_t*)raw_block;

  // Loop for number of used inodes
  for(int i=0; i<super->num_inodes-super->inodes_free; i++)
  {
    // Read a block into 4 sfs_dirents 
    GetFileName(fname, i, super, &start_block);
    cout << fname << endl;
  }
}

/* Very slow and inefficient method of finding file names. I'm sure there is a better way to do this
   But my brain isn't working... so I guess it is what it is. */
void GetFileName(char* fname, uint32_t inode, sfs_superblock *super, uint32_t *start_block)
{
  sfs_dirent dirents[4];
  strcpy(fname, "nada");
  for(uint32_t i=*start_block; i<super->num_blocks; i++)
  {
    // Read a block into 4 sfs_dirents 
    ReadDirectories(i, dirents);
    for(int j=0; j<4; j++)
    {
      if(dirents[j].inode == inode)
      {
        // fname = dirents[j].name;
        strcpy(fname, dirents[j].name);
        return;
      }
    }
  }
}

/* Functions below this point are for testing by priting structures*/
void PrintBitMap(bitmap_t *bitmap)
{
  uint32_t temp;
  uint32_t test = 416;
  
  for(int i=31; i>=0; i--)
  {
    temp = get_bit(bitmap, i);
    cout << temp;
  }
}

void ReadDirectories(uint32_t block, sfs_dirent* dirents)
{
  char raw_block[128];
  int j=0, k=0;
  
  // Read a block into 4 sfs_dirents 
  for(int i=0; i<4; i++)
  {
    driver_read(raw_block, block);//+j);
    for(int j=0; j<28; j++)
      dirents[i].name[j] = raw_block[i*32+j];
    dirents[i].inode = raw_block[i*32+31]<<24 | raw_block[i*32+30] << 16 | raw_block[i*32+29] << 8 | raw_block[i*32+28];
    // PrintDir(dirents[i]);
    // driver_read(inode, super->inodes + dirents[i].inode);
    // PrintInode(inode[0]);
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
  cout << "inode: " << dir.inode << endl;
}