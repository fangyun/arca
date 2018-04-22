#ifndef ARCA_UTIL_H
#define ARCA_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define MIN(a, b) ((a) < (b) ? (a) : (b));
#define MAX(a, b) ((a) > (b) ? (a) : (b));

/* 如果@str以@prefix开头则返回true */
int str_prefix(char *prefix, char *str);

/* 以错误消息终止 */
void die(const char *format, ...)  __attribute__ ((noreturn));

/* 在系统调用失败后终止。 (调用perror()) */
void fail(char *msg) __attribute__ ((noreturn));

int file_exists(const char *name);

/**************************************************************************************************/
/* 数据文件 */

/* 查找数据文件在下列位置:
 * 1) 当前目录
 * 2) DATA_DIR 环境变量。 / 编译时缺省
 * 复制第一个匹配到@buffer (如果没有匹配的@filename仍然被复制. */
#define get_data_file(buffer, filename)    get_data_file_(buffer, sizeof(buffer), filename)
void get_data_file_(char buffer[], int size, const char *filename);

/* get_data_file() + fopen() */
FILE *fopen_data_file(const char *filename, const char *mode);


/**************************************************************************************************/
/* 可移植性的定义. */

#ifdef _WIN32

/*
 * 有时我们使用winsock，并希望避免警告，只在winsock2.h之后包含windows.h。
 */
#include <winsock2.h>
#include <windows.h>
#include <ctype.h>

#define sleep(seconds) Sleep((seconds) * 1000)
#define __sync_fetch_and_add(ap, b) InterlockedExchangeAdd((LONG volatile *) (ap), (b));
#define __sync_fetch_and_sub(ap, b) InterlockedExchangeAdd((LONG volatile *) (ap), -((LONG)b));

/* MinGW gcc, 没有内置函数stpcpy()的函数原型。 */
char *stpcpy (char *dest, const char *src);

const char *strcasestr(const char *haystack, const char *needle);

#endif /* _WIN32 */


/**************************************************************************************************/
/* 其它定义. */

/* 在大型配置中，使用DOUBLE_FLOATING =1，计数 > 1M，在那里24位floating_t尾数不够*/
#ifdef DOUBLE_FLOATING
#  define floating_t double
#  define PRIfloating "%lf"
#else
#  define floating_t float
#  define PRIfloating "%f"
#endif

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect((x), 0)

static inline void *
checked_malloc(size_t size, char *filename, unsigned int line, const char *func)
{
	void *p = malloc(size);
	if (!p)
		die("%s:%u: %s: 内存用尽 malloc(%u)\n",
		    filename, line, func, (unsigned) size);
	return p;
}

static inline void *
checked_calloc(size_t nmemb, size_t size, const char *filename, unsigned int line, const char *func)
{
	void *p = calloc(nmemb, size);
	if (!p)
		die("%s:%u: %s: 内存用尽 calloc(%u, %u)\n",
		    filename, line, func, (unsigned) nmemb, (unsigned) size);
	return p;
}

#define malloc2(size)        checked_malloc((size), __FILE__, __LINE__, __func__)
#define calloc2(nmemb, size) checked_calloc((nmemb), (size), __FILE__, __LINE__, __func__)

#define checked_write(fd, pt, size)	(assert(write((fd), (pt), (size)) == (size)))
#define checked_fread(pt, size, n, f)   (assert(fread((pt), (size), (n), (f)) == (n)))


/**************************************************************************************************/
/* 简单的字符串缓冲存储输出初始大小必须足够存储所有输出，否则程序将中止。可以使用不同的方法分配字符串和结构，参见下面。*/

typedef struct
{
	int remaining;
	char *str;       /* whole output */
	char *cur;
} strbuf_t;

/* 初始化传入字符串缓冲区。返回buf. */
strbuf_t *strbuf_init(strbuf_t *buf, char *buffer, int size);

/* 初始化传入字符串缓冲区。返回buf。
 * 内部字符串malloc()后，必须在以后释放。 */
strbuf_t *strbuf_init_alloc(strbuf_t *buf, int size);

/* 创建新字符串缓冲区，内部使用malloc()分配内存.
 * 两者都必须以后free()掉. */
strbuf_t *new_strbuf(int size);


/* 字符串缓冲版本的printf():
 * 使用sbprintf(buf, format，…)来积累输出. */
int strbuf_printf(strbuf_t *buf, const char *format, ...);
#define sbprintf strbuf_printf


#endif
