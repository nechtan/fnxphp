// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: ini.c 324896 2012-04-06 09:46:37Z nechtan $ */

zend_class_entry *fnx_config_ini_ce;

fnx_config_t * fnx_config_ini_instance(fnx_config_t *this_ptr, zval *filename, zval *section TSRMLS_DC);

#define FNX_CONFIG_INI_PARSING_START   0
#define FNX_CONFIG_INI_PARSING_PROCESS 1
#define FNX_CONFIG_INI_PARSING_END     2

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(fnx_config_ini_construct_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, config_file)
	ZEND_ARG_INFO(0, section)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_config_ini_get_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_config_ini_rget_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_config_ini_unset_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_config_ini_set_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_config_ini_isset_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ static void fnx_config_ini_zval_deep_copy(zval **p) 
 */
static void fnx_config_ini_zval_deep_copy(zval **p) {
	zval *value;
	ALLOC_ZVAL(value);
	*value = **p;

	switch (Z_TYPE_PP(p)) {
		case IS_ARRAY:
			{
				array_init(value);
				zend_hash_copy(Z_ARRVAL_P(value), Z_ARRVAL_PP(p), 
						(copy_ctor_func_t)fnx_config_ini_zval_deep_copy, NULL, sizeof(zval *));
			}
			break;
		default:
			zval_copy_ctor(value);
			Z_TYPE_P(value) = Z_TYPE_PP(p);
	}

	INIT_PZVAL(value);
	*p = value;
}
/* }}} */

/** {{{ zval * fnx_config_ini_format(fnx_config_t *instance, zval **ppzval TSRMLS_DC)
*/
zval * fnx_config_ini_format(fnx_config_t *instance, zval **ppzval TSRMLS_DC) {
	zval *readonly, *ret;
	readonly = zend_read_property(fnx_config_ini_ce, instance, ZEND_STRL(FNX_CONFIG_PROPERT_NAME_READONLY), 1 TSRMLS_CC);
	ret = fnx_config_ini_instance(NULL, *ppzval, NULL TSRMLS_CC);
	return ret;
}
/* }}} */

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2))
/** {{{ static void fnx_config_ini_simple_parser_cb(zval *key, zval *value, zval *index, int callback_type, zval *arr TSRMLS_DC)
*/
static void fnx_config_ini_simple_parser_cb(zval *key, zval *value, zval *index, int callback_type, zval *arr TSRMLS_DC) {
	zval *element;
	switch (callback_type) {
		case ZEND_INI_PARSER_ENTRY:
			{
				char *skey, *seg, *ptr;
				zval **ppzval, *dst;

				if (!value) {
					break;
				}

				dst = arr;
				skey = estrndup(Z_STRVAL_P(key), Z_STRLEN_P(key));
				if ((seg = php_strtok_r(skey, ".", &ptr))) {
					do {
					    char *real_key = seg;
						seg = php_strtok_r(NULL, ".", &ptr);
						if (zend_symtable_find(Z_ARRVAL_P(dst), real_key, strlen(real_key) + 1, (void **) &ppzval) == FAILURE) {
							if (seg) {
								zval *tmp;
							    MAKE_STD_ZVAL(tmp);   
								array_init(tmp);
								zend_symtable_update(Z_ARRVAL_P(dst), 
										real_key, strlen(real_key) + 1, (void **)&tmp, sizeof(zval *), (void **)&ppzval);
							} else {
							    MAKE_STD_ZVAL(element);
								ZVAL_ZVAL(element, value, 1, 0);
								zend_symtable_update(Z_ARRVAL_P(dst), 
										real_key, strlen(real_key) + 1, (void **)&element, sizeof(zval *), NULL);
								break;
							}
						} else {
							if (IS_ARRAY != Z_TYPE_PP(ppzval)) {
								if (seg) {
									zval *tmp;
									MAKE_STD_ZVAL(tmp);   
									array_init(tmp);
									zend_symtable_update(Z_ARRVAL_P(dst), 
											real_key, strlen(real_key) + 1, (void **)&tmp, sizeof(zval *), (void **)&ppzval);
								} else {
									MAKE_STD_ZVAL(element);
									ZVAL_ZVAL(element, value, 1, 0);
									zend_symtable_update(Z_ARRVAL_P(dst), 
											real_key, strlen(real_key) + 1, (void **)&element, sizeof(zval *), NULL);
								}
							} 
						}
						dst = *ppzval;
					} while (seg);
				}
				efree(skey);
			}
			break;

		case ZEND_INI_PARSER_POP_ENTRY:
			{
				zval *hash, **find_hash, *dst;

				if (!value) {
					break;
				}

				if (!(Z_STRLEN_P(key) > 1 && Z_STRVAL_P(key)[0] == '0')
						&& is_numeric_string(Z_STRVAL_P(key), Z_STRLEN_P(key), NULL, NULL, 0) == IS_LONG) {
					ulong skey = (ulong)zend_atol(Z_STRVAL_P(key), Z_STRLEN_P(key));
					if (zend_hash_index_find(Z_ARRVAL_P(arr), skey, (void **) &find_hash) == FAILURE) {
						MAKE_STD_ZVAL(hash);
						array_init(hash);
						zend_hash_index_update(Z_ARRVAL_P(arr), skey, &hash, sizeof(zval *), NULL);
					} else {
						hash = *find_hash;
					}
				} else {
					char *seg, *ptr;
					char *skey = estrndup(Z_STRVAL_P(key), Z_STRLEN_P(key));

					dst = arr;
					if ((seg = php_strtok_r(skey, ".", &ptr))) {
						while (seg) {
							if (zend_symtable_find(Z_ARRVAL_P(dst), seg, strlen(seg) + 1, (void **) &find_hash) == FAILURE) {
								MAKE_STD_ZVAL(hash);
								array_init(hash);
								zend_symtable_update(Z_ARRVAL_P(dst), 
										seg, strlen(seg) + 1, (void **)&hash, sizeof(zval *), (void **)&find_hash);
							}
							dst = *find_hash;
							seg = php_strtok_r(NULL, ".", &ptr);
						}
						hash = dst;
					} else {
						if (zend_symtable_find(Z_ARRVAL_P(dst), seg, strlen(seg) + 1, (void **)&find_hash) == FAILURE) {
							MAKE_STD_ZVAL(hash);
							array_init(hash);
							zend_symtable_update(Z_ARRVAL_P(dst), seg, strlen(seg) + 1, (void **)&hash, sizeof(zval *), NULL);
						} else {
							hash = *find_hash;
						}
					}
					efree(skey);
				}

				if (Z_TYPE_P(hash) != IS_ARRAY) {
					zval_dtor(hash);
					INIT_PZVAL(hash);
					array_init(hash);
				}

				MAKE_STD_ZVAL(element);
				ZVAL_ZVAL(element, value, 1, 0);

				if (index && Z_STRLEN_P(index) > 0) {
					add_assoc_zval_ex(hash, Z_STRVAL_P(index), Z_STRLEN_P(index) + 1, element);
				} else {
					add_next_index_zval(hash, element);
				}
			}
			break;

		case ZEND_INI_PARSER_SECTION:
			break;
	}
}
/* }}} */

/** {{{ static void fnx_config_ini_parser_cb(zval *key, zval *value, zval *index, int callback_type, zval *arr TSRMLS_DC)
*/
static void fnx_config_ini_parser_cb(zval *key, zval *value, zval *index, int callback_type, zval *arr TSRMLS_DC) {

	if (FNX_G(parsing_flag) == FNX_CONFIG_INI_PARSING_END) {
		return;
	}

	if (callback_type == ZEND_INI_PARSER_SECTION) {
		zval **parent;
		char *seg, *skey;
		uint skey_len;

		if (FNX_G(parsing_flag) == FNX_CONFIG_INI_PARSING_PROCESS) {
			FNX_G(parsing_flag) = FNX_CONFIG_INI_PARSING_END;
			return;
		}

		skey = estrndup(Z_STRVAL_P(key), Z_STRLEN_P(key));

		MAKE_STD_ZVAL(FNX_G(active_ini_file_section));
		array_init(FNX_G(active_ini_file_section));

		if ((seg = strchr(skey, ':'))) {
			char *section;

			while (*(seg) == ' ' || *(seg) == ':') {
				*(seg++) = '\0';
			}

			if ((section = strrchr(seg, ':'))) {
			    /* muilt-inherit */
				do {
					while (*(section) == ' ' || *(section) == ':') {
						*(section++) = '\0';
					}
					if (zend_symtable_find(Z_ARRVAL_P(arr), section, strlen(section) + 1, (void **)&parent) == SUCCESS) {
						zend_hash_copy(Z_ARRVAL_P(FNX_G(active_ini_file_section)), Z_ARRVAL_PP(parent),
							   	(copy_ctor_func_t)fnx_config_ini_zval_deep_copy, NULL, sizeof(zval *));
					}
				} while ((section = strrchr(seg, ':')));
			}

			/* remove the tail space, thinking of 'foo : bar : test' */
            section = seg + strlen(seg) - 1;
			while (*section == ' ' || *section == ':') {
				*(section--) = '\0';
			}

			if (zend_symtable_find(Z_ARRVAL_P(arr), seg, strlen(seg) + 1, (void **)&parent) == SUCCESS) {
				zend_hash_copy(Z_ARRVAL_P(FNX_G(active_ini_file_section)), Z_ARRVAL_PP(parent),
						(copy_ctor_func_t)fnx_config_ini_zval_deep_copy, NULL, sizeof(zval *));
			}
		}
	    seg = skey + strlen(skey) - 1;
        while (*seg == ' ' || *seg == ':') {
			*(seg--) = '\0';
		}
		skey_len = strlen(skey);
		zend_symtable_update(Z_ARRVAL_P(arr), skey, skey_len + 1, &FNX_G(active_ini_file_section), sizeof(zval *), NULL);
		if (FNX_G(ini_wanted_section) && Z_STRLEN_P(FNX_G(ini_wanted_section)) == skey_len
				&& !strncasecmp(Z_STRVAL_P(FNX_G(ini_wanted_section)), skey, skey_len)) {
			FNX_G(parsing_flag) = FNX_CONFIG_INI_PARSING_PROCESS;
		}
		efree(skey);
	} else if (value) {
		zval *active_arr;
		if (FNX_G(active_ini_file_section)) {
			active_arr = FNX_G(active_ini_file_section);
		} else {
			active_arr = arr;
		}
		fnx_config_ini_simple_parser_cb(key, value, index, callback_type, active_arr TSRMLS_CC);
	}
}
/* }}} */
#else 
/** {{{ static void fnx_config_ini_simple_parser_cb(zval *key, zval *value, int callback_type, zval *arr)
*/
static void fnx_config_ini_simple_parser_cb(zval *key, zval *value, int callback_type, zval *arr) {
	zval *element;
	switch (callback_type) {
		case ZEND_INI_PARSER_ENTRY:
			{
				char *skey, *seg, *ptr;
				zval **ppzval, *dst;

				if (!value) {
					break;
				}

				dst = arr;
				skey = estrndup(Z_STRVAL_P(key), Z_STRLEN_P(key));
				if ((seg = php_strtok_r(skey, ".", &ptr))) {
					do {
					    char *real_key = seg;
						seg = php_strtok_r(NULL, ".", &ptr);
						if (zend_symtable_find(Z_ARRVAL_P(dst), real_key, strlen(real_key) + 1, (void **) &ppzval) == FAILURE) {
							if (seg) {
								zval *tmp;
							    MAKE_STD_ZVAL(tmp);   
								array_init(tmp);
								zend_symtable_update(Z_ARRVAL_P(dst), 
										real_key, strlen(real_key) + 1, (void **)&tmp, sizeof(zval *), (void **)&ppzval);
							} else {
							    MAKE_STD_ZVAL(element);
								ZVAL_ZVAL(element, value, 1, 0);
								zend_symtable_update(Z_ARRVAL_P(dst), 
										real_key, strlen(real_key) + 1, (void **)&element, sizeof(zval *), NULL);
								break;
							}
						} else {
							if (IS_ARRAY != Z_TYPE_PP(ppzval)) {
								if (seg) {
									zval *tmp;
									MAKE_STD_ZVAL(tmp);   
									array_init(tmp);
									zend_symtable_update(Z_ARRVAL_P(dst), 
											real_key, strlen(real_key) + 1, (void **)&tmp, sizeof(zval *), (void **)&ppzval);
								} else {
									MAKE_STD_ZVAL(element);
									ZVAL_ZVAL(element, value, 1, 0);
									zend_symtable_update(Z_ARRVAL_P(dst), 
											real_key, strlen(real_key) + 1, (void **)&element, sizeof(zval *), NULL);
								}
							} 
						}
						dst = *ppzval;
					} while (seg);
				}
				efree(skey);
			}
			break;

		case ZEND_INI_PARSER_POP_ENTRY:
			{
				zval *hash, **find_hash, *dst;

				if (!value) {
					break;
				}

				if (!(Z_STRLEN_P(key) > 1 && Z_STRVAL_P(key)[0] == '0')
						&& is_numeric_string(Z_STRVAL_P(key), Z_STRLEN_P(key), NULL, NULL, 0) == IS_LONG) {
					ulong skey = (ulong)zend_atol(Z_STRVAL_P(key), Z_STRLEN_P(key));
					if (zend_hash_index_find(Z_ARRVAL_P(arr), skey, (void **) &find_hash) == FAILURE) {
						MAKE_STD_ZVAL(hash);
						array_init(hash);
						zend_hash_index_update(Z_ARRVAL_P(arr), skey, &hash, sizeof(zval *), NULL);
					} else {
						hash = *find_hash;
					}
				} else {
					char *seg, *ptr;
					char *skey = estrndup(Z_STRVAL_P(key), Z_STRLEN_P(key));

					dst = arr;
					if ((seg = php_strtok_r(skey, ".", &ptr))) {
						while (seg) {
							if (zend_symtable_find(Z_ARRVAL_P(dst), seg, strlen(seg) + 1, (void **) &find_hash) == FAILURE) {
								MAKE_STD_ZVAL(hash);
								array_init(hash);
								zend_symtable_update(Z_ARRVAL_P(dst), 
										seg, strlen(seg) + 1, (void **)&hash, sizeof(zval *), (void **)&find_hash);
							}
							dst = *find_hash;
							seg = php_strtok_r(NULL, ".", &ptr);
						}
						hash = dst;
					} else {
						if (zend_symtable_find(Z_ARRVAL_P(dst), seg, strlen(seg) + 1, (void **)&find_hash) == FAILURE) {
							MAKE_STD_ZVAL(hash);
							array_init(hash);
							zend_symtable_update(Z_ARRVAL_P(dst), seg, strlen(seg) + 1, (void **)&hash, sizeof(zval *), NULL);
						} else {
							hash = *find_hash;
						}
					}
					efree(skey);
				}

				if (Z_TYPE_P(hash) != IS_ARRAY) {
					zval_dtor(hash);
					INIT_PZVAL(hash);
					array_init(hash);
				}

				MAKE_STD_ZVAL(element);
				ZVAL_ZVAL(element, value, 1, 0);
				add_next_index_zval(hash, element);
			}
			break;

		case ZEND_INI_PARSER_SECTION:
			break;
	}
}
/* }}} */

/** {{{ static void fnx_config_ini_parser_cb(zval *key, zval *value, int callback_type, zval *arr)
*/
static void fnx_config_ini_parser_cb(zval *key, zval *value, int callback_type, zval *arr) {
	TSRMLS_FETCH();

	if (FNX_G(parsing_flag) == FNX_CONFIG_INI_PARSING_END) {
		return;
	}

	if (callback_type == ZEND_INI_PARSER_SECTION) {
		zval **parent;
		char *seg, *skey;
		uint skey_len;

		if (FNX_G(parsing_flag) == FNX_CONFIG_INI_PARSING_PROCESS) {
			FNX_G(parsing_flag) = FNX_CONFIG_INI_PARSING_END;
			return;
		}

		skey = estrndup(Z_STRVAL_P(key), Z_STRLEN_P(key));

		MAKE_STD_ZVAL(FNX_G(active_ini_file_section));
		array_init(FNX_G(active_ini_file_section));

		if ((seg = strchr(skey, ':'))) {
			char *section;

			while (*(seg) == ' ' || *(seg) == ':') {
				*(seg++) = '\0';
			}

			if ((section = strrchr(seg, ':'))) {
			    /* muilt-inherit */
				do {
					while (*(section) == ' ' || *(section) == ':') {
						*(section++) = '\0';
					}
					if (zend_symtable_find(Z_ARRVAL_P(arr), section, strlen(section) + 1, (void **)&parent) == SUCCESS) {
						zend_hash_copy(Z_ARRVAL_P(FNX_G(active_ini_file_section)), Z_ARRVAL_PP(parent),
							   	(copy_ctor_func_t)fnx_config_ini_zval_deep_copy, NULL, sizeof(zval *));
					}
				} while ((section = strrchr(seg, ':')));
			}

			/* remove the tail space, thinking of 'foo : bar : test' */
            section = seg + strlen(seg) - 1;
			while (*section == ' ' || *section == ':') {
				*(section--) = '\0';
			}

			if (zend_symtable_find(Z_ARRVAL_P(arr), seg, strlen(seg) + 1, (void **)&parent) == SUCCESS) {
				zend_hash_copy(Z_ARRVAL_P(FNX_G(active_ini_file_section)), Z_ARRVAL_PP(parent),
						(copy_ctor_func_t)fnx_config_ini_zval_deep_copy, NULL, sizeof(zval *));
			}
		}
	    seg = skey + strlen(skey) - 1;
        while (*seg == ' ' || *seg == ':') {
			*(seg--) = '\0';
		}	
		skey_len = strlen(skey);
		zend_symtable_update(Z_ARRVAL_P(arr), skey, skey_len + 1, &FNX_G(active_ini_file_section), sizeof(zval *), NULL);
		if (FNX_G(ini_wanted_section) && Z_STRLEN_P(FNX_G(ini_wanted_section)) == skey_len
				&& !strncasecmp(Z_STRVAL_P(FNX_G(ini_wanted_section)), skey, Z_STRLEN_P(FNX_G(ini_wanted_section)))) {
			FNX_G(parsing_flag) = FNX_CONFIG_INI_PARSING_PROCESS;
		}
		efree(skey);
	} else if (value) {
		zval *active_arr;
		if (FNX_G(active_ini_file_section)) {
			active_arr = FNX_G(active_ini_file_section);
		} else {
			active_arr = arr;
		}
		fnx_config_ini_simple_parser_cb(key, value, callback_type, active_arr);
	}
}
/* }}} */
#endif

/** {{{ fnx_config_t * fnx_config_ini_instance(fnx_config_t *this_ptr, zval *filename, zval *section_name TSRMLS_DC)
*/
fnx_config_t * fnx_config_ini_instance(fnx_config_t *this_ptr, zval *filename, zval *section_name TSRMLS_DC) {
	fnx_config_t *instance;
	zval *configs = NULL;

	if (filename && Z_TYPE_P(filename) == IS_ARRAY) {
		if (this_ptr) {
			instance = this_ptr;
		} else {
			MAKE_STD_ZVAL(instance);
			object_init_ex(instance, fnx_config_ini_ce);
		}

		zend_update_property(fnx_config_ini_ce, instance, ZEND_STRL(FNX_CONFIG_PROPERT_NAME), filename TSRMLS_CC);

		return instance;
	} else if (filename && Z_TYPE_P(filename) == IS_STRING) {
	    struct stat sb;
	    zend_file_handle fh = {0};
		char *ini_file = Z_STRVAL_P(filename);
		
		MAKE_STD_ZVAL(configs);
		ZVAL_NULL(configs);

		if (VCWD_STAT(ini_file, &sb) == 0) {
			if (S_ISREG(sb.st_mode)) {
				if ((fh.handle.fp = VCWD_FOPEN(ini_file, "r"))) {
					fh.filename = ini_file;
					fh.type = ZEND_HANDLE_FP;
					FNX_G(active_ini_file_section) = NULL;

					FNX_G(parsing_flag) = FNX_CONFIG_INI_PARSING_START;
					if (section_name && Z_STRLEN_P(section_name)) {
						FNX_G(ini_wanted_section) = section_name;
					} else {
						FNX_G(ini_wanted_section) = NULL;
					}

	 				array_init(configs);
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2))
					if (zend_parse_ini_file(&fh, 0, 0 /* ZEND_INI_SCANNER_NORMAL */,
						   	(zend_ini_parser_cb_t)fnx_config_ini_parser_cb, configs TSRMLS_CC) == FAILURE
							|| Z_TYPE_P(configs) != IS_ARRAY)
#else
					if (zend_parse_ini_file(&fh, 0, (zend_ini_parser_cb_t)fnx_config_ini_parser_cb, configs) == FAILURE
							|| Z_TYPE_P(configs) != IS_ARRAY)
#endif
					{
						zval_dtor(configs);
						efree(configs);
						fnx_trigger_error(E_ERROR TSRMLS_CC, "Parsing ini file '%s' failed", ini_file);
						return NULL;
					}
				}
			}
		} else {
			efree(configs);
			fnx_trigger_error(E_ERROR TSRMLS_CC, "Unable to find config file '%s'", ini_file);
			return NULL;
		}

		if (section_name && Z_STRLEN_P(section_name)) {
			zval **section;
			zval tmp;
			if (zend_symtable_find(Z_ARRVAL_P(configs),
						Z_STRVAL_P(section_name), Z_STRLEN_P(section_name) + 1, (void **)&section) == FAILURE) {
				zval_dtor(configs);
				efree(configs);
				fnx_trigger_error(E_ERROR TSRMLS_CC, "There is no section '%s' in '%s'", Z_STRVAL_P(section_name), ini_file);
				return NULL;
			}
			INIT_PZVAL(&tmp);
			array_init(&tmp);
			zend_hash_copy(Z_ARRVAL(tmp), Z_ARRVAL_PP(section), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
			zval_dtor(configs);

			*configs = tmp;
		} 

		if (this_ptr) {
			instance = this_ptr;
		} else {
			MAKE_STD_ZVAL(instance);
			object_init_ex(instance, fnx_config_ini_ce);
		}

		zend_update_property(fnx_config_ini_ce, instance, ZEND_STRL(FNX_CONFIG_PROPERT_NAME), configs TSRMLS_CC);
		zval_ptr_dtor(&configs);

		return instance;
	} else {
		fnx_trigger_error(FNX_ERR_TYPE_ERROR TSRMLS_CC, "Invalid parameters provided, must be path of ini file");
		return NULL;
	}
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::__construct(mixed $config_path, string $section_name)
*/
PHP_METHOD(fnx_config_ini, __construct) {
	zval *filename, *section = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &filename, &section) == FAILURE) {
		zval *prop;
		MAKE_STD_ZVAL(prop);
		array_init(prop);
		zend_update_property(fnx_config_ini_ce, getThis(), ZEND_STRL(FNX_CONFIG_PROPERT_NAME), prop TSRMLS_CC);
		zval_ptr_dtor(&prop);
		return;
	}

	(void)fnx_config_ini_instance(getThis(), filename, section TSRMLS_CC);
}
/** }}} */

/** {{{ proto public Fnx_Config_Ini::get(string $name = NULL)
*/
PHP_METHOD(fnx_config_ini, get) {
	zval *ret, **ppzval;
	char *name;
	uint len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &len) == FAILURE) {
		return;
	}

	if (!len) {
		RETURN_ZVAL(getThis(), 1, 0);
	} else {
		zval *properties;
		char *entry, *seg, *pptr;

		properties = zend_read_property(fnx_config_ini_ce, getThis(), ZEND_STRL(FNX_CONFIG_PROPERT_NAME), 1 TSRMLS_CC);

		if (Z_TYPE_P(properties) != IS_ARRAY) {
			RETURN_NULL();
		}

		entry = estrndup(name, len);
		if ((seg = php_strtok_r(entry, ".", &pptr))) {
			while (seg) {
				if (zend_hash_find(Z_ARRVAL_P(properties), seg, strlen(seg) + 1, (void **) &ppzval) == FAILURE) {
					efree(entry);
					RETURN_NULL();
				}

				properties = *ppzval;
				seg = php_strtok_r(NULL, ".", &pptr);
			}
		} else {
			if (zend_hash_find(Z_ARRVAL_P(properties), name, len + 1, (void **)&ppzval) == FAILURE) {
				efree(entry);
				RETURN_NULL();
			}
		}

		efree(entry);

		if (Z_TYPE_PP(ppzval) == IS_ARRAY) {
			if ((ret = fnx_config_ini_format(getThis(), ppzval TSRMLS_CC))) {
				RETURN_ZVAL(ret, 1, 1);
			} else {
				RETURN_NULL();
			}
		} else {
			RETURN_ZVAL(*ppzval, 1, 0);
		}
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::toArray(void)
*/
PHP_METHOD(fnx_config_ini, toArray) {
	zval *properties = zend_read_property(fnx_config_ini_ce, getThis(), ZEND_STRL(FNX_CONFIG_PROPERT_NAME), 1 TSRMLS_CC);
	RETURN_ZVAL(properties, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::set($name, $value)
*/
PHP_METHOD(fnx_config_ini, set) {
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::__isset($name)
*/
PHP_METHOD(fnx_config_ini, __isset) {
	char * name;
	int len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		return;
	} else {
		zval *prop = zend_read_property(fnx_config_ini_ce, getThis(), ZEND_STRL(FNX_CONFIG_PROPERT_NAME), 1 TSRMLS_CC);
		RETURN_BOOL(zend_hash_exists(Z_ARRVAL_P(prop), name, len + 1));
	}
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::count($name)
*/
PHP_METHOD(fnx_config_ini, count) {
	zval *prop = zend_read_property(fnx_config_ini_ce, getThis(), ZEND_STRL(FNX_CONFIG_PROPERT_NAME), 1 TSRMLS_CC);
	RETURN_LONG(zend_hash_num_elements(Z_ARRVAL_P(prop)));
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::offsetUnset($index)
*/
PHP_METHOD(fnx_config_ini, offsetUnset) {
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::rewind(void)
*/
PHP_METHOD(fnx_config_ini, rewind) {
	zval *prop = zend_read_property(fnx_config_ini_ce, getThis(), ZEND_STRL(FNX_CONFIG_PROPERT_NAME), 1 TSRMLS_CC);
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(prop));
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::current(void)
*/
PHP_METHOD(fnx_config_ini, current) {
	zval *prop, **ppzval, *ret;

	prop = zend_read_property(fnx_config_ini_ce, getThis(), ZEND_STRL(FNX_CONFIG_PROPERT_NAME), 1 TSRMLS_CC);
	if (zend_hash_get_current_data(Z_ARRVAL_P(prop), (void **)&ppzval) == FAILURE) {
		RETURN_FALSE;
	}

	if (Z_TYPE_PP(ppzval) == IS_ARRAY) {
		if ((ret = fnx_config_ini_format(getThis(),  ppzval TSRMLS_CC))) {
			RETURN_ZVAL(ret, 1, 1);
		} else {
			RETURN_NULL();
		}
	} else {
		RETURN_ZVAL(*ppzval, 1, 0);
	}
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::key(void)
*/
PHP_METHOD(fnx_config_ini, key) {
	zval *prop;
	char *string;
	long index;

	prop = zend_read_property(fnx_config_ini_ce, getThis(), ZEND_STRL(FNX_CONFIG_PROPERT_NAME), 1 TSRMLS_CC);
	if (zend_hash_get_current_key(Z_ARRVAL_P(prop), &string, &index, 0) == HASH_KEY_IS_LONG) {
		RETURN_LONG(index);
	} else {
		RETURN_STRING(string, 1);
	}
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::next(void)
*/
PHP_METHOD(fnx_config_ini, next) {
	zval *prop = zend_read_property(fnx_config_ini_ce, getThis(), ZEND_STRL(FNX_CONFIG_PROPERT_NAME), 1 TSRMLS_CC);
	zend_hash_move_forward(Z_ARRVAL_P(prop));
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::valid(void)
*/
PHP_METHOD(fnx_config_ini, valid) {
	zval *prop = zend_read_property(fnx_config_ini_ce, getThis(), ZEND_STRL(FNX_CONFIG_PROPERT_NAME), 1 TSRMLS_CC);
	RETURN_LONG(zend_hash_has_more_elements(Z_ARRVAL_P(prop)) == SUCCESS);
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::readonly(void)
*/
PHP_METHOD(fnx_config_ini, readonly) {
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Fnx_Config_Ini::__destruct
*/
PHP_METHOD(fnx_config_ini, __destruct) {
}
/* }}} */

/** {{{ proto private Fnx_Config_Ini::__clone
*/
PHP_METHOD(fnx_config_ini, __clone) {
}
/* }}} */

/** {{{ fnx_config_ini_methods
*/
zend_function_entry fnx_config_ini_methods[] = {
	PHP_ME(fnx_config_ini, __construct,	fnx_config_ini_construct_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	/* PHP_ME(fnx_config_ini, __destruct,	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR) */
	PHP_ME(fnx_config_ini, __isset, fnx_config_ini_isset_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_config_ini, get,	fnx_config_ini_get_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_config_ini, set, fnx_config_ini_set_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_config_ini, count, fnx_config_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_config_ini, rewind, fnx_config_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_config_ini, current, fnx_config_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_config_ini, next, fnx_config_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_config_ini, valid, fnx_config_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_config_ini, key, fnx_config_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_config_ini, toArray, fnx_config_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_config_ini, readonly, fnx_config_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_config_ini, offsetUnset, fnx_config_ini_unset_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_config_ini, offsetGet, get, fnx_config_ini_rget_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_config_ini, offsetExists, __isset, fnx_config_ini_isset_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_config_ini, offsetSet, set, fnx_config_ini_set_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_config_ini, __get, get, fnx_config_ini_get_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_config_ini, __set, set, fnx_config_ini_set_arginfo, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(config_ini) {
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Config_Ini", "Fnx\\Config\\Ini", fnx_config_ini_methods);
	fnx_config_ini_ce = zend_register_internal_class_ex(&ce, fnx_config_ce, NULL TSRMLS_CC);

#ifdef HAVE_SPL
	zend_class_implements(fnx_config_ini_ce TSRMLS_CC, 3, zend_ce_iterator, zend_ce_arrayaccess, spl_ce_Countable);
#else
	zend_class_implements(fnx_config_ini_ce TSRMLS_CC, 2, zend_ce_iterator, zend_ce_arrayaccess);
#endif

	fnx_config_ini_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	return SUCCESS;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
