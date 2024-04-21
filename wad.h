#ifndef MWAD_WAD_H
#define MWAD_WAD_H

#include <stdint.h>
#include <stdio.h>

#ifndef MWAD_BLOCK_SIZE
#define MWAD_BLOCK_SIZE 64
#endif

/**
 * \brief          Represents a node in the file system
 */
struct mwad_node
{
    char name[9];
    uint32_t offset;                           /** Offset of lump data for content or map marker, zero otherwise */
    uint32_t size;                             /** Size of lump data for content, zero otherwise */
    struct mwad_node* down;                    /** Pointer to child node for namespaces or markers, NULL otherwise */
    struct mwad_node* next;                    /** Next node in file system */
};

/**
 * \brief          Dynamic linked list storing preallocated blocks of memory for nodes of size MWAD_BLOCK_SIZE each
 */
struct mwad_block
{
    struct mwad_node nodes[MWAD_BLOCK_SIZE];
    struct mwad_block* next;
};

/**
 * \brief          Core WAD file structure
 */
struct mwad
{
    FILE* file;
    uint32_t offset;                           /** Offset of descriptors in WAD file */
    struct mwad_node* node;
    struct mwad_block* block;                  /** Initial node block */
    struct mwad_block* last;                   /** The last node block, the current one in use */
    struct mwad_node* next;                    /** Pointer to the first unused node in block */
};

/**
 * \brief          Loads a WAD file from a given path
 * \param[in]      pPath: Path of WAD file
 * \return         Initialized mwad structure
 */
struct mwad* mwad_open(const char* pPath);

/**
 * \brief          Lists all entries in a directory
 * \param[in]      pWad: mwad structure
 * \param[in]      pPath: Path of directory to list, with or without trailing slash
 * \param[out]     pCount: Number of directories found, if successful
 * \return         Character array containing entries separated by null terminators
 */
char* mwad_list_directory(struct mwad* pWad, const char* pPath, int* pCount);

/**
 * \brief          Reads a file
 * \param[in]      pWad: mwad structure
 * \param[in]      pPath: Path of file
 * \param[in]      offset: Offset to start reading file at
 * \param[in]      size: Number of bytes to read
 * \param[inout]   pBuffer: Buffer to write to
 * \return         Number of bytes read, or zero if error occured
 */
size_t mwad_read(struct mwad* pWad, const char* pPath, size_t offset, size_t size, char* pBuffer);

/**
 * \brief          Deconstructs mwad structure
 * \param[in]      pWad: mwad structure to deinitialize
 */
void mwad_close(struct mwad* pWad);

/**
 * \brief          Retrieves the last error message
 * \return         Last error message, or NULL
 */
const char* mwad_last_error();

#endif
