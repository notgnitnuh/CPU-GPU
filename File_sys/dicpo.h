extern "C"{
#include "sfs_code/driver.h"
#include "sfs_code/sfs_superblock.h"
#include "sfs_code/bitmap.h"
#include "sfs_code/sfs_dir.h"
#include "sfs_code/sfs_inode.h"
}
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

// FIXME: Print functions for testing, remove once done
void PrintSuperblock(sfs_superblock *super);
void PrintInode(sfs_inode inode);
void PrintDir(sfs_dirent dir);
void PrintBlock(uint32_t block);
void PrintBitMap(bitmap_t *bitmap);


void GetFileBlock(sfs_inode_t * node, size_t blockNumber, void* data);
bool GetInode(char* fname, sfs_superblock *super, sfs_dirent &dirent);
void ReadDirectories(uint32_t block, sfs_dirent* dirents);
void CopyToLocal(sfs_superblock* super, sfs_dirent dirent);