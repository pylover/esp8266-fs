#include "fs.h"
#include "debug.h"
#include "httpd.h"

#include <mem.h>
#include <spi_flash.h>


#define NODE_ADDR(i) \
    ((FS_SECTOR_START * FS_SECTOR_SIZE) + ((i) * FS_NODE_SIZE))


ICACHE_FLASH_ATTR
fs_err_t fs_format() {
    fs_err_t err = FS_OK;
    uint16_t s;
    uint32_t *tmp = os_zalloc(FS_SECTOR_SIZE);

    /* Erase sectors */
    for (s = FS_SECTOR_START; s <= FS_FAT_SECTOR_LAST; s++) {
        INFO("Erasing sector: 0X%03X", s);
        err = spi_flash_erase_sector(s);
        if (err != SPI_FLASH_RESULT_OK) {
        	ERROR("Canot erase sector: %d, err: %d\r\n", s, err);
        	err = FS_ERR_FAT_ERASE;
            break;
        }
        
        err = spi_flash_write(s * FS_SECTOR_SIZE, tmp, FS_SECTOR_SIZE);
        if (err != SPI_FLASH_RESULT_OK) {
        	ERROR("Canot write sector: %d, err: %d\r\n", s, err);
        	err = FS_ERR_FAT_ERASE;
            break;
        }
    }
    os_free(tmp);
    return err;
}


static ICACHE_FLASH_ATTR
fs_err_t _node_iter(uint8_t filter, fs_node_cb_t cb, void *arg) {
    fs_err_t err;
    fs_err_t retval = FS_ERR_ITER_END;
    uint16_t i;
    uint32_t s;
    struct fs_node *t = (struct fs_node *)os_zalloc(FS_SECTOR_SIZE);
    struct fs_node *n;

    for (s = FS_SECTOR_START; s <= FS_FAT_SECTOR_LAST; s++) {
        /* Read sector: 0x%03X */
        err = spi_flash_read(s * FS_SECTOR_SIZE, (uint32_t*)t, FS_SECTOR_SIZE);
        if (err != SPI_FLASH_RESULT_OK) {
        	ERROR("Canot read sector: 0x%03X, err: %d", s, err);
        	retval = FS_ERR_FAT_ERASE;
            break;
        }
        /* Loop over nodes */
        for (i = 0; i < FS_NODES_PER_SECTOR; i++) {
            n = t + i;
            
            /* Filter: %d %d */
            if ( !((n->flags & filter) || (n->flags == filter)) ) {
                continue;
            }
            
            /* Calculate Id */
            n->id = (FS_NODES_PER_SECTOR * (s - FS_SECTOR_START)) + i;
            DEBUG("Sector: 0x%03X id: %3u flags: %u size :%u p: %p", 
                    s, n->id, n->flags, n->size, n);

            /* Callback: %d */
            err = cb(n, arg);

            /* Requested Next node */
            if (err == FS_ERR_ITER_NEXT) {
                continue;
            }
 
            if (err == FS_SAVE) {
                CHK("Requested to save then break: id: %d flags: %d", n->id, 
                        n->flags);
                
                err = spi_flash_erase_sector(s);
                if (err != SPI_FLASH_RESULT_OK) {
                	ERROR("Canot erase sector: %d, err: %d\r\n", s, err);
                	retval = FS_ERR_SECTOR_ERASE;
                    break;
                }
	            if (spi_flash_write(s * FS_SECTOR_SIZE, (uint32_t*)t, 
                            FS_SECTOR_SIZE)) {
                    retval = FS_ERR_WRITE_NODE;
                    break;
                }
                retval = FS_OK;
                break;
            }
            
            retval = err;
            break;
        }
    }
    os_free(t);
    return retval;
}


static ICACHE_FLASH_ATTR
fs_err_t _namematch_cb( struct fs_node *n, void *arg) {
    struct fs_file *f = (struct fs_file*) arg;
    if (os_strncmp(f->node.name, n->name, FS_FILENAME_MAX) == 0) {
        /* found */
        return FS_OK;
    }
    return FS_ERR_ITER_NEXT;
}


static ICACHE_FLASH_ATTR
fs_err_t _allocate_cb(struct fs_node *n, void *arg) {
    struct fs_file *f = (struct fs_file*) arg;
    os_memcpy(n->name, f->node.name, FS_FILENAME_MAX);
    n->flags = FS_NODE_ALLOCATED;
    n->size = 0;
    n->nextid = n->id;
    os_memcpy(&f->node, n, FS_NODE_SIZE);
    /* Just return Save on the first occurance */
    return FS_SAVE;
}


ICACHE_FLASH_ATTR
fs_err_t fs_new(struct fs_file *f) {
    fs_err_t err;

    err = _node_iter(FS_NODE_ALLOCATED, _namematch_cb, f);
    CHK("iter err: %d", err);
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
    
    CHK("Free node found: id: %d", f->node.id);
    return FS_OK;
}


ICACHE_FLASH_ATTR
fs_err_t fs_close(struct fs_file *f) {
    return FS_OK;
}


static ICACHE_FLASH_ATTR
fs_err_t _node_replace_with_next(struct fs_node *n) {
    uint16_t addr = FS_NODE_ADDR(n->nextid);
    return spi_flash_read(addr, (uint32_t*)n, sizeof(struct fs_node));
}


static ICACHE_FLASH_ATTR
fs_err_t _node_replace_with_last(struct fs_node *n) {
    fs_err_t err;

    if (FS_NODE_IS_LAST(n)) {
        return FS_OK;
    }
    
    err = _node_replace_with_next(n);
    if (err) {
        return err;
    }

    return _node_replace_with_last(n);
}

static ICACHE_FLASH_ATTR
fs_err_t _node_save(struct fs_node *n) {
    fs_err_t err;
    uint32_t sectaddr;
    err = spi_flash_read(sectaddr, (uint32_t*)buff, FS_SECTOR_SIZE);
    if (err) {
        break;
    }
    
    f->node.size += rb_read(b, buff + (FS_SECTOR_SIZE - sectavail), 
            MIN(avail, sectavail));
    
    err = spi_flash_erase_sector(sectaddr / FS_SECTOR_SIZE);
    if (err) {
        break;
    }
    err = spi_flash_write(sectaddr, (uint32_t*)buff, FS_SECTOR_SIZE);
    if (err) {
        break;
    }
}
 

ICACHE_FLASH_ATTR
fs_err_t fs_rbwrite(struct fs_file *f, struct ringbuffer *b) {
    fs_err_t err = FS_OK;
    uint16_t avail;
    uint16_t sectavail;
    uint32_t sectaddr;
    char *buff;
    struct fs_node n;

    buff = os_zalloc(FS_SECTOR_SIZE);
    while (true) {
        avail = RB_USED(b);
        if (!avail) {
            break;
        }
        os_memcpy(&n, &f->node, sizeof(struct fs_node));
        err = _node_replace_with_last(&n);
        if (err) {
            break;
        }

        sectavail = FS_SECTOR_SIZE - (f->node.size % FS_SECTOR_SIZE);
        sectaddr = FS_NODE_TARGET_ADDR(n.id);
        CHK("Writing: %d bytes, lastnode: %d lastsect: 0x%06X "
                "sectavail: %d filename: %s", 
                avail, n.id, sectaddr, sectavail, f->node.name);
        
        if (sectavail) {
            err = spi_flash_read(sectaddr, (uint32_t*)buff, FS_SECTOR_SIZE);
            if (err) {
                break;
            }
            
            f->node.size += rb_read(b, buff + (FS_SECTOR_SIZE - sectavail), 
                    MIN(avail, sectavail));
            
            err = spi_flash_erase_sector(sectaddr / FS_SECTOR_SIZE);
            if (err) {
                break;
            }
            err = spi_flash_write(sectaddr, (uint32_t*)buff, FS_SECTOR_SIZE);
            if (err) {
                break;
            }
            
            err = _node_save(&f->node);
            if (err) {
                break;
            }
        }
        
    }
    os_free(buff);
    return err;
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

