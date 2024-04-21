#define FUSE_USE_VERSION 31

#include <errno.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wad.h"

struct mwad* wad;

static int do_getattr(const char* pPath, struct stat* pStat)
{
    int result;

    if(mwad_is_directory(wad, pPath))
    {
        pStat->st_mode = S_IFDIR | 0555;
        pStat->st_nlink = 2;

        return 0;
    }
    else if((result = mwad_get_file(wad, pPath)))
    {
        pStat->st_mode = S_IFREG | 0555;
        pStat->st_nlink = 1;
        pStat->st_size = result;

        return 0;
    }
    else
    {
        return -ENOENT;
    }
}

static int do_read(const char* pPath, char* pBuffer, size_t size, off_t offset, struct fuse_file_info* pFile)
{
    return mwad_read(wad, pPath, offset, size, pBuffer);
}

static int do_readdir(const char* pPath, void* pBuffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* pFile)
{
    int count;
    char* pBuf = mwad_list_directory(wad, pPath, &count);
    if(!pBuf) return -ENOENT;

    filler(pBuffer, ".", NULL, 0);
    filler(pBuffer, "..", NULL, 0);

    char* pCurr = pBuf;
    while(count--)
    {
        filler(pBuffer, pCurr, NULL, 0);
        pCurr += strlen(pCurr) + 1;
    }

    free(pBuf);
    return 0;
}

static const struct fuse_operations operations = {
    .getattr = do_getattr,
    .read = do_read,
    .readdir = do_readdir
};

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        printf("Incorrect usage\n");
        return 1;
    }

    wad = mwad_open(argv[argc - 2]);
    argv[argc - 2] = argv[argc - 1];
    --argc;

    int ret = fuse_main(argc, argv, &operations, NULL);

    mwad_close(wad);
    return ret;
}
