// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: php_fnx.h 325564 2012-05-07 08:27:31Z nechtan $ */

#ifndef PHP_FNX_H
#define PHP_FNX_H

extern zend_module_entry fnx_module_entry;
#define phpext_fnx_ptr &fnx_module_entry

#ifdef PHP_WIN32
#define PHP_FNX_API __declspec(dllexport)
#ifndef _MSC_VER
#define _MSC_VER 1600
#endif
#else
#define PHP_FNX_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#ifdef ZTS
#define FNX_G(v) TSRMG(fnx_globals_id, zend_fnx_globals *, v)
#else
#define FNX_G(v) (fnx_globals.v)
#endif

#define FNX_VERSION 					"1.8.10"
#define FNX_HELLOWORD 					"PHP is my girlfriend"
#define FNX_ABOUT 					"Oh Captain! my Captain!"
#define FNX_URL 					"http://fnx.com"

#define FNX_STARTUP_FUNCTION(module)   	ZEND_MINIT_FUNCTION(fnx_##module)
#define FNX_RINIT_FUNCTION(modle)		ZEND_RINIT_FUNCTION(fnx_##module)
#define FNX_STARTUP(module)	 		  	ZEND_MODULE_STARTUP_N(fnx_##module)(INIT_FUNC_ARGS_PASSTHRU)
#define FNX_SHUTDOWN_FUNCTION(module)  	ZEND_MINIT_FUNCTION(fnx_##module)
#define FNX_SHUTDOWN(module)	 	    ZEND_MODULE_SHUTDOWN_N(fnx_##module)(INIT_FUNC_ARGS_PASSTHRU)

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
#define FNX_HAVE_NAMESPACE
#else
#define Z_ADDREF_P 	 ZVAL_ADDREF
#define Z_REFCOUNT_P ZVAL_REFCOUNT
#define Z_DELREF_P 	 ZVAL_DELREF
#endif

#define fnx_application_t	zval
#define fnx_view_t 			zval
#define fnx_controller_t	zval
#define fnx_request_t		zval
#define fnx_router_t		zval
#define fnx_route_t			zval
#define fnx_dispatcher_t	zval
#define fnx_action_t		zval
#define fnx_loader_t		zval
#define fnx_response_t		zval
#define fnx_config_t		zval
#define fnx_registry_t		zval
#define fnx_plugin_t		zval
#define fnx_session_t		zval
#define fnx_exception_t		zval

extern PHPAPI int  php_register_info_logo(char *logo_string, const char *mimetype, const unsigned char *data, int size);
extern PHPAPI void php_var_dump(zval **struc, int level TSRMLS_DC);
extern PHPAPI void php_debug_zval_dump(zval **struc, int level TSRMLS_DC);

ZEND_BEGIN_MODULE_GLOBALS(fnx)
	char 		*ext;
	char		*base_uri;
	char 		*environ;
	char 		*directory;
	char 		*local_library;
	char        *local_namespace;
	char 		*global_library;
	char 		*view_ext;
	char 		*default_module;
	char 		*default_controller;
	char 		*default_action;
	char 		*bootstrap;
	char 		*name_separator;
	zend_bool 	lowcase_path;
	zend_bool 	use_spl_autoload;
	zend_bool 	throw_exception;
	zend_bool 	cache_config;
	zend_bool   action_prefer;
	zend_bool	name_suffix;
	zend_bool  	autoload_started;
	zend_bool  	running;
	zend_bool  	in_exception;
	zend_bool  	catch_exception;
/* {{{ This only effects internally */
	zend_bool  	st_compatible;
/* }}} */
	long		forward_limit;
	HashTable	*configs;
	zval 		*modules;
	zval        *default_route;
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	uint 		buf_nesting;
	void		*buffer;
	void 		*owrite_handler;
#endif
	zval        *active_ini_file_section;
	zval        *ini_wanted_section;
	uint        parsing_flag;
#ifdef FNX_HAVE_NAMESPACE
	zend_bool	use_namespace;
#endif
ZEND_END_MODULE_GLOBALS(fnx)

PHP_MINIT_FUNCTION(fnx);
PHP_MSHUTDOWN_FUNCTION(fnx);
PHP_RINIT_FUNCTION(fnx);
PHP_RSHUTDOWN_FUNCTION(fnx);
PHP_MINFO_FUNCTION(fnx);

extern ZEND_DECLARE_MODULE_GLOBALS(fnx);

#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
