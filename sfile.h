//
// iva_n_@mail.ru
// Ivan Shipaev
// Simple file system

#ifndef _SFILE_H_
#define _SFILE_H_
#include "stdint.h"
#include "sfile_conf.h"
//------------------------------------------------------------------------
typedef struct sfs_t {
	struct {
		uint8_t *buf;
		uint32_t block;
		uint8_t fchange;
	} cache;
	uint32_t block_size;
	uint32_t block_count;
    int (*read)(uint32_t block, void *buffer);
    int (*prog)(uint32_t block, const void *buffer);
    int (*erase)(uint32_t block);
    SFS_MUTEX *mutex;
} sfs_t;
//------------------------------------------------------------------------
typedef struct sfile_t {
	sfs_t *fs;
	uint32_t msize;		// максимальный размер данных
	uint32_t start;		// адрес указатель на начала файла
	uint32_t offset;
} sfile_t;
//------------------------------------------------------------------------
void s_fs_init(sfs_t* fs, void *buffer,
		uint32_t block_size, uint32_t block_count,
		int (*read)(uint32_t block, void *buffer),
		int (*prog)(uint32_t block, const void *buffer),
		int (*erase)(uint32_t block), SFS_MUTEX* mutex);
int s_fs_sync(sfs_t *fs);
//---------------------------------------------------------------------
int s_flush(sfile_t *file);
int s_write(sfile_t *file, const void *buf, uint32_t len);
int s_read(sfile_t *file, void *buf, uint32_t len);
void s_open(sfs_t* fs, sfile_t *file, uint32_t addr, uint32_t msize);
int s_seek(sfile_t* file, int offset);
uint32_t s_getoff(sfile_t* file);
int s_msize(sfile_t *file);
#endif /*_SFILE_H_*/
