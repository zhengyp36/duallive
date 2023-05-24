#ifndef _XUTILS_FILE_MAP_H
#define _XUTILS_FILE_MAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Example 1:
 * -----------------------------------------------------------------------------
 *   file_map_t fm;
 *
 *   file_map_init(&fm, "test.dat");
 *   if (file_map_set_size(&fm, 1024 * 1024))
 *       die("set %s size error", fm.path);
 *
 *   if (file_map_open(&fm))
 *       die("open %s error", fm.path);
 *
 *   snprintf(fm.buf, fm.size, "Hello, world\n");
 *   file_map_fini(&fm);
 * -----------------------------------------------------------------------------
 *
 * Example 2:
 * -----------------------------------------------------------------------------
 *   file_map_t fm;
 *
 *   file_map_init(&fm, "/dev/sdc");
 *   if (file_map_open(&fm, FM_RDONLY))
 *       die("open %s error", fm.path);
 *
 *   char second_sector[512];
 *   memcpy(second_sector, fm.buf+512, sizeof(second_sector));
 *
 *   file_map_fini(&fm);
 * -----------------------------------------------------------------------------
 */

enum {
	FM_NORMAL,
	FM_RDONLY,
	FM_PRIVATE,
};

#define FM_SIZE_BITS 62

typedef struct file_map {
	const char *	path;
	void *		buf;
	uint64_t	size : FM_SIZE_BITS;
	uint64_t	mode : 64 - FM_SIZE_BITS;
} file_map_t;

void file_map_init(file_map_t *fm, const char *path);
int  file_map_set_size(file_map_t *fm, size_t size);
int  file_map_expand(file_map_t *fm, size_t size);
void file_map_fini(file_map_t *fm);

int file_map_open_(file_map_t *fm, int quiet, int reopen, int mode, ...);
int file_map_sync(file_map_t *fm);

#define file_map_open(fm, ...)						\
	file_map_open_(fm, 0, 0, ##__VA_ARGS__, FM_NORMAL)

#define file_map_reopen(fm, ...)					\
	file_map_open_(fm, 0, 1, ##__VA_ARGS__, FM_NORMAL)

#define file_map_open_quiet(fm, ...)					\
	file_map_open_(fm, 1, 0, ##__VA_ARGS__, FM_NORMAL)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _XUTILS_FILE_MAP_H
