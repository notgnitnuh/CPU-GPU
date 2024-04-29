#include "dicpo.h"

int main()
{
    // find inode relating to desired file
    // read in a block at a time
    // write block to new file
    return 1;
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
