#include <stdio.h>

#include "wad.h"

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        printf("Invalid number of arguments\n");
        return 1;
    }

    struct mwad* pWad = mwad_open(argv[1]);
    if(!pWad)
    {
        printf("Failed to load WAD file: %s\n", mwad_last_error());
        return 1;
    }

    char buf[16] = { 0 };
    size_t result = mwad_read(pWad, "/mp.txt", 0, sizeof(buf) - 1, buf);
    if(result != sizeof(buf) - 1)
    {
        printf("Failed to read file (%ld): %s\n", result, mwad_last_error());
    }
    else
    {
        printf("%s\n", buf);
    }

    mwad_close(pWad);
    return 0;
}
