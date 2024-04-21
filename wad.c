#include "wad.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define ERR_OPEN                               "Failed to open file"
#define ERR_INVALID_SIGNATURE                  "Invalid WAD signature"
#define ERR_READ_HEADER                        "Failed to read header"
#define ERR_SEEK_DESCRIPTORS                   "Failed to locate descriptors"
#define ERR_READ_ENTRY                         "Failed to read entry"
#define ERR_DIR_NOT_FOUND                      "Directory not found"
#define ERR_FILE_NOT_FOUND                     "File not found"
#define ERR_NESTED_MARKER                      "Marker contains nested entries"
#define ERR_INCOMPLETE_MARKER                  "Marker did not have necessary number of elements"
#define ERR_UNTERMINATED_NAMESPACE             "Namespace missing end"
#define ERR_ENTRY_CORRUPT                      "Attempting to read corrupted entry"
#define ERR_INVALID_OFFSET                     "Offset into content is larger than content"
#define ERR_INVALID_PATH                       "Specified invalid path"

static const char* last_error = NULL;

static struct mwad_node* new_node(struct mwad* pWad)
{
    struct mwad_node* pNode = pWad->next++;
    if(pWad->next == &pWad->last->nodes[MWAD_BLOCK_SIZE])
    {
        pWad->last->next = malloc(sizeof(struct mwad_block));
        pWad->last = pWad->last->next;
    }

    return pNode;
}

static struct mwad_node* from_entry(struct mwad* pWad, const char* pEntry)
{
    struct mwad_node* pNode = new_node(pWad);
    memcpy(pNode->name, pEntry + sizeof(uint32_t) * 2, 8);
    pNode->name[9] = 0;
    pNode->offset = *(uint32_t*)pEntry;
    pNode->size = *(uint32_t*)(pEntry + sizeof(uint32_t));
    pNode->down = NULL;
    pNode->next = NULL;
    return pNode;
}

static struct mwad_node* load_node(struct mwad* pWad, uint32_t* pCount)
{
    char entry[sizeof(uint32_t) * 2 + sizeof(char) * 8 + 1];
    entry[sizeof(entry) - 1] = 0;

    if(fread(entry, sizeof(entry) - 1, 1, pWad->file) < 1)
    {
        last_error = ERR_READ_ENTRY;
        return NULL;
    }

    --(*pCount);

    const char* pName = entry + sizeof(uint32_t) * 2;
    size_t len = strlen(pName);

    if(len == 4 && pName[0] == 'E' && pName[2] == 'M' && isdigit(pName[1]) && isdigit(pName[3]))
    {
        // Map marker

        struct mwad_node* pNode = from_entry(pWad, entry);
        struct mwad_node* pCurr = pNode;

        for(int i = 0; i < 10; ++i)
        {
            if(*pCount == 0)
            {
                last_error = ERR_INCOMPLETE_MARKER;
                return NULL;
            }

            struct mwad_node* pNext = load_node(pWad, pCount);
            if(!pNext) return NULL;

            if(pNext->size == 0)
            {
                last_error = ERR_NESTED_MARKER;
                return NULL;
            }

            pCurr->next = pNext;
            pCurr = pNext;
        }

        pNode->down = pNode->next;
        pNode->next = NULL;

        return pNode;
    }
    else if(len >= 7 && strcmp(pName + len - 6, "_START") == 0)
    {
        // Beginning of namespace

        struct mwad_node* pNode = from_entry(pWad, entry);
        pNode->name[len - 6] = 0;
        struct mwad_node* pCurr = pNode;

        while(*pCount)
        {
            struct mwad_node* pNext = load_node(pWad, pCount);
            if(!pNext) return NULL;

            if(pNext == pWad->node)
            {
                pNode->down = pNode->next;
                pNode->next = NULL;

                return pNode;
            }
            else
            {
                pCurr->next = pNext;
                pCurr = pNext;
            }
        }

        last_error = ERR_UNTERMINATED_NAMESPACE;
        return NULL;
    }
    else if(len >= 5 && len < 7 && strcmp(pName + len - 4, "_END") == 0)
    {
        // End of namespace

        return pWad->node;
    }
    else
    {
        // Content

        return from_entry(pWad, entry);
    }
}

struct mwad* mwad_open(const char* pPath)
{
    FILE* file = fopen(pPath, "r");
    if(!file)
    {
        last_error = ERR_OPEN;
        return NULL;
    }

    char sig[4];
    if(fread(sig, sizeof(char), 4, file) < 4 || sig[1] != 'W' || sig[2] != 'A' || sig[3] != 'D')
    {
        last_error = ERR_INVALID_SIGNATURE;
        goto close;
    }

    uint32_t count;
    uint32_t offset;

    if(fread(&count, sizeof(uint32_t), 1, file) < 1 || count == 0)
    {
        last_error = ERR_READ_HEADER;
        goto close;
    }

    if(fread(&offset, sizeof(uint32_t), 1, file) < 1)
    {
        last_error = ERR_READ_HEADER;
        goto close;
    }

    if(fseek(file, offset, SEEK_SET) != 0)
    {
        last_error = ERR_SEEK_DESCRIPTORS;
        goto close;
    }

    struct mwad* pWad = malloc(sizeof(struct mwad));
    pWad->file = file;
    pWad->offset = offset;
    pWad->block = malloc(sizeof(struct mwad_block));
    pWad->last = pWad->block;
    pWad->next = pWad->block->nodes;

    struct mwad_node* pCurr = pWad->node = load_node(pWad, &count);
    if(pCurr == NULL) goto abort;

    while(count)
    {
        struct mwad_node* pNode = load_node(pWad, &count);
        if(pNode == NULL) goto abort;

        pCurr->next = pNode;
        pCurr = pNode;
    }

    return pWad;

abort:;
    while(pWad->block != pWad->last)
    {
        struct mwad_block* pNext = pWad->block->next;
        free(pWad->block);
        pWad->block = pNext;
    }

    free(pWad->block);
    free(pWad);
close:;
    fclose(file);
    return NULL;
}

/**
 * \brief          Recursively finds the first node in a directory
 * \param[in]      pNode: Node to start searching from
 * \param[in]      pPath: String of path
 * \return         First node in directory or NULL
 */
static struct mwad_node* search_directory(struct mwad_node* pNode, const char* pPath)
{
    if(strlen(pPath) == 0) return pNode;

    while(pNode)
    {
        size_t len = strlen(pNode->name);

        if(strncmp(pNode->name, pPath, len) == 0)
        {
            const char* pNext = pPath + len;
            if(pNext[0] == 0) return pNode->down;

            return search_directory(pNode->down, pNext + 1);
        }

        pNode = pNode->next;
    }

    last_error = ERR_DIR_NOT_FOUND;
    return NULL;
}

/**
 * \brief          Finds the specified file in a directory
 * \param[in]      pWad: wad structure
 * \param[in]      pPath: String of path
 * \return         Node matching path or NULL
 */
static struct mwad_node* search_file(struct mwad* pWad, const char* pPath)
{
    const char* pBasePath = pPath;
    size_t len = strlen(pPath);

    // Find last slash in path
    const char* pDelim = pPath;
    while(pDelim)
    {
        pPath = pDelim + 1;
        pDelim = memchr(pPath, '/', len);
    }

    // Create new path excluding file name
    size_t delta = pPath - pBasePath;
    char buf[delta + 1];
    memcpy(buf, pBasePath, delta);
    buf[delta] = 0;

    struct mwad_node* pNode = search_directory(pWad->node, buf + 1);
    if(!pNode) return NULL;

    while(pNode)
    {
        if(strcmp(pNode->name, pPath) == 0) return pNode;
        pNode = pNode->next;
    }

    last_error = ERR_FILE_NOT_FOUND;
    return NULL;
}

int mwad_is_directory(struct mwad* pWad, const char* pPath)
{
    return search_directory(pWad->node, pPath + 1) != NULL;
}

uint32_t mwad_get_file(struct mwad* pWad, const char* pPath)
{
    struct mwad_node* pNode = search_file(pWad, pPath);
    if(pNode) return pNode->size;
    return 0;
}

char* mwad_list_directory(struct mwad* pWad, const char* pPath, int* pCount)
{
    struct mwad_node* pNode = search_directory(pWad->node, pPath + 1);
    if(!pNode) return NULL;

    *pCount = 0;
    size_t length = 0;
    struct mwad_node* pIter = pNode;

    while(pIter)
    {
        ++(*pCount);
        length += strlen(pIter->name);
        pIter = pIter->next;
    }

    char* pBuffer = malloc(length + *pCount);
    char* pList = pBuffer;

    while(pNode)
    {
        size_t len = strlen(pNode->name);
        memcpy(pBuffer, pNode->name, len + 1);
        pBuffer += len + 1;

        pNode = pNode->next;
    }

    return pList;
}

size_t mwad_read(struct mwad* pWad, const char* pPath, size_t offset, size_t size, char* pBuffer)
{
    struct mwad_node* pNode = search_file(pWad, pPath);
    if(!pNode) return 0;

    if(offset >= pNode->size)
    {
        last_error = ERR_INVALID_OFFSET;
        return 0;
    }

    if(offset + size > pNode->size)
    {
        size = pNode->size - offset;
    }

    if(fseek(pWad->file, pNode->offset + offset, SEEK_SET) != 0)
    {
        last_error = ERR_ENTRY_CORRUPT;
        return 0;
    }

    return fread(pBuffer, sizeof(uint8_t), size, pWad->file);
}

void mwad_close(struct mwad* pWad)
{
    while(pWad->block != pWad->last)
    {
        struct mwad_block* pNext = pWad->block->next;
        free(pWad->block);
        pWad->block = pNext;
    }

    free(pWad->block);
    fclose(pWad->file);
    free(pWad);
}

const char* mwad_last_error()
{
    if(!last_error) return NULL;

    const char* pErr = last_error;
    last_error = NULL;
    return pErr;
}
