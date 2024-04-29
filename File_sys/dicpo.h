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


void GetFileBlock(sfs_inode_t * node, size_t blockNumber, void* data);