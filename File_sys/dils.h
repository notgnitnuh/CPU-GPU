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
#include <ctime>

// FIXME: Print functions for testing, remove once done
void PrintSuperblock(sfs_superblock *super);
void PrintInode(sfs_inode inode);
void PrintDir(sfs_dirent dir);
void PrintBlock(uint32_t block);
void PrintBitMap(bitmap_t *bitmap);


void LongListing(sfs_superblock *super);
void ShortListing(sfs_superblock *super);
void ReadDirectories(uint32_t block, sfs_dirent* dirents);
bool CheckNULL(char c);
void PrintInodeLongList(sfs_inode inode, sfs_superblock *super, uint32_t inode_num);
void GetFileName(char* fname, uint32_t inode, sfs_superblock *super, uint32_t *start_block);