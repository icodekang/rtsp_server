#ifndef __vlog_h
#define __vlog_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h> /* for va_list */
#include <stdio.h> /* for size_t */

# if defined __GNUC__
#   define vlog_CHECK_PRINTF(m,n) __attribute__((format(printf,m,n)))
# else 
#   define vlog_CHECK_PRINTF(m,n)
# endif

typedef struct vlog_category_s vlog_category_t;

int vlog_init(const char *config);
int vlog_reload(const char *config);
void vlog_fini(void);

void vlog_profile(void);

vlog_category_t *vlog_get_category(const char *cname);
int vlog_level_enabled(vlog_category_t *category, const int level);

int vlog_put_mdc(const char *key, const char *value);
char *vlog_get_mdc(const char *key);
void vlog_remove_mdc(const char *key);
void vlog_clean_mdc(void);

int vlog_level_switch(vlog_category_t * category, int level);
int vlog_level_enabled(vlog_category_t * category, int level);

void vlog(vlog_category_t * category,
	const char *file, size_t filelen,
	const char *func, size_t funclen,
	long line, int level,
	const char *format, ...) vlog_CHECK_PRINTF(8,9);
void vvlog(vlog_category_t * category,
	const char *file, size_t filelen,
	const char *func, size_t funclen,
	long line, int level,
	const char *format, va_list args);
void hvlog(vlog_category_t * category,
	const char *file, size_t filelen,
	const char *func, size_t funclen,
	long line, int level,
	const void *buf, size_t buflen);

int dvlog_init(const char *confpath, const char *cname);
int dvlog_set_category(const char *cname);

void dvlog(const char *file, size_t filelen,
	const char *func, size_t funclen,
	long line, int level,
	const char *format, ...) vlog_CHECK_PRINTF(7,8);
void vdvlog(const char *file, size_t filelen,
	const char *func, size_t funclen,
	long line, int level,
	const char *format, va_list args);
void hdvlog(const char *file, size_t filelen,
	const char *func, size_t funclen,
	long line, int level,
	const void *buf, size_t buflen);

typedef struct vlog_msg_s {
	char *buf;
	size_t len;
	char *path;
} vlog_msg_t;

typedef int (*vlog_record_fn)(vlog_msg_t *msg);
int vlog_set_record(const char *rname, vlog_record_fn record);

const char *vlog_version(void);

/******* useful macros, can be redefined at user's h file **********/

typedef enum {
	vlog_LEVEL_DEBUG = 20,
	vlog_LEVEL_INFO = 40,
	vlog_LEVEL_NOTICE = 60,
	vlog_LEVEL_WARN = 80,
	vlog_LEVEL_ERROR = 100,
	vlog_LEVEL_FATAL = 120
} vlog_level; 

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
# if defined __GNUC__ && __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
/* vlog macros */
#define vlog_fatal(cat, ...) \
	vlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_FATAL, __VA_ARGS__)
#define vlog_error(cat, ...) \
	vlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_ERROR, __VA_ARGS__)
#define vlog_warn(cat, ...) \
	vlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_WARN, __VA_ARGS__)
#define vlog_notice(cat, ...) \
	vlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_NOTICE, __VA_ARGS__)
#define vlog_info(cat, ...) \
	vlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_INFO, __VA_ARGS__)
#define vlog_debug(cat, ...) \
	vlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_DEBUG, __VA_ARGS__)
/* dvlog macros */
#define dvlog_fatal(...) \
	dvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_FATAL, __VA_ARGS__)
#define dvlog_error(...) \
	dvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_ERROR, __VA_ARGS__)
#define dvlog_warn(...) \
	dvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_WARN, __VA_ARGS__)
#define dvlog_notice(...) \
	dvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_NOTICE, __VA_ARGS__)
#define dvlog_info(...) \
	dvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_INFO, __VA_ARGS__)
#define dvlog_debug(...) \
	dvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_DEBUG, __VA_ARGS__)
#elif defined __GNUC__
/* vlog macros */
#define vlog_fatal(cat, format, args...) \
	vlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_FATAL, format, ##args)
#define vlog_error(cat, format, args...) \
	vlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_ERROR, format, ##args)
#define vlog_warn(cat, format, args...) \
	vlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_WARN, format, ##args)
#define vlog_notice(cat, format, args...) \
	vlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_NOTICE, format, ##args)
#define vlog_info(cat, format, args...) \
	vlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_INFO, format, ##args)
#define vlog_debug(cat, format, args...) \
	vlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_DEBUG, format, ##args)
/* dvlog macros */
#define dvlog_fatal(format, args...) \
	dvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_FATAL, format, ##args)
#define dvlog_error(format, args...) \
	dvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_ERROR, format, ##args)
#define dvlog_warn(format, args...) \
	dvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_WARN, format, ##args)
#define dvlog_notice(format, args...) \
	dvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_NOTICE, format, ##args)
#define dvlog_info(format, args...) \
	dvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_INFO, format, ##args)
#define dvlog_debug(format, args...) \
	dvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_DEBUG, format, ##args)
#endif

/* vvlog macros */
#define vvlog_fatal(cat, format, args) \
	vvlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_FATAL, format, args)
#define vvlog_error(cat, format, args) \
	vvlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_ERROR, format, args)
#define vvlog_warn(cat, format, args) \
	vvlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_WARN, format, args)
#define vvlog_notice(cat, format, args) \
	vvlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_NOTICE, format, args)
#define vvlog_info(cat, format, args) \
	vvlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_INFO, format, args)
#define vvlog_debug(cat, format, args) \
	vvlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_DEBUG, format, args)

/* hvlog macros */
#define hvlog_fatal(cat, buf, buf_len) \
	hvlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_FATAL, buf, buf_len)
#define hvlog_error(cat, buf, buf_len) \
	hvlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_ERROR, buf, buf_len)
#define hvlog_warn(cat, buf, buf_len) \
	hvlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_WARN, buf, buf_len)
#define hvlog_notice(cat, buf, buf_len) \
	hvlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_NOTICE, buf, buf_len)
#define hvlog_info(cat, buf, buf_len) \
	hvlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_INFO, buf, buf_len)
#define hvlog_debug(cat, buf, buf_len) \
	hvlog(cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_DEBUG, buf, buf_len)


/* vdvlog macros */
#define vdvlog_fatal(format, args) \
	vdvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_FATAL, format, args)
#define vdvlog_error(format, args) \
	vdvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_ERROR, format, args)
#define vdvlog_warn(format, args) \
	vdvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_WARN, format, args)
#define vdvlog_notice(format, args) \
	vdvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_NOTICE, format, args)
#define vdvlog_info(format, args) \
	vdvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_INFO, format, args)
#define vdvlog_debug(format, args) \
	vdvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_DEBUG, format, args)

/* hdvlog macros */
#define hdvlog_fatal(buf, buf_len) \
	hdvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_FATAL, buf, buf_len)
#define hdvlog_error(buf, buf_len) \
	hdvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_ERROR, buf, buf_len)
#define hdvlog_warn(buf, buf_len) \
	hdvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_WARN, buf, buf_len)
#define hdvlog_notice(buf, buf_len) \
	hdvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_NOTICE, buf, buf_len)
#define hdvlog_info(buf, buf_len) \
	hdvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_INFO, buf, buf_len)
#define hdvlog_debug(buf, buf_len) \
	hdvlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	vlog_LEVEL_DEBUG, buf, buf_len)

/* enabled macros */
#define vlog_fatal_enabled(vlg)  vlog_level_enabled(vlg, vlog_LEVEL_FATAL)
#define vlog_error_enabled(vlg)  vlog_level_enabled(vlg, vlog_LEVEL_ERROR)
#define vlog_warn_enabled(vlg)   vlog_level_enabled(vlg, vlog_LEVEL_WARN)
#define vlog_notice_enabled(vlg) vlog_level_enabled(vlg, vlog_LEVEL_NOTICE)
#define vlog_info_enabled(vlg)   vlog_level_enabled(vlg, vlog_LEVEL_INFO)
#define vlog_debug_enabled(vlg)  vlog_level_enabled(vlg, vlog_LEVEL_DEBUG)

#define log_fatal(...)  dvlog_fatal(__VA_ARGS__)
#define log_error(...)  dvlog_error(__VA_ARGS__)
#define log_warn(...)   dvlog_warn(__VA_ARGS__)
#define log_notice(...) dvlog_notice(__VA_ARGS__)
#define log_info(...)   dvlog_info(__VA_ARGS__)
#define log_debug(...)  dvlog_debug(__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
