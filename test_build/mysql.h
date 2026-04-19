#ifndef _STUB_MYSQL_H
#define _STUB_MYSQL_H
#include <stdint.h>
#include <string.h>
typedef struct{int d;}MYSQL;
typedef struct{int d;}MYSQL_RES;
typedef char** MYSQL_ROW;
static inline MYSQL*mysql_init(MYSQL*m){(void)m;return(MYSQL*)1;}
static inline MYSQL*mysql_real_connect(MYSQL*m,const char*h,const char*u,const char*p,const char*d,int port,void*s,int f){(void)m;(void)h;(void)u;(void)p;(void)d;(void)port;(void)s;(void)f;return m;}
static inline int mysql_query(MYSQL*m,const char*q){(void)m;(void)q;return 0;}
static inline MYSQL_RES*mysql_store_result(MYSQL*m){(void)m;return 0;}
static inline int mysql_num_fields(MYSQL_RES*r){(void)r;return 0;}
static inline unsigned long long mysql_num_rows(MYSQL_RES*r){(void)r;return 0;}
static inline MYSQL_ROW mysql_fetch_row(MYSQL_RES*r){(void)r;return 0;}
static inline void mysql_free_result(MYSQL_RES*r){(void)r;}
static inline void mysql_close(MYSQL*m){(void)m;}
static inline const char*mysql_error(MYSQL*m){(void)m;return"";}
static inline unsigned long mysql_real_escape_string(MYSQL*m,char*to,const char*from,unsigned long len){(void)m;if(to&&from)strncpy(to,from,len);return len;}
static inline int mysql_select_db(MYSQL*m,const char*d){(void)m;(void)d;return 0;}
static inline unsigned long long mysql_insert_id(MYSQL*m){(void)m;return 0;}
#endif
