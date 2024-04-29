#include "dicpo.h"

int main(int argc, char **argv)
{
    char raw_superblock[128];
    int inode;
    sfs_dirent dirent;

    if(argc != 3)
    {
        cout << "Usage \n  ./dicpo <disk.img> <filename>\n";
        exit(1);
    }

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
            // find inode relating to desired file
            if(GetInode(argv[2], super, dirent))
            {
                CopyToLocal(super, dirent);
            }
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

void CopyToLocal(sfs_superblock* super, sfs_dirent dirent)
{
    // Open output file
    int fo = open(dirent.name, O_CREAT | O_RDWR | O_TRUNC);
    dup2(fo,1);

    cout << "FIXME: test line\n" << endl;

    close(fo);

    // Use GetFileBlock using inode to write to output file.
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

/* Very slow and inefficient method of finding file names. I'm sure there is a better way to do this
   But my brain isn't working... so I guess it is what it is. */
bool GetInode(char* fname, sfs_superblock *super, sfs_dirent &dirent)
{
    sfs_dirent dirents[4];
    for(uint32_t i=super->rootdir; i<super->num_blocks; i++)
    {
        // Read a block into 4 sfs_dirents 
        ReadDirectories(i, dirents);
        for(int j=0; j<4; j++)
        {
            if(!strcmp(dirents[j].name, fname))
            {
                dirent = dirents[j];
                return true;
            }
        }
    }
    cout << "Directory not found" << endl;
    return false;
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