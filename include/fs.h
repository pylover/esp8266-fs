#ifndef FS_H
#define FS_H

#include <c_types.h>


/* Limits */
#define FS_FILENAME_MAX            56
#define FS_FAT_NODES_PER_SECTOR    64

#ifndef FS_SECTOR_START
  #if SPI_SIZE_MAP == 2
  #define FS_SECTOR_START       0x70   
  #define FS_SECTOR_END         0x79   
  #elif SPI_SIZE_MAP == 4
  #define FS_SECTOR_START       0x100  
  #define FS_SECTOR_END         0x200  
  #elif SPI_SIZE_MAP == 6
  #define FS_SECTOR_START       0x200  
  #define FS_SECTOR_END         0x300  
  #endif
#endif

#ifndef FS_FAT_SECTORS
#define FS_FAT_SECTORS      2
#endif

#ifndef FS_SECTOR_FAT_START
#define FS_SECTOR_FAT_START    FS_SECTOR_START
#endif

#define FS_SECTOR_FAT_END      (FS_SECTOR_START + (FS_FAT_SECTORS - 1)) 

#ifndef FS_PART_START_SECTOR     
#define FS_PART_START_SECTOR     (FS_SECTOR_START + FS_FAT_SECTORS)
#endif


#define FS_SIZE               (FS_SECTOR_END - FS_PART_START_SECTOR)
#define FS_SECTOR_SIZE        4092 


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



typedef uint8_t fs_err_t;


/* Do not modify the size */
struct fs_node {
    char name[FS_FILENAME_MAX];       // 56
    uint32_t addr;                    //  4
    uint16_t startsector;             //  2
    uint16_t size;                    //  2
};


struct file {
    char name[FS_FILENAME_MAX];       // 56
    uint32_t nodeaddr;                //  4
    uint16_t startsector;             //  2
    uint16_t size;                    //  2

    uint8_t status; 
    uint16_t bufflen;
    char *buff;
};


typedef fs_err_t (*fs_cb_t)(struct file *f);
typedef fs_err_t (*fs_node_cb_t)(struct file *f, struct fs_node *n);

// TODO:
// format
// new
// write
// Append
// list
// read
// flush

fs_err_t fs_format();
fs_err_t fs_new(struct file *f);
fs_err_t fs_write(struct file *f, char *data, uint16_t len);
//fs_err_t fs_get(struct file *f);
//fs_err_t fs_list(struct file *f, fscb_t cb);
//fs_err_t fs_search(struct file *f, char *word, uint8_t wlen, fscb_t cb);
//fs_err_t fs_append(struct file *f, char *data, filesize16_t len);
//fs_err_t fs_read(struct file *f, char *data, filesize16_t len);

#endif
