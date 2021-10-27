//
// iva_n_@mail.ru
// Ivan Shipaev
// Simple file system

#define _SFILE_

#include "stdlib.h"
#include "string.h"
#include "sfile.h"
static int __s_fs_sync(sfs_t *fs);
//---------------------------------------------------------------------
static int __s_fs_read(sfs_t *fs, uint32_t block)
{
	if (block > fs->block_count) return -1;
	if (fs->read(block, fs->cache.buf)<0) return -1;
	fs->cache.block = block;
	fs->cache.fchange = 0;
	return 0;
}
//---------------------------------------------------------------------
static int __s_memcpy_from(sfs_t *fs, void* pram, uint32_t addr, uint32_t len)
{
	uint32_t block = addr/fs->block_size;
	uint32_t off = addr%fs->block_size;
	uint8_t *p = pram;
	int tlen;
	while(len) {
		tlen = fs->block_size - off;
		tlen = __min(tlen, len);
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
static int __s_memcpy_to(sfs_t *fs, uint32_t addr, const void* pram, uint32_t len)
{
	uint32_t block = addr/fs->block_size;
	uint32_t off = addr%fs->block_size;
	const uint8_t *p = pram;
	int tlen;
	while(len) {
		tlen = fs->block_size - off;
		tlen = __min(tlen, len);
		if (fs->cache.block != block) {
			if (__s_fs_sync(fs)<0) return -1;
			if (__s_fs_read(fs, block)<0) return -1;
		}
		if (memcmp(fs->cache.buf + off, p, tlen)) {
			memcpy(fs->cache.buf + off, p, tlen);
			fs->cache.fchange = 1;
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
static int __s_fs_sync(sfs_t *fs)
{
	if (fs->cache.fchange) {
		if (fs->erase(fs->cache.block)<0) return -1;
		if (fs->prog(fs->cache.block, fs->cache.buf)<0) return -1;
		fs->cache.fchange = 0;
	}
	return 0;
}
//---------------------------------------------------------------------
static inline int __s_flush(sfile_t *file)
{
	return __s_fs_sync(file->fs);
}
//---------------------------------------------------------------------
static int __s_write(sfile_t *file, const void *buf, uint32_t len)
{
	if (file->msize - file->offset < len) return -1;
	if(__s_memcpy_to(file->fs, file->start + file->offset, buf, len) < 0) return -1;
	file->offset += len;
	return 0;
}
//---------------------------------------------------------------------
static int __s_read(sfile_t *file, void *buf, uint32_t len)
{
	if (file->msize - file->offset < len) return -1;
	if (__s_memcpy_from(file->fs, buf, file->start + file->offset, len) < 0) return -1;
	file->offset += len;
	return 0;
}
//---------------------------------------------------------------------
// Thread-safe function
//---------------------------------------------------------------------
void s_fs_init(sfs_t* fs, void *buffer,
		uint32_t block_size, uint32_t block_count,
		int (*read)(uint32_t block, void *buffer),
		int (*prog)(uint32_t block, const void *buffer),
		int (*erase)(uint32_t block), SFS_MUTEX* mutex)
{
	fs->cache.fchange = 0;
	fs->cache.block = 0xffffffff;
	fs->cache.buf = buffer;
	fs->block_size = block_size;
	fs->block_count = block_count;
	fs->read = read;
	fs->prog = prog;
	fs->erase = erase;
	fs->mutex = mutex;
}
//---------------------------------------------------------------------
int s_fs_sync(sfs_t *fs)
{
	int rc;
	SFS_MUTEX_TAKE(fs->mutex);
	rc = __s_fs_sync(fs);
	SFS_MUTEX_GIVE(fs->mutex);
	return rc;
}
//---------------------------------------------------------------------
int s_flush(sfile_t *file)
{
	int rc;
	SFS_MUTEX_TAKE(file->fs->mutex);
	rc = __s_flush(file);
	SFS_MUTEX_GIVE(file->fs->mutex);
	return rc;
}
//---------------------------------------------------------------------
int s_write(sfile_t *file, const void *buf, uint32_t len)
{
	int rc;
	SFS_MUTEX_TAKE(file->fs->mutex);
	rc = __s_write(file, buf, len);
	SFS_MUTEX_GIVE(file->fs->mutex);
	return rc;
}
//---------------------------------------------------------------------
int s_read(sfile_t *file, void *buf, uint32_t len)
{
	int rc;
	SFS_MUTEX_TAKE(file->fs->mutex);
	rc = __s_read(file, buf, len);
	SFS_MUTEX_GIVE(file->fs->mutex);
	return rc;
}
//---------------------------------------------------------------------
void s_open(sfs_t* fs, sfile_t *file, uint32_t addr, uint32_t msize)
{
	file->fs = fs;
	file->msize = msize;
	file->start = addr;
	file->offset = 0;
}
//---------------------------------------------------------------------
int s_seek(sfile_t* file, int offset)
{
	if (offset > file->msize) return -1;
	file->offset = offset;
	return 0;
}
//---------------------------------------------------------------------
uint32_t s_getoff(sfile_t* file)
{
	return file->offset;
}
//---------------------------------------------------------------------
int s_msize(sfile_t *file)
{
	return file->msize;
}
