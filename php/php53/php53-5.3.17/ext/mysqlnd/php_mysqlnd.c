/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2012 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Georg Richter <georg@mysql.com>                             |
  |          Andrey Hristov <andrey@mysql.com>                           |
  |          Ulf Wendel <uwendel@mysql.com>                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */
#include "php.h"
#include "php_ini.h"
#include "mysqlnd.h"
#include "mysqlnd_priv.h"
#include "mysqlnd_debug.h"
#include "ext/standard/info.h"

/* {{{ mysqlnd_functions[]
 *
 * Every user visible function must have an entry in mysqlnd_functions[].
 */
static zend_function_entry mysqlnd_functions[] = {
	PHP_FE_END
};
/* }}} */


/* {{{ mysqlnd_minfo_print_hash */
#if MYSQLND_UNICODE
PHPAPI void mysqlnd_minfo_print_hash(zval *values)
{
	zval **values_entry;
	HashPosition pos_values;

	zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(values), &pos_values);
	while (zend_hash_get_current_data_ex(Z_ARRVAL_P(values),
		(void **)&values_entry, &pos_values) == SUCCESS) {
		zstr	string_key;
		uint	string_key_len;
		ulong	num_key;
		int		s_len;
		char	*s = NULL;

		TSRMLS_FETCH();
		zend_hash_get_current_key_ex(Z_ARRVAL_P(values), &string_key, &string_key_len, &num_key, 0, &pos_values);

		convert_to_string(*values_entry);

		if (zend_unicode_to_string(ZEND_U_CONVERTER(UG(runtime_encoding_conv)),
								   &s, &s_len, string_key.u, string_key_len TSRMLS_CC) == SUCCESS) {
			php_info_print_table_row(2, s, Z_STRVAL_PP(values_entry));
		}
		if (s) {
			mnd_efree(s);
		}

		zend_hash_move_forward_ex(Z_ARRVAL_P(values), &pos_values);
	}
}
#else
PHPAPI void mysqlnd_minfo_print_hash(zval *values)
{
	zval **values_entry;
	HashPosition pos_values;

	zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(values), &pos_values);
	while (zend_hash_get_current_data_ex(Z_ARRVAL_P(values), (void **)&values_entry, &pos_values) == SUCCESS) {
		char	*string_key;
		uint	string_key_len;
		ulong	num_key;

		zend_hash_get_current_key_ex(Z_ARRVAL_P(values), &string_key, &string_key_len, &num_key, 0, &pos_values);

		convert_to_string(*values_entry);
		php_info_print_table_row(2, string_key, Z_STRVAL_PP(values_entry));

		zend_hash_move_forward_ex(Z_ARRVAL_P(values), &pos_values);
	}
}
#endif
/* }}} */


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(mysqlnd)
{
	char buf[32];
	zval values;

	php_info_print_table_start();
	php_info_print_table_header(2, "mysqlnd", "enabled");
	php_info_print_table_row(2, "Version", mysqlnd_get_client_info());
	php_info_print_table_row(2, "Compression",
#ifdef MYSQLND_COMPRESSION_ENABLED
								"supported");
#else
								"not supported");
#endif
	php_info_print_table_row(2, "SSL",
#ifdef MYSQLND_SSL_SUPPORTED
								"supported");
#else
								"not supported");
#endif
	snprintf(buf, sizeof(buf), "%ld", MYSQLND_G(net_cmd_buffer_size));
	php_info_print_table_row(2, "Command buffer size", buf);
	snprintf(buf, sizeof(buf), "%ld", MYSQLND_G(net_read_buffer_size));
	php_info_print_table_row(2, "Read buffer size", buf);
	snprintf(buf, sizeof(buf), "%ld", MYSQLND_G(net_read_timeout));
	php_info_print_table_row(2, "Read timeout", buf);
	php_info_print_table_row(2, "Collecting statistics", MYSQLND_G(collect_statistics)? "Yes":"No");
	php_info_print_table_row(2, "Collecting memory statistics", MYSQLND_G(collect_memory_statistics)? "Yes":"No");

	php_info_print_table_row(2, "Tracing", MYSQLND_G(debug)? MYSQLND_G(debug):"n/a");
	php_info_print_table_end();

	/* Print client stats */
	php_info_print_table_start();
	php_info_print_table_header(2, "Client statistics", "");
	mysqlnd_get_client_stats(&values);
	mysqlnd_minfo_print_hash(&values);

	zval_dtor(&values);
	php_info_print_table_end();
}
/* }}} */


PHPAPI ZEND_DECLARE_MODULE_GLOBALS(mysqlnd)


/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(mysqlnd)
{
	mysqlnd_globals->collect_statistics = TRUE;
	mysqlnd_globals->collect_memory_statistics = FALSE;
	mysqlnd_globals->debug = NULL;	/* The actual string */
	mysqlnd_globals->dbg = NULL;	/* The DBG object*/
	mysqlnd_globals->net_cmd_buffer_size = MYSQLND_NET_CMD_BUFFER_MIN_SIZE;
	mysqlnd_globals->net_read_buffer_size = 32768;
	mysqlnd_globals->net_read_timeout = 31536000;
	mysqlnd_globals->log_mask = 0;
	mysqlnd_globals->mempool_default_size = 16000;
	mysqlnd_globals->debug_emalloc_fail_threshold = -1;
	mysqlnd_globals->debug_ecalloc_fail_threshold = -1;
	mysqlnd_globals->debug_erealloc_fail_threshold = -1;
	mysqlnd_globals->debug_malloc_fail_threshold = -1;
	mysqlnd_globals->debug_calloc_fail_threshold = -1;
	mysqlnd_globals->debug_realloc_fail_threshold = -1;
}
/* }}} */


static PHP_INI_MH(OnUpdateNetCmdBufferSize)
{
	long long_value = atol(new_value);
	if (long_value < MYSQLND_NET_CMD_BUFFER_MIN_SIZE) {
		return FAILURE;
	}
	MYSQLND_G(net_cmd_buffer_size) = long_value;

	return SUCCESS;
}

/* {{{ PHP_INI_BEGIN
*/
PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN("mysqlnd.collect_statistics",	"1", 	PHP_INI_ALL, OnUpdateBool,	collect_statistics, zend_mysqlnd_globals, mysqlnd_globals)
	STD_PHP_INI_BOOLEAN("mysqlnd.collect_memory_statistics",	"0", 	PHP_INI_SYSTEM, OnUpdateBool,	collect_memory_statistics, zend_mysqlnd_globals, mysqlnd_globals)
	STD_PHP_INI_ENTRY("mysqlnd.debug",					NULL, 	PHP_INI_SYSTEM, OnUpdateString,	debug, zend_mysqlnd_globals, mysqlnd_globals)
	STD_PHP_INI_ENTRY("mysqlnd.net_cmd_buffer_size",	MYSQLND_NET_CMD_BUFFER_MIN_SIZE_STR,	PHP_INI_ALL,	OnUpdateNetCmdBufferSize,	net_cmd_buffer_size,	zend_mysqlnd_globals,		mysqlnd_globals)
	STD_PHP_INI_ENTRY("mysqlnd.net_read_buffer_size",	"32768",PHP_INI_ALL,	OnUpdateLong,	net_read_buffer_size,	zend_mysqlnd_globals,		mysqlnd_globals)
	STD_PHP_INI_ENTRY("mysqlnd.net_read_timeout",	"31536000",	PHP_INI_SYSTEM, OnUpdateLong,	net_read_timeout, zend_mysqlnd_globals, mysqlnd_globals)
	STD_PHP_INI_ENTRY("mysqlnd.log_mask",				"0", 	PHP_INI_ALL,	OnUpdateLong,	log_mask, zend_mysqlnd_globals, mysqlnd_globals)
	STD_PHP_INI_ENTRY("mysqlnd.mempool_default_size","16000",   PHP_INI_ALL,	OnUpdateLong,	mempool_default_size,	zend_mysqlnd_globals,		mysqlnd_globals)

#ifdef PHP_DEBUG
	STD_PHP_INI_ENTRY("mysqlnd.debug_emalloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_emalloc_fail_threshold,	zend_mysqlnd_globals,		mysqlnd_globals)
	STD_PHP_INI_ENTRY("mysqlnd.debug_ecalloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_ecalloc_fail_threshold,	zend_mysqlnd_globals,		mysqlnd_globals)
	STD_PHP_INI_ENTRY("mysqlnd.debug_erealloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_erealloc_fail_threshold,	zend_mysqlnd_globals,		mysqlnd_globals)

	STD_PHP_INI_ENTRY("mysqlnd.debug_malloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_malloc_fail_threshold,	zend_mysqlnd_globals,		mysqlnd_globals)
	STD_PHP_INI_ENTRY("mysqlnd.debug_calloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_calloc_fail_threshold,	zend_mysqlnd_globals,		mysqlnd_globals)
	STD_PHP_INI_ENTRY("mysqlnd.debug_realloc_fail_threshold","-1",   PHP_INI_SYSTEM,	OnUpdateLong,	debug_realloc_fail_threshold,	zend_mysqlnd_globals,		mysqlnd_globals)
#endif
PHP_INI_END()
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(mysqlnd)
{
	REGISTER_INI_ENTRIES();

	mysqlnd_library_init(TSRMLS_C);
	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
static PHP_MSHUTDOWN_FUNCTION(mysqlnd)
{
	mysqlnd_library_end(TSRMLS_C);

	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */


#if defined(PHP_DEBUG)
/* {{{ PHP_RINIT_FUNCTION
 */
static PHP_RINIT_FUNCTION(mysqlnd)
{
#if defined(PHP_DEBUG)
	if (MYSQLND_G(debug)) {
		MYSQLND_DEBUG *dbg = mysqlnd_debug_init(mysqlnd_debug_std_no_trace_funcs TSRMLS_CC);
		if (!dbg) {
			return FAILURE;
		}
		dbg->m->set_mode(dbg, MYSQLND_G(debug));
		MYSQLND_G(dbg) = dbg;
	}
#endif
	return SUCCESS;
}
/* }}} */
#endif


#if defined(PHP_DEBUG)
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(mysqlnd)
{
	MYSQLND_DEBUG *dbg = MYSQLND_G(dbg);
	DBG_ENTER("RSHUTDOWN");
	if (dbg) {
		dbg->m->close(dbg);
		dbg->m->free_handle(dbg);
		MYSQLND_G(dbg) = NULL;
	}
	return SUCCESS;
}
/* }}} */
#endif



static const zend_module_dep mysqlnd_deps[] = {
	ZEND_MOD_REQUIRED("standard")
	ZEND_MOD_END
};

/* {{{ mysqlnd_module_entry
 */
zend_module_entry mysqlnd_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	mysqlnd_deps,
	"mysqlnd",
	mysqlnd_functions,
	PHP_MINIT(mysqlnd),
	PHP_MSHUTDOWN(mysqlnd),
#if defined(PHP_DEBUG)
	PHP_RINIT(mysqlnd),
#else
	NULL,
#endif
#ifdef PHP_DEBUG
	PHP_RSHUTDOWN(mysqlnd),
#else
	NULL,
#endif
	PHP_MINFO(mysqlnd),
	MYSQLND_VERSION,
	PHP_MODULE_GLOBALS(mysqlnd),
	PHP_GINIT(mysqlnd),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

/* {{{ COMPILE_DL_MYSQLND */
#ifdef COMPILE_DL_MYSQLND
ZEND_GET_MODULE(mysqlnd)
#endif
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
