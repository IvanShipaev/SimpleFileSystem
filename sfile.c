//
// iva_n_@mail.ru
// Ivan Shipaev
// Simple file system

#define _SFILE_

#include "compiler.h"
#include "string.h"
#include "sfile.h"
#include "crc.h"
//------------------------------------------------------------------------
typedef struct C_file_head {
	uint32_t len;
	uint32_t crc;
} C_file_head;
static int __s_fs_sync(struct _S_FILESYSTEM *fs);
//---------------------------------------------------------------------
static int __s_fs_read(struct _S_FILESYSTEM *fs, uint32_t block)
{
	if (block > fs->block_count) return -1;
	if (fs->read(block, fs->cache.buf)<0) return -1;
	fs->cache.block = block;
	fs->cache.f_change_block = 0;
	return 0;
}
//---------------------------------------------------------------------
static int __s_memcpy_from(S_FILESYSTEM *fs, void* pram, uint32_t addr_flash, uint32_t len)
{
	uint32_t block = addr_flash/fs->block_size;
	uint32_t off = addr_flash%fs->block_size;
	uint8_t *p = pram;
	int tlen;
	while(len) {
		tlen = Min(fs->block_size - off, len);
		if (fs->cache.block != block) {
			if (__s_fs_sync(fs)<0) return -1;
			if (__s_fs_read(fs, block)<0) return -1;
		}
		memcpy(p, fs->cache.buf + off, tlen);
		off = 0;
		p += tlen;
		len -= tlen;
		block++;
	}
	return 0;
}
static int __s_memcpy_to(S_FILESYSTEM *fs, uint32_t addr_flash, const void* pram, uint32_t len)
{
	uint32_t block = addr_flash/fs->block_size;
	uint32_t off = addr_flash%fs->block_size;
	const uint8_t *p = pram;
	int tlen;
	while(len) {
		tlen = Min(fs->block_size - off, len);
		if (fs->cache.block != block) {
			if (__s_fs_sync(fs)<0) return -1;
			if (__s_fs_read(fs, block)<0) return -1;
		}
		if (memcmp(fs->cache.buf + off, p, tlen)) {
			memcpy(fs->cache.buf + off, p, tlen);
			fs->cache.f_change_block = 1;
		}
		off = 0;
		p += tlen;
		len -= tlen;
		block++;
	}
	return 0;
}

//---------------------------------------------------------------------
// Not thread-safe function
//---------------------------------------------------------------------
static int __s_fs_sync(struct _S_FILESYSTEM *fs)
{
	if (fs->cache.f_change_block) {
		if (fs->erase(fs->cache.block)<0) return -1;
		if (fs->prog(fs->cache.block, fs->cache.buf)<0) return -1;
		fs->cache.f_change_block = 0;
	}
	return 0;
}
//---------------------------------------------------------------------
static int __s_flush(S_FILE *file)
{
	return __s_fs_sync(file->fs);
}
//---------------------------------------------------------------------
static int __s_write(S_FILE *file, const void *buf, uint32_t len)
{
	if (file->msize - file->offset <= len) return -1;
	if(__s_memcpy_to(file->fs, file->start + file->offset, buf, len) < 0) return -1;
	file->offset += len;
	return 0;
}
//---------------------------------------------------------------------
static int __s_read(S_FILE *file, void *buf, uint32_t len)
{
	if (file->msize - file->offset <= len) return -1;
	if (__s_memcpy_from(file->fs, buf, file->start + file->offset, len) < 0) return -1;
	file->offset += len;
	return 0;
}
//---------------------------------------------------------------------
// Thread-safe function
//---------------------------------------------------------------------
#if (S_FS_USE_MUTEX)
void s_init_fs(S_FILESYSTEM* fs, void *buffer, uint32_t block_size,
			   uint32_t block_count, int (*read)(uint32_t block, void *buffer),
			   int (*prog)(uint32_t block, const void *buffer),
			   int (*erase)(uint32_t block), em_mutex_t* mutex)
#else
void s_init_fs(S_FILESYSTEM* fs, void *buffer, uint32_t block_size,
			   uint32_t block_count, int (*read)(uint32_t block, void *buffer),
			   int (*prog)(uint32_t block, const void *buffer),
			   int (*erase)(uint32_t block))
#endif
{
	fs->cache.f_change_block = 0;
	fs->cache.block = 0xffffffff;
	fs->cache.buf = buffer;
	fs->block_size = block_size;
	fs->block_count = block_count;
	fs->read = read;
	fs->prog = prog;
	fs->erase = erase;
#if (S_FS_USE_MUTEX)
	fs->mutex = mutex ? mutex : em_mutex_new();
	EM_ASSERT(fs->mutex);
#endif
}
//---------------------------------------------------------------------
int s_fs_sync(struct _S_FILESYSTEM *fs)
{
#if (S_FS_USE_MUTEX)
	em_mutex_take(fs->mutex, EM_TIM_INFINITY);
	int rc = __s_fs_sync(fs);
	em_mutex_give(fs->mutex);
	return rc;
#else
	return __s_fs_sync(fs);
#endif
}
//---------------------------------------------------------------------
int s_flush(S_FILE *file)
{
#if (S_FS_USE_MUTEX)
	em_mutex_take(file->fs->mutex, EM_TIM_INFINITY);
	int rc = __s_flush(file);
	em_mutex_give(file->fs->mutex);
#else
	int rc = __s_flush(file);
#endif
	return rc;
}
//---------------------------------------------------------------------
int s_write(S_FILE *file, const void *buf, uint32_t len)
{
#if (S_FS_USE_MUTEX)
	em_mutex_take(file->fs->mutex, EM_TIM_INFINITY);
	int rc = __s_write(file, buf, len);
	em_mutex_give(file->fs->mutex);
#else
	int rc = __s_write(file, buf, len);
#endif
	return rc;
}
//---------------------------------------------------------------------
int s_read(S_FILE *file, void *buf, uint32_t len)
{
#if (S_FS_USE_MUTEX)
	em_mutex_take(file->fs->mutex, EM_TIM_INFINITY);
	int rc = __s_read(file, buf, len);
	em_mutex_give(file->fs->mutex);
#else
	int rc = __s_read(file, buf, len);
#endif
	return rc;
}
//---------------------------------------------------------------------
void s_open(S_FILESYSTEM* fs, S_FILE *file, uint32_t addr, uint32_t msize)
{
	file->fs = fs;
	file->msize = msize;
	file->start = addr;
	file->offset = 0;
}
//---------------------------------------------------------------------
int s_seek(S_FILE* file, int offset)
{
	if (offset >= file->msize) return -1;
	file->offset = offset;
	return 0;
}
//---------------------------------------------------------------------
uint32_t s_getoff(S_FILE* file)
{
	return file->offset;
}
//---------------------------------------------------------------------
int s_msize(S_FILE *file)
{
	return file->msize;
}
