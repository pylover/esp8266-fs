#ifndef FS_H
#define FS_H

#include <c_types.h>


/* Limits */

/**
 * Start sector.
 */
#ifndef FS_SECTOR_START
  #if SPI_SIZE_MAP == 2
  #define FS_SECTOR_START        0x70   
  #elif SPI_SIZE_MAP == 4
  #define FS_SECTOR_START        0x100  
  #elif SPI_SIZE_MAP == 6
  #define FS_SECTOR_START        0x200  
  #endif
#endif

#ifndef FS_SECTORS
#define FS_SECTORS                 64
#endif

#define FS_SECTOR_SIZE             4096 
#define FS_NODE_SIZE               64
#define FS_NODES_PER_SECTOR        64

#define FS_FAT_SECTORS      (FS_SECTORS * FS_NODE_SIZE / FS_SECTOR_SIZE) 
#define FS_FAT_SECTOR_LAST  ((FS_FAT_SECTORS - 1) + FS_SECTOR_START)

/* Some utilities */
#define FS_NODE_ADDR(id)   (((id) * FS_NODE_SIZE) + \
        (FS_SECTOR_START * FS_SECTOR_SIZE))
#define FS_NODE_TARGET_SECTOR(id)   ((id) + FS_FAT_SECTOR_LAST + 1)
#define FS_NODE_TARGET_ADDR(id)   (FS_NODE_TARGET_SECTOR(id) * FS_SECTOR_SIZE)
#define FS_NODE_IS_LAST(n) ((n)->id == (n)->nextid)


/* Node match flags */
#define FS_NODE_FREE            0
#define FS_NODE_ALLOCATED       1

/* Errors */
#define FS_OK                     0
#define FS_SAVE                   1
#define FS_ERR_FAT_ERASE         10
#define FS_ERR_FAT_READ          11
#define FS_ERR_FILE_EXISTS       12
#define FS_ERR_FILE_NOTFOUND     13
#define FS_ERR_ITER_NEXT         14
#define FS_ERR_ITER_END          15
#define FS_ERR_NOSPACE           16
#define FS_ERR_WRITE_NODE        17
#define FS_ERR_SECTOR_ERASE      18


/* File open statuses */
#define FS_FILESTATUS_IDLE      0
#define FS_FILESTATUS_READ      1
#define FS_FILESTATUS_WRITE     2
#define FS_FILESTATUS_APPEND    3


// TODO:
// new
// write
// Append
// list
// read
// flush


/* Do not modify the definition order & size, it must be exactly 64 bytes. */
#define FS_FILENAME_MAX            54

struct fs_node {
    char name[FS_FILENAME_MAX + 1]; // 55
    uint8_t flags;                  //  1
    uint32_t size;                  //  4
    uint16_t id;                    //  2
    uint16_t nextid;                //  2
};


struct fs_file {
    struct fs_node node;

    uint8_t status; 
};


typedef uint8_t fs_err_t;
typedef fs_err_t (*fs_cb_t)(struct fs_file *f);
typedef fs_err_t (*fs_node_cb_t)(struct fs_node *n, void *args);



fs_err_t fs_format();
fs_err_t fs_new(struct fs_file *f);
fs_err_t fs_write(struct fs_file *f, const char *tmp, uint16_t len);
fs_err_t fs_close(struct fs_file *f);
//fs_err_t fs_get(struct fs_file *f);
//fs_err_t fs_list(struct fs_file *f, fscb_t cb);
//fs_err_t fs_search(struct fs_file *f, char *word, uint8_t wlen, fscb_t cb);
//fs_err_t fs_append(struct fs_file *f, char *data, filesize16_t len);
//fs_err_t fs_read(struct fs_file *f, char *data, filesize16_t len);

#endif
