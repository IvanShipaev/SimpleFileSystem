//
// iva_n_@mail.ru
// Ivan Shipaev
// Simple file system

#ifndef _SFILE_H_
#define _SFILE_H_
#include "stdint.h"
#include "task_include.h"
/*
 * #define S_FS_USE_MUTEX	1  - использовать мьютек
 * #define S_FS_USE_MUTEX	0  - не использовать мьютек
 */
#define S_FS_USE_MUTEX	1
//------------------------------------------------------------------------
typedef struct _S_FILESYSTEM {
	struct {
		uint8_t f_change_block;
		uint8_t *buf;
		uint32_t block;
	} cache;
	uint32_t block_size;
	uint32_t block_count;
    int (*read)(uint32_t block, void *buffer);
    int (*prog)(uint32_t block, const void *buffer);
    int (*erase)(uint32_t block);
#if (S_FS_USE_MUTEX)
    em_mutex_t *mutex;
#endif

} S_FILESYSTEM;
//------------------------------------------------------------------------
typedef struct _S_FILE {
	S_FILESYSTEM *fs;
	uint32_t msize;		// максимальный размер данных
	uint32_t start;		// адрес указатель на начала файла
	uint32_t offset;
} S_FILE;
//------------------------------------------------------------------------
#if (S_FS_USE_MUTEX)
void s_init_fs(S_FILESYSTEM* fs, void *buffer, uint32_t block_size,
			   uint32_t block_count, int (*read)(uint32_t block, void *buffer),
			   int (*prog)(uint32_t block, const void *buffer),
			   int (*erase)(uint32_t block), em_mutex_t* mutex);
#else
void s_init_fs(S_FILESYSTEM* fs, void *buffer, uint32_t block_size,
			   uint32_t block_count, int (*read)(uint32_t block, void *buffer),
			   int (*prog)(uint32_t block, const void *buffer),
			   int (*erase)(uint32_t block));
#endif
int s_fs_sync(struct _S_FILESYSTEM *fs);
//---------------------------------------------------------------------
int s_flush(S_FILE *file);
int s_write(S_FILE *file, const void *buf, uint32_t len);
int s_read(S_FILE *file, void *buf, uint32_t len);
void s_open(S_FILESYSTEM* fs, S_FILE *file, uint32_t addr, uint32_t msize);
int s_seek(S_FILE* file, int offset);
uint32_t s_getoff(S_FILE* file);
int s_msize(S_FILE *file);
#endif /*_SFILE_H_*/
