/*
  +----------------------------------------------------------------------+
  | APC                                                                  |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2011 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Brian Shire <shire@php.net>                                 |
  +----------------------------------------------------------------------+

 */

/* $Id: apc_iterator.c 327134 2012-08-15 14:21:28Z ab $ */

#include "php_apc.h"
#include "apc_iterator.h"
#include "apc_cache.h"
#include "apc_zend.h"

#include "ext/standard/md5.h"

#include "zend_interfaces.h"

zend_class_entry *apc_iterator_ce;
zend_object_handlers apc_iterator_object_handlers;

/* {{{ apc_iterator_item */
static apc_iterator_item_t* apc_iterator_item_ctor(apc_iterator_t *iterator, slot_t **slot_pp TSRMLS_DC) {
    zval *zvalue;
    char md5str[33];
    slot_t *slot = *slot_pp;
    apc_context_t ctxt = {0, };
    apc_iterator_item_t *item = ecalloc(1, sizeof(apc_iterator_item_t));

    if (slot->key.type == APC_CACHE_KEY_FILE) {
        /* keys should be unique and with stat=1 we could have multiple files with the same name, so use '<device> <inode>' instead */
#ifdef PHP_WIN32
        item->key_len = spprintf(&item->key, 0, "%I64d %I64d", slot->key.data.file.device, slot->key.data.file.inode);
#else
        item->key_len = spprintf(&item->key, 0, "%ld %ld", (ulong)slot->key.data.file.device, (ulong)slot->key.data.file.inode);
#endif
        item->filename_key = estrdup(slot->value->data.file.filename);
    } else if (slot->key.type == APC_CACHE_KEY_USER) {
        item->key = estrndup((char*)slot->key.data.user.identifier, slot->key.data.user.identifier_len);
        item->key_len = slot->key.data.user.identifier_len;
        item->filename_key = item->key; 
    } else if (slot->key.type == APC_CACHE_KEY_FPFILE) {
        item->key = estrndup((char*)slot->key.data.fpfile.fullpath, slot->key.data.fpfile.fullpath_len);
        item->key_len = slot->key.data.fpfile.fullpath_len;
        item->filename_key = item->key;
    } else {
        apc_error("Internal error, invalid entry type." TSRMLS_CC);
    }

    ALLOC_INIT_ZVAL(item->value);
    array_init(item->value);

    if (APC_ITER_TYPE & iterator->format) {
        if(slot->value->type == APC_CACHE_ENTRY_FILE) {
            add_assoc_string(item->value, "type", "file", 1);
        } else if(slot->value->type == APC_CACHE_ENTRY_USER) {
            add_assoc_string(item->value, "type", "user", 1);
        }
    }
    if (APC_ITER_FILENAME & iterator->format) {
        if(slot->value->type == APC_CACHE_ENTRY_FILE) {
            if (slot->key.type == APC_CACHE_KEY_FILE) {
              add_assoc_string(item->value, "filename", slot->value->data.file.filename, 1);
            } else {  /* APC_CACHE_FPFILE */
              add_assoc_string(item->value, "filename", (char*)slot->key.data.fpfile.fullpath, 1);
            }
        }
    }
    if (APC_ITER_DEVICE & iterator->format) {
        if(slot->key.type == APC_CACHE_KEY_FILE) {
#ifdef PHP_WIN32
			char buf[20];
			sprintf(buf, "%I64d", slot->key.data.file.device);
			add_assoc_string(item->value, "device", buf, 1);
#else
            add_assoc_long(item->value, "device", slot->key.data.file.device);
#endif
        }
    }
    if (APC_ITER_INODE & iterator->format) {
        if(slot->key.type == APC_CACHE_KEY_FILE) {
#ifdef PHP_WIN32
			char buf[20];
			sprintf(buf, "%I64d", slot->key.data.file.device);
			add_assoc_string(item->value, "device", buf, 1);
#else
            add_assoc_long(item->value, "inode", slot->key.data.file.inode);
#endif
        }
    }
    if (APC_ITER_KEY & iterator->format) {
        add_assoc_stringl(item->value, "key", item->key, (item->key_len - 1), 1);
    }
    if (APC_ITER_VALUE & iterator->format) {
        if(slot->value->type == APC_CACHE_ENTRY_USER) {

            ctxt.pool = apc_pool_create(APC_UNPOOL, apc_php_malloc, apc_php_free, NULL, NULL TSRMLS_CC);
            ctxt.copy = APC_COPY_OUT_USER;

            MAKE_STD_ZVAL(zvalue);
            apc_cache_fetch_zval(zvalue, slot->value->data.user.val, &ctxt TSRMLS_CC);
            apc_pool_destroy(ctxt.pool TSRMLS_CC);
            add_assoc_zval(item->value, "value", zvalue);
        }
    }
    if (APC_ITER_MD5 & iterator->format) {
        if(slot->value->type == APC_CACHE_ENTRY_FILE) {
            if(APCG(file_md5) && slot->key.md5) {
                make_digest(md5str, slot->key.md5);
                add_assoc_string(item->value, "md5", md5str, 1);
            }
        }
    }
    if (APC_ITER_NUM_HITS & iterator->format) {
        add_assoc_long(item->value, "num_hits", slot->num_hits);
    }
    if (APC_ITER_MTIME & iterator->format) {
        add_assoc_long(item->value, "mtime", slot->key.mtime);
    }
    if (APC_ITER_CTIME & iterator->format) {
        add_assoc_long(item->value, "creation_time", slot->creation_time);
    }
    if (APC_ITER_DTIME & iterator->format) {
        add_assoc_long(item->value, "deletion_time", slot->deletion_time);
    }
    if (APC_ITER_ATIME & iterator->format) {
        add_assoc_long(item->value, "access_time", slot->access_time);
    }
    if (APC_ITER_REFCOUNT & iterator->format) {
        add_assoc_long(item->value, "ref_count", slot->value->ref_count);
    }
    if (APC_ITER_MEM_SIZE & iterator->format) {
        add_assoc_long(item->value, "mem_size", slot->value->mem_size);
    }
    if (APC_ITER_TTL & iterator->format) {
        if(slot->value->type == APC_CACHE_ENTRY_USER) {
            add_assoc_long(item->value, "ttl", slot->value->data.user.ttl);
        }
    }

    return item;
}
/* }}} */

/* {{{ apc_iterator_clone */
static zend_object_value apc_iterator_clone(zval *zobject TSRMLS_DC) {
    zend_object_value value = {0};
    apc_error("APCIterator object cannot be cloned." TSRMLS_CC);
    return value;
}
/* }}} */

/* {{{ apc_iterator_item_dtor */
static void apc_iterator_item_dtor(apc_iterator_item_t *item) {
    if (item->filename_key && item->filename_key != item->key) {
        efree(item->filename_key);
    }
    if (item->key) {
        efree(item->key);
    }
    if (item->value) {
        zval_ptr_dtor(&item->value);
    }
    efree(item);
}
/* }}} */

/* {{{ apc_iterator_destroy */
static void apc_iterator_destroy(void *object, zend_object_handle handle TSRMLS_DC) {
    apc_iterator_t *iterator = (apc_iterator_t*)object;

    if (iterator->initialized == 0) {
        return;
    }

    while (apc_stack_size(iterator->stack) > 0) {
        apc_iterator_item_dtor(apc_stack_pop(iterator->stack));
    }
    apc_stack_destroy(iterator->stack TSRMLS_CC);

#ifdef ITERATOR_PCRE
    if (iterator->regex) {
        efree(iterator->regex);
    }
#endif
    if (iterator->search_hash) {
        zend_hash_destroy(iterator->search_hash);
        efree(iterator->search_hash);
    }
    iterator->initialized = 0;

}
/* }}} */

/* {{{ acp_iterator_free */
static void apc_iterator_free(void *object TSRMLS_DC) {
    zend_object_std_dtor(object TSRMLS_CC);
    efree(object);
}
/* }}} */

/* {{{ apc_iterator_create */
static zend_object_value apc_iterator_create(zend_class_entry *ce TSRMLS_DC) {
    zend_object_value retval;
    apc_iterator_t *iterator;

    iterator = emalloc(sizeof(apc_iterator_t));
    iterator->obj.ce = ce;
    ALLOC_HASHTABLE(iterator->obj.properties);
    zend_hash_init(iterator->obj.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
#ifdef ZEND_ENGINE_2_4
    iterator->obj.properties_table = NULL;
#endif
    iterator->obj.guards = NULL;
    iterator->initialized = 0;
    retval.handle = zend_objects_store_put(iterator, apc_iterator_destroy, apc_iterator_free, NULL TSRMLS_CC);
    retval.handlers = &apc_iterator_object_handlers;

    return retval;
}
/* }}} */

/* {{{ apc_iterator_search_match
 *       Verify if the key matches oru search parameters
 */
static int apc_iterator_search_match(apc_iterator_t *iterator, slot_t **slot) {
    char *key;
    int key_len;
    char *fname_key = NULL;
    int fname_key_len = 0;
    int rval = 1;

    if ((*slot)->key.type == APC_CACHE_KEY_FILE) {
        key = (*slot)->value->data.file.filename;
        key_len = strlen(key);
        fname_key_len = spprintf(&fname_key, 0, "%ld %ld", (*slot)->key.data.file.device, (*slot)->key.data.file.inode);
    } else if ((*slot)->key.type == APC_CACHE_KEY_USER) {
        key = (char*)(*slot)->key.data.user.identifier;
        key_len = (*slot)->key.data.user.identifier_len;
    } else if ((*slot)->key.type == APC_CACHE_KEY_FPFILE) {
        key = (char*)(*slot)->key.data.fpfile.fullpath;
        key_len = (*slot)->key.data.fpfile.fullpath_len;
    } else {
        return 0;
    }

#ifdef ITERATOR_PCRE
    if (iterator->regex) {
        rval = (pcre_exec(iterator->re, NULL, key, strlen(key), 0, 0, NULL, 0) >= 0);
    }
#endif
            
    if (iterator->search_hash) {
        rval = zend_hash_exists(iterator->search_hash, key, key_len);
        if (!rval && fname_key) {
            rval = zend_hash_exists(iterator->search_hash, fname_key, fname_key_len+1);
        }
    }

    if (fname_key) {
        efree(fname_key);
    }

    return rval;
}
/* }}} */

/* {{{ apc_iterator_check_expiry */
static int apc_iterator_check_expiry(apc_cache_t* cache, slot_t **slot, time_t t)
{
    if((*slot)->value->type == APC_CACHE_ENTRY_USER) {
        if((*slot)->value->data.user.ttl) {
            if((time_t) ((*slot)->creation_time + (*slot)->value->data.user.ttl) < t) {
                return 0;
            }
        } else if(cache->ttl) {
            if((*slot)->creation_time + cache->ttl < t) {
                return 0;
            }
        }
    } else if((*slot)->access_time < (t - cache->ttl)) {
        return 0;
    }

    return 1;
}
/* }}} */

/* {{{ apc_iterator_fetch_active */
static int apc_iterator_fetch_active(apc_iterator_t *iterator TSRMLS_DC) {
    int count=0;
    slot_t **slot;
    apc_iterator_item_t *item;
    time_t t;

    t = apc_time();

    while (apc_stack_size(iterator->stack) > 0) {
        apc_iterator_item_dtor(apc_stack_pop(iterator->stack));
    }

    CACHE_LOCK(iterator->cache);
    while(count <= iterator->chunk_size && iterator->slot_idx < iterator->cache->num_slots) {
        slot = &iterator->cache->slots[iterator->slot_idx];
        while(*slot) {
            if (apc_iterator_check_expiry(iterator->cache, slot, t)) {
                if (apc_iterator_search_match(iterator, slot)) {
                    count++;
                    item = apc_iterator_item_ctor(iterator, slot TSRMLS_CC);
                    if (item) {
                        apc_stack_push(iterator->stack, item TSRMLS_CC);
                    }
                }
            }
            slot = &(*slot)->next;
        }
        iterator->slot_idx++;
    }
    CACHE_UNLOCK(iterator->cache);
    iterator->stack_idx = 0;
    return count;
}
/* }}} */

/* {{{ apc_iterator_fetch_deleted */
static int apc_iterator_fetch_deleted(apc_iterator_t *iterator TSRMLS_DC) {
    int count=0;
    slot_t **slot;
    apc_iterator_item_t *item;

    CACHE_LOCK(iterator->cache);
    slot = &iterator->cache->header->deleted_list;
    while ((*slot) && count <= iterator->slot_idx) {
        count++;
        slot = &(*slot)->next;
    }
    count = 0;
    while ((*slot) && count < iterator->chunk_size) {
        if (apc_iterator_search_match(iterator, slot)) {
            count++;
            item = apc_iterator_item_ctor(iterator, slot TSRMLS_CC);
            if (item) {
                apc_stack_push(iterator->stack, item TSRMLS_CC);
            }
        }
        slot = &(*slot)->next;
    }
    CACHE_UNLOCK(iterator->cache);
    iterator->slot_idx += count;
    iterator->stack_idx = 0;
    return count;
}
/* }}} */

/* {{{ apc_iterator_totals */
static void apc_iterator_totals(apc_iterator_t *iterator TSRMLS_DC) {
    slot_t **slot;
    int i;

    CACHE_LOCK(iterator->cache);
    for (i=0; i < iterator->cache->num_slots; i++) {
        slot = &iterator->cache->slots[i];
        while((*slot)) {
            if (apc_iterator_search_match(iterator, slot)) {
                iterator->size += (*slot)->value->mem_size;
                iterator->hits += (*slot)->num_hits;
                iterator->count++;
            }
            slot = &(*slot)->next;
        }
    }
    CACHE_UNLOCK(iterator->cache);
    iterator->totals_flag = 1;
}
/* }}} */

/* {{{ proto object APCIterator::__costruct(string cache [, mixed search [, long format [, long chunk_size [, long list ]]]]) */
PHP_METHOD(apc_iterator, __construct) {
    zval *object = getThis();
    apc_iterator_t *iterator = (apc_iterator_t*)zend_object_store_get_object(object TSRMLS_CC);
    char *cachetype;
    int cachetype_len;
    long format = APC_ITER_ALL;
    long chunk_size=0;
    zval *search = NULL;
    long list = APC_LIST_ACTIVE;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|zlll", &cachetype, &cachetype_len, &search, &format, &chunk_size, &list) == FAILURE) {
        return;
    }

    if (!APCG(enabled)) {
        apc_error("APC must be enabled to use APCIterator." TSRMLS_CC);
    }

    if (chunk_size < 0) {
        apc_error("APCIterator chunk size must be 0 or greater." TSRMLS_CC);
        return;
    }

    if (format > APC_ITER_ALL) {
        apc_error("APCIterator format is invalid." TSRMLS_CC);
        return;
    }

    if (list == APC_LIST_ACTIVE) {
        iterator->fetch = apc_iterator_fetch_active;
    } else if (list == APC_LIST_DELETED) {
        iterator->fetch = apc_iterator_fetch_deleted;
    } else {
        apc_warning("APCIterator invalid list type." TSRMLS_CC);
        return;
    }

    if(!strcasecmp(cachetype,"user")) {
        iterator->cache = apc_user_cache;
    } else {
        iterator->cache = apc_cache;
    }

    iterator->slot_idx = 0;
    iterator->stack_idx = 0;
    iterator->key_idx = 0;
    iterator->chunk_size = chunk_size == 0 ? APC_DEFAULT_CHUNK_SIZE : chunk_size;
    iterator->stack = apc_stack_create(chunk_size TSRMLS_CC);
    iterator->format = format;
    iterator->totals_flag = 0;
    iterator->count = 0;
    iterator->size = 0;
    iterator->hits = 0;
    iterator->regex = NULL;
    iterator->regex_len = 0;
    iterator->search_hash = NULL;
    if (search && Z_TYPE_P(search) == IS_STRING && Z_STRLEN_P(search)) {
#ifdef ITERATOR_PCRE
        iterator->regex = estrndup(Z_STRVAL_P(search), Z_STRLEN_P(search));
        iterator->regex_len = Z_STRLEN_P(search);
        iterator->re = pcre_get_compiled_regex(Z_STRVAL_P(search), NULL, NULL TSRMLS_CC);

        if(!iterator->re) {
            apc_error("Could not compile regular expression: %s" TSRMLS_CC, Z_STRVAL_P(search));
        }
#else
        apc_error("Regular expressions support is not enabled, please enable PCRE for APCIterator regex support" TSRMLS_CC);
#endif
    } else if (search && Z_TYPE_P(search) == IS_ARRAY) {
        Z_ADDREF_P(search);
        iterator->search_hash = apc_flip_hash(Z_ARRVAL_P(search));
    }
    iterator->initialized = 1;
}
/* }}} */

/* {{{ proto APCIterator::rewind() */
PHP_METHOD(apc_iterator, rewind) {
    apc_iterator_t *iterator = (apc_iterator_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    if (iterator->initialized == 0) {
        RETURN_FALSE;
    }

    iterator->slot_idx = 0;
    iterator->stack_idx = 0;
    iterator->key_idx = 0;
    iterator->fetch(iterator TSRMLS_CC);
}
/* }}} */

/* {{{ proto boolean APCIterator::valid() */
PHP_METHOD(apc_iterator, valid) {
    apc_iterator_t *iterator = (apc_iterator_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    if (iterator->initialized == 0) {
        RETURN_FALSE;
    }

    if (apc_stack_size(iterator->stack) == iterator->stack_idx) {
        iterator->fetch(iterator TSRMLS_CC);
    }

    RETURN_BOOL(apc_stack_size(iterator->stack) == 0 ? 0 : 1);
}
/* }}} */

/* {{{ proto mixed APCIterator::current() */
PHP_METHOD(apc_iterator, current) {
    apc_iterator_item_t *item;
    apc_iterator_t *iterator = (apc_iterator_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    if (iterator->initialized == 0) {
        RETURN_FALSE;
    }

    if (apc_stack_size(iterator->stack) == iterator->stack_idx) {
        if (iterator->fetch(iterator TSRMLS_CC) == 0) {
            RETURN_FALSE;
        }
    }

    item = apc_stack_get(iterator->stack, iterator->stack_idx);
    RETURN_ZVAL(item->value, 1, 0);
}
/* }}} */

/* {{{ proto string APCIterator::key() */
PHP_METHOD(apc_iterator, key) {
    apc_iterator_item_t *item;
    apc_iterator_t *iterator = (apc_iterator_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    if (iterator->initialized == 0 || apc_stack_size(iterator->stack) == 0) {
        RETURN_FALSE;
    }

    if (apc_stack_size(iterator->stack) == iterator->stack_idx) {
        if (iterator->fetch(iterator TSRMLS_CC) == 0) {
            RETURN_FALSE;
        }
    }

    item = apc_stack_get(iterator->stack, iterator->stack_idx);

    if (item->key) {
        RETURN_STRINGL(item->key, (item->key_len-1), 1);
    } else {
        RETURN_LONG(iterator->key_idx);
    }
}
/* }}} */

/* {{{ proto APCIterator::next() */
PHP_METHOD(apc_iterator, next) {
    apc_iterator_t *iterator = (apc_iterator_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    if (iterator->initialized == 0 || apc_stack_size(iterator->stack) == 0) {
        RETURN_FALSE;
    }

    iterator->stack_idx++;
    iterator->key_idx++;

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto long APCIterator::getTotalHits() */
PHP_METHOD(apc_iterator, getTotalHits) {
    apc_iterator_t *iterator = (apc_iterator_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    if (iterator->initialized == 0) {
        RETURN_FALSE;
    }

    if (iterator->totals_flag == 0) {
        apc_iterator_totals(iterator TSRMLS_CC);
    }

    RETURN_LONG(iterator->hits);
}
/* }}} */

/* {{{ proto long APCIterator::getTotalSize() */
PHP_METHOD(apc_iterator, getTotalSize) {
    apc_iterator_t *iterator = (apc_iterator_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    if (iterator->initialized == 0) {
        RETURN_FALSE;
    }

    if (iterator->totals_flag == 0) {
        apc_iterator_totals(iterator TSRMLS_CC);
    }

    RETURN_LONG(iterator->size);
}
/* }}} */

/* {{{ proto long APCIterator::getTotalCount() */
PHP_METHOD(apc_iterator, getTotalCount) {
    apc_iterator_t *iterator = (apc_iterator_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    if (iterator->initialized == 0) {
        RETURN_FALSE;
    }

    if (iterator->totals_flag == 0) {
        apc_iterator_totals(iterator TSRMLS_CC);
    }

    RETURN_LONG(iterator->count);
}
/* }}} */

/* {{{ arginfo */
#if (PHP_MAJOR_VERSION >= 6 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3))
# define PHP_APC_ARGINFO
#else
# define PHP_APC_ARGINFO static
#endif

PHP_APC_ARGINFO
ZEND_BEGIN_ARG_INFO_EX(arginfo_apc_iterator___construct, 0, 0, 1)
	ZEND_ARG_INFO(0, cache)
	ZEND_ARG_INFO(0, search)
	ZEND_ARG_INFO(0, format)
	ZEND_ARG_INFO(0, chunk_size)
	ZEND_ARG_INFO(0, list)
ZEND_END_ARG_INFO()

PHP_APC_ARGINFO
ZEND_BEGIN_ARG_INFO_EX(arginfo_apc_iterator_void, 0, 0, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ apc_iterator_functions */
static zend_function_entry apc_iterator_functions[] = {
    PHP_ME(apc_iterator, __construct, arginfo_apc_iterator___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(apc_iterator, rewind, arginfo_apc_iterator_void, ZEND_ACC_PUBLIC)
    PHP_ME(apc_iterator, current, arginfo_apc_iterator_void, ZEND_ACC_PUBLIC)
    PHP_ME(apc_iterator, key, arginfo_apc_iterator_void, ZEND_ACC_PUBLIC)
    PHP_ME(apc_iterator, next, arginfo_apc_iterator_void, ZEND_ACC_PUBLIC)
    PHP_ME(apc_iterator, valid, arginfo_apc_iterator_void, ZEND_ACC_PUBLIC)
    PHP_ME(apc_iterator, getTotalHits, arginfo_apc_iterator_void, ZEND_ACC_PUBLIC)
    PHP_ME(apc_iterator, getTotalSize, arginfo_apc_iterator_void, ZEND_ACC_PUBLIC)
    PHP_ME(apc_iterator, getTotalCount, arginfo_apc_iterator_void, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ apc_iterator_init */
int apc_iterator_init(int module_number TSRMLS_DC) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, APC_ITERATOR_NAME, apc_iterator_functions);
    apc_iterator_ce = zend_register_internal_class(&ce TSRMLS_CC);
    apc_iterator_ce->create_object = apc_iterator_create;
    zend_class_implements(apc_iterator_ce TSRMLS_CC, 1, zend_ce_iterator);

    zend_register_long_constant("APC_LIST_ACTIVE", sizeof("APC_LIST_ACTIVE"), APC_LIST_ACTIVE, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_LIST_DELETED", sizeof("APC_LIST_DELETED"), APC_LIST_DELETED, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);

    zend_register_long_constant("APC_ITER_TYPE", sizeof("APC_ITER_TYPE"), APC_ITER_TYPE, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_KEY", sizeof("APC_ITER_KEY"), APC_ITER_KEY, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_FILENAME", sizeof("APC_ITER_FILENAME"), APC_ITER_FILENAME, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_DEVICE", sizeof("APC_ITER_DEVICE"), APC_ITER_DEVICE, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_INODE", sizeof("APC_ITER_INODE"), APC_ITER_INODE, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_VALUE", sizeof("APC_ITER_VALUE"), APC_ITER_VALUE, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_MD5", sizeof("APC_ITER_MD5"), APC_ITER_MD5, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_NUM_HITS", sizeof("APC_ITER_NUM_HITS"), APC_ITER_NUM_HITS, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_MTIME", sizeof("APC_ITER_MTIME"), APC_ITER_MTIME, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_CTIME", sizeof("APC_ITER_CTIME"), APC_ITER_CTIME, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_DTIME", sizeof("APC_ITER_DTIME"), APC_ITER_DTIME, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_ATIME", sizeof("APC_ITER_ATIME"), APC_ITER_ATIME, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_REFCOUNT", sizeof("APC_ITER_REFCOUNT"), APC_ITER_REFCOUNT, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_MEM_SIZE", sizeof("APC_ITER_MEM_SIZE"), APC_ITER_MEM_SIZE, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_TTL", sizeof("APC_ITER_TTL"), APC_ITER_TTL, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_NONE", sizeof("APC_ITER_NONE"), APC_ITER_NONE, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);
    zend_register_long_constant("APC_ITER_ALL", sizeof("APC_ITER_ALL"), APC_ITER_ALL, CONST_PERSISTENT | CONST_CS, module_number TSRMLS_CC);

    memcpy(&apc_iterator_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    apc_iterator_object_handlers.clone_obj = apc_iterator_clone;

    return SUCCESS;
}
/* }}} */

/* {{{ apc_iterator_delete */
int apc_iterator_delete(zval *zobj TSRMLS_DC) {
    apc_iterator_t *iterator;
    zend_class_entry *ce = Z_OBJCE_P(zobj);
    apc_iterator_item_t *item;

    if (!ce || !instanceof_function(ce, apc_iterator_ce TSRMLS_CC)) {
        apc_error("apc_delete object argument must be instance of APCIterator" TSRMLS_CC);
        return 0;
    }
    iterator = (apc_iterator_t*)zend_object_store_get_object(zobj TSRMLS_CC);

    if (iterator->initialized == 0) {
        return 0;
    }

    while (iterator->fetch(iterator TSRMLS_CC)) {
        while (iterator->stack_idx < apc_stack_size(iterator->stack)) {
            item = apc_stack_get(iterator->stack, iterator->stack_idx++);
            if (iterator->cache == apc_cache) {
                apc_cache_delete(apc_cache, item->filename_key, strlen(item->filename_key) + 1 TSRMLS_CC);
            } else {
                apc_cache_user_delete(apc_user_cache, item->key, item->key_len TSRMLS_CC);
            }
        }
    }

    return 1;
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim>600: expandtab sw=4 ts=4 sts=4 fdm=marker
 * vim<600: expandtab sw=4 ts=4 sts=4
 */
