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

/**
 * It must be multiple of FS_SECTOR_SIZE
 */
#ifndef FS_SECTORS
#define FS_SECTORS                 128
#endif

#define FS_SECTOR_SIZE             4096 
#define FS_FILENAME_MAX            54
#define FS_NODE_SIZE               64


#define FS_FAT_SECTORS      (FS_SECTORS * FS_NODE_SIZE / FS_SECTOR_SIZE) 
#define FS_FAT_SECTOR_LAST  ((FS_FAT_SECTORS - 1) + FS_SECTOR_START)


/* Do not modify the size, it must be exactly 64 bytes. */
struct fs_node {
    char name[FS_FILENAME_MAX + 1]; // 55
    uint32_t size;                  //  4
    uint16_t id;                    //  2
    uint16_t nid;                   //  2
    uint8_t flags;                  //  1
};


struct fs_file {
    struct fs_node node;

    uint8_t status; 
    uint16_t bufflen;
    char *buff;
};


typedef uint8_t fs_err_t;
typedef fs_err_t (*fs_cb_t)(struct fs_file *f);
typedef fs_err_t (*fs_node_cb_t)(struct fs_file *f, struct fs_node *n);


/* Node match flags */
#define FS_NODE_FREE            0
#define FS_NODE_ALLOCATED       1

/* Errors */
#define FS_OK                     0
#define FS_ERR_FAT_ERASE         10
#define FS_ERR_FAT_READ          11
#define FS_ERR_FILE_EXISTS       12
#define FS_ERR_FILE_NOTFOUND     13
#define FS_ERR_ITER_NEXT         14
#define FS_ERR_ITER_END          15


/* File open statuses */
#define FS_FILESTATUS_IDLE      0
#define FS_FILESTATUS_READ      1
#define FS_FILESTATUS_WRITE     2
#define FS_FILESTATUS_APPEND    3



// TODO:
// format
// new
// write
// Append
// list
// read
// flush

fs_err_t fs_format();
fs_err_t fs_new(struct fs_file *f);
fs_err_t fs_write(struct fs_file *f, char *data, uint16_t len);
fs_err_t fs_close(struct fs_file *f);
//fs_err_t fs_get(struct fs_file *f);
//fs_err_t fs_list(struct fs_file *f, fscb_t cb);
//fs_err_t fs_search(struct fs_file *f, char *word, uint8_t wlen, fscb_t cb);
//fs_err_t fs_append(struct fs_file *f, char *data, filesize16_t len);
//fs_err_t fs_read(struct fs_file *f, char *data, filesize16_t len);

#endif
