#include "fs.h"
#include "debug.h"

#include <mem.h>
#include <spi_flash.h>


#define NODE_ADDR(i) \
    ((FS_SECTOR_START * FS_SECTOR_SIZE) + ((i) * FS_NODE_SIZE))

#define NODE_READ(a, n)  \
    spi_flash_read(a, (uint32_t*)n, sizeof(struct fs_node))

#define NODE_WRITE(a, n)  \
    spi_flash_write(a, (uint32_t*)n, sizeof(struct fs_node))


static ICACHE_FLASH_ATTR
fs_err_t _node_iter(uint8_t filter, fs_node_cb_t cb, void *arg) {
    fs_err_t err;
    struct fs_node node;
    uint16_t id;
    uint32_t addr;
        
    for (id = 0; id < FS_SECTORS; id++) {
        addr = NODE_ADDR(id);
        os_memset(&node, 0, FS_NODE_SIZE);
        //DEBUG("Before read => id: %3u addr: 0X%06X flags: %u s:%u", id, addr, 
        //        node.flags, node.size);
	    if (NODE_READ(addr, &node)) {
            return FS_ERR_FAT_READ;
        }
        
        DEBUG("After read => id: %3u addr: 0X%06X flags: %u s:%u", id, addr, 
                node.flags, node.size);

        if ( !((node.flags & filter) || (node.flags == filter)) ) {
            continue;
        }

        /* Callback */
        node.id = id;
        err = cb(&node, arg);

        /* Requested Next node */
        if (err == FS_ERR_ITER_NEXT) {
            continue;
        }
 
        if (err == FS_SAVE) {
            CHK("Requested to save then break: 0x%06X flags: %d", addr, 
                    node.flags);
	        if (NODE_WRITE(addr, &node)) {
                return FS_ERR_WRITE_NODE;
            }
            return FS_OK;
        }

        /* Requested to break */
        if (err == FS_OK) {
            return FS_OK;
        }

       
        /* Error in user callback */
        if (err) {
            return err;
        }
        
    }
    return FS_ERR_ITER_END;
}


static ICACHE_FLASH_ATTR
fs_err_t _exactmatch_cb( struct fs_node *n, void *arg) {
    struct fs_file *f = (struct fs_file*) arg;
    if (os_strncmp(f->node.name, n->name, FS_FILENAME_MAX) == 0) {
        CHK("found");
        return FS_OK;
    }
    return FS_ERR_ITER_NEXT;
}


static ICACHE_FLASH_ATTR
fs_err_t _allocate_cb(struct fs_node *n, void *arg) {
    struct fs_file *f = (struct fs_file*) arg;
    os_memcpy(n->name, f->node.name, FS_FILENAME_MAX);
    n->flags = FS_NODE_ALLOCATED;
    n->size = 23000;
    os_memcpy(&f->node, n, FS_NODE_SIZE);
    CHK("Just return Save on the first occurance");
    return FS_SAVE;
}


ICACHE_FLASH_ATTR
fs_err_t fs_format() {
    fs_err_t err;
    uint16_t s;
    uint32_t tmp = os_zalloc(FS_SECTOR_SIZE);

    /* Erase sectors */
    for (s = FS_SECTOR_START; s <= FS_FAT_SECTOR_LAST; s++) {
        INFO("Erasing sector: 0X%03X", s);
        err = spi_flash_erase_sector(s);
        if (err != SPI_FLASH_RESULT_OK) {
        	ERROR("Canot erase sector: %d, err: %d\r\n", s, err);
        	return FS_ERR_FAT_ERASE;
        }
        
        err = spi_flash_write(s * FS_SECTOR_SIZE, tmp, FS_SECTOR_SIZE);
        if (err != SPI_FLASH_RESULT_OK) {
        	ERROR("Canot write sector: %d, err: %d\r\n", s, err);
        	return FS_ERR_FAT_ERASE;
        }
    }
    return FS_OK;
}



ICACHE_FLASH_ATTR
fs_err_t fs_new(struct fs_file *f) {
    fs_err_t err;

    /* check if file already exists */
    err = _node_iter(FS_NODE_ALLOCATED, _exactmatch_cb, f);
    if (err == FS_OK) {
        return FS_ERR_FILE_EXISTS;
    }
    if (err != FS_ERR_ITER_END) {
        return err;
    }
    
    /* loop over fat nodes to find a free node to store file. */
    err = _node_iter(FS_NODE_FREE, _allocate_cb, f);
    if (err) {
        return FS_ERR_NOSPACE;
    }
    
    /* Allocate buffer for file data */
    f->buff = os_zalloc(FS_SECTOR_SIZE);
    f->bufflen = 0;
    CHK("Free node found: id: %d", f->node.id);
    return FS_OK;
    
    /* TODO: set filesize to zero */
}


ICACHE_FLASH_ATTR
fs_err_t fs_close(struct fs_file *f) {
    os_free(f->buff);
    return FS_OK;
}
//static 
//void _write_sector(uint16_t len) {
//	SpiFlashOpResult err;
//
//	system_soft_wdt_feed();
//	system_upgrade_erase_flash(FOTA_SECTORSIZE);
//	//os_delay_us(100);
//	//os_printf("E: 0x%05X\r\n", fs.sector * FOTA_SECTORSIZE);
//
//	system_soft_wdt_feed();
//	char sector[FOTA_SECTORSIZE];
//    memset(sector, 0, FOTA_SECTORSIZE);
//
//	rb_safepop(&rb, sector, len);
//	system_upgrade(sector, len);
//	//os_delay_us(100);
//	total += len;
//	INFO("W: 0x%05X, len: %d, total: %d\r\n", 
//			fs.sector * FOTA_SECTORSIZE, len, total);
//	fs.sector++;
//	//err = spi_flash_write(fs.sector * FOTA_SECTORSIZE, 
//	//		sector, 
//	//		FOTA_SECTORSIZE);
//	//if (err != SPI_FLASH_RESULT_OK) {
//	//	os_printf("Canot write flash: %d\r\n", err);
//	//	return;
//	//}
//
//}
//
//
//int fota_feed(char * data, Size datalen, bool last) {
//	RingBuffer *b = &rb;
//	int err = rb_safepush(b, data, datalen);
//	if (err != RB_OK) {
//		return err;
//	}
//
//	while (rb_used(b) >= FOTA_SECTORSIZE) {
//		_write_sector(FOTA_SECTORSIZE);
//	}
//
//	if (last) {
//		_write_sector(rb_used(b));
//	}
//		
//	return RB_OK;
//}
//
//
//void fota_init() {
//	// Buffer
//	rb.blob = (char*) os_malloc(FOTA_BUFFERSIZE + 1);
//	rb_reset((RingBuffer*)&rb);
//	fs.sector = system_upgrade_userbin_check() == UPGRADE_FW_BIN1 ?
//		SYSTEM_PARTITION_OTA2_ADDR / FOTA_SECTORSIZE: 1;
//	
//	total = 0;
//	//system_soft_wdt_stop();
//	//wifi_fpm_close();
//	//bool fp = spi_flash_erase_protect_disable();
//	//if (!fp) {
//	//	os_printf("Cannot disable the flash protection\r\n");
//	//	return;
//	//}
//
//	system_upgrade_init();
//	system_upgrade_flag_set(UPGRADE_FLAG_START);
//	os_printf("FOTA: Init Sector: %X\r\n", fs.sector);
//}
//
//
//void fota_finalize() {
//	os_free(rb.blob);
//	os_printf("REBOOTING\r\n");
//	system_soft_wdt_feed();
//	system_upgrade_deinit();
//	system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
//	system_upgrade_reboot();
//}

