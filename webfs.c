#include "httpd.h"
#include "fs.h"


static ICACHE_FLASH_ATTR
httpd_err_t webfs_format(struct httpd_session *s) {
    fs_err_t err = fs_format();
    if (err) {
        return err;
    }
    return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, "Format Successfull"CR, 20);
}


static ICACHE_FLASH_ATTR
httpd_err_t webfs_post(struct httpd_session *s) {
    httpd_err_t err;
    struct fs_file *f;
    size32_t more = HTTPD_REQUESTBODY_REMAINING(s);
    size32_t chunklen;
    
    if (s->reverse == NULL) {
        /* Initialize */
        f = os_zalloc(sizeof(struct fs_file));
    
        /* Copy Name */
        os_strcpy(f->node.name, s->request.path + 3);
        WDTCHECK(s);
        err = fs_new(f);
        if (err == FS_ERR_FILE_EXISTS) {
            return HTTPD_RESPONSE_CONFLICT(s);
        }
        if (err) {
            return err;
        }
        s->reverse = f;
    }
    else {
        f = (struct fs_file*)s->reverse;
    }
    
    CHK("Write");
    err = fs_rbwrite(f, &s->req_rb);
    if (err) {
        return err;
    }
        
    if (more) {
        CHK("Unhold");
        if(!HTTPD_SCHEDULE(HTTPD_SIG_RECVUNHOLD, s)) {
            return HTTPD_ERR_TASKQ_FULL;
        }
    }
    else {
        err = fs_close(f);
        if (err) {
            return err;
        }
        os_free(f);
        WDTCHECK(s);
        return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, "Done."CR, 7);
    }
    return HTTPD_MORE;
   
}
