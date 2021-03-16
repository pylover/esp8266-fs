#include "fs.h"
#include "debug.h"

#include <spi_flash.h>


ICACHE_FLASH_ATTR
fs_err_t fs_format() {
    fs_err_t err;
    uint16_t s;
    for (s = FS_SECTOR_FAT_START; s <= FS_SECTOR_FAT_END; s++) {
        INFO("Erasing sector: %04X", s);
        err = spi_flash_erase_sector(s);
        if (err != SPI_FLASH_RESULT_OK) {
        	ERROR("Canot erase sector: %d, err: %d\r\n", s, err);
        	return FS_ERR_FAT_ERASE;
        }
    }
    return FS_OK;
}

#define NS      64
#define SS      4096


static ICACHE_FLASH_ATTR
fs_err_t _node_iter(struct file *f, fs_node_cb_t cb) {
    fs_err_t err;
    struct fs_node node;
    uint8_t i;
    uint16_t s;
    uint32_t nodeaddr;

    for (s = FS_SECTOR_FAT_START; s <= FS_SECTOR_FAT_END; s++) {
        for (i = 0; i < FS_FAT_NODES_PER_SECTOR; i++) {
            system_soft_wdt_feed();
            nodeaddr = s * SS + i * NS;
	        if (spi_flash_read(nodeaddr, (uint32_t*)&node, NS)) {
                return FS_ERR_FAT_READ;
            }
            DEBUG("Iter: %s => sector: 0x%04X node: %02d", f->name, s, i);
            node.addr = nodeaddr;
            err = cb(f, &node);
            
            /* Requested to break */
            if (err == FS_OK) {
                os_memcpy(f, &node, NS);
                return FS_OK;
            }
            /* Requested Next node */
            else if (err == FS_ERR_ITER_NEXT) {
                continue;
            }
            else if (err) {
                return err;
            }
        }
    }
    return FS_ERR_ITER_END;
}


static ICACHE_FLASH_ATTR
fs_err_t _exactmatch_cb(struct file *f, struct fs_node *n) {
    if (os_strncmp(f->name, n->name, FS_FILENAME_MAX) == 0) {
        CHK("found");
        return FS_OK;
    }
    return FS_ERR_ITER_NEXT;
}


//ICACHE_FLASH_ATTR
//fs_err_t fs_get(struct file *f) {
//    fs_err_t err;
//    struct fs_node node;
//    uint8_t i;
//    uint16_t s;
//    for (s = FS_SECTOR_FAT_START; s <= FS_SECTOR_FAT_END; s++) {
//        for (i = 0; i < FS_FAT_NODES_PER_SECTOR; i++) {
//            system_soft_wdt_feed();
//	        if (spi_flash_read(s * SS + i * NS, (uint32_t*)&node, NS)) {
//                return FS_ERR_FAT_READ;
//            }
//            DEBUG("Search: %s => sector: 0x%04X node: %02d", f->name, s, i);
//            if (os_strncmp(f->name, node.name, FS_FILENAME_MAX) == 0) {
//                CHK("found");
//                os_memcpy(f, &node, NS);
//                return FS_OK;
//            }
//        }
//    }
//    return FS_ERR_FILENOTFOUND;
//}


ICACHE_FLASH_ATTR
fs_err_t fs_new(struct file *f) {
    fs_err_t err;

    /* check if file already exists */
    err = _node_iter(f, _exactmatch_cb);
    if (err == FS_OK) {
        return FS_ERR_FILE_EXISTS;
    }
    if (err != FS_ERR_ITER_END) {
        return FS_ERR_FILE_NOTFOUND;
    }
    
    /* TODO: loop over fat nodes to find a free node to store file. */
    /* TODO: set filesize to zero */
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



