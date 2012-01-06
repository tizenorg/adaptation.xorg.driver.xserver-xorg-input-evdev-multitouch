/*
 * xserver-xorg-input-evdev-multitouch
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Sung-Jin Park <sj76.park@samsung.com>
 *          Sangjin LEE <lsj119@samsung.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.  Red
 * Hat makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SYNC_DEBUG_H
#define _SYNC_DEBUG_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <semaphore.h>

#ifdef __GNUC__
#define MAY_NOT_BE_USED __attribute__ ((unused))
#else
#define MAY_NOT_BE_USED
#endif 


#define SYNC_DEBUG_SHM_KEY	0xDEAD2012
#define SHM_MESG_SIZE		( 1024 * 1024 )

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

typedef struct {
	int initialized;

	sem_t sem;

	int count;

	int cur_pos;

	int isLogging;

	int isPrintEnabled;

	unsigned long tvStart;

	int _shmid;

	char debug_msg[0];
} VSYNC_DEBUG_INFO;

static volatile VSYNC_DEBUG_INFO * _vsync_debug = NULL;

static int
_vsync_debug_init(void)
{
	int shmid = 0;

	struct shmid_ds ds;

	shmid = shmget(SYNC_DEBUG_SHM_KEY, SHM_MESG_SIZE, IPC_CREAT | IPC_RMID);
	if (shmid < 0)
	{
		fprintf(stderr, "\tError : shmget size:%d\n", SHM_MESG_SIZE);
		goto bail;
	}

	if (shmctl(shmid, IPC_STAT, &ds) < 0)
	{
		fprintf(stderr, "\tError : shmctl\n");
		goto bail;
	}

	if (ds.shm_segsz < SHM_MESG_SIZE)
	{
		fprintf(stderr, "\tError : size check\n");
		goto bail;
	}

	_vsync_debug = shmat(shmid, NULL, 0);
	if (_vsync_debug == (void *) -1)
	{
		fprintf(stderr, "\tError : shmat\n");
		goto bail;
	}

	return shmid;

bail:
	fprintf(stderr, "VSYNC_DEBUG.... Error\n");

	if (_vsync_debug != NULL && _vsync_debug != (void *) -1)
		shmdt((void*)_vsync_debug);

	return -1;
}

static unsigned long
_vsync_debug_get_time(void)
{
	struct timespec tp;

	if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
		return (tp.tv_sec * 1000) + (tp.tv_nsec / 1000000L);

	return 0;		
}

static void
_vsync_debug_lock(void)
{
	if (sem_wait((sem_t *) &_vsync_debug->sem) < 0)
		fprintf(stderr, "sem_wait error\n");
}


static void
_vsync_debug_unlock(void)
{
	if (sem_post((sem_t *) &_vsync_debug->sem) < 0)
		fprintf(stderr, "sem_post error\n");
}

static void
_vsync_debug_set_slot(int isSync, const char * format, va_list args)
{
	unsigned long cur_time;

	if (_vsync_debug == NULL)
		_vsync_debug_init();

	if (!_vsync_debug->initialized)
		return;

	if (! _vsync_debug->isLogging)
		return;

	if (!_vsync_debug->isPrintEnabled)
		return;

	if (_vsync_debug->cur_pos > SHM_MESG_SIZE - 1024)
		return;

	cur_time = _vsync_debug_get_time();

	_vsync_debug_lock();

	if (isSync)
	{
		_vsync_debug->count++;
		_vsync_debug->cur_pos += sprintf((char *) _vsync_debug->debug_msg + _vsync_debug->cur_pos, "[32m");

	}
	_vsync_debug->cur_pos += sprintf((char *) _vsync_debug->debug_msg + _vsync_debug->cur_pos, "%6ld ", cur_time);
	_vsync_debug->cur_pos += vsprintf((char *) _vsync_debug->debug_msg + _vsync_debug->cur_pos, format, args);
	_vsync_debug->debug_msg[_vsync_debug->cur_pos++] = '\n';
	_vsync_debug->debug_msg[_vsync_debug->cur_pos] = '\0';

	if (isSync)
		_vsync_debug->cur_pos += sprintf((char *) _vsync_debug->debug_msg + _vsync_debug->cur_pos, "[0m");

	_vsync_debug_unlock();
}

static void MAY_NOT_BE_USED
vsync_debug_start(void)
{
	if (_vsync_debug == NULL)
		_vsync_debug_init();

	if (!_vsync_debug->initialized)
		return;

	_vsync_debug->isLogging = 1;
	_vsync_debug->tvStart = _vsync_debug_get_time();
}

static void MAY_NOT_BE_USED
vsync_debug_stop(void)
{
	unsigned long tvStop;

	if (_vsync_debug == NULL)
		_vsync_debug_init();

	if (!_vsync_debug->initialized)
		return;

	_vsync_debug->isLogging = 0;
	tvStop = _vsync_debug_get_time();
	
	_vsync_debug_lock();
	if (_vsync_debug->isPrintEnabled)
	{
		//Print Debug
		fprintf(stderr, "VSYNC DEBUG: count:%d, start:%6ld, end:%6ld\n", _vsync_debug->count, _vsync_debug->tvStart, tvStop);
		fprintf(stderr, "%s\n", _vsync_debug->debug_msg);
	}
	_vsync_debug->cur_pos = 0;
	_vsync_debug->debug_msg[0] = '\0';
	_vsync_debug_unlock();
}

static void MAY_NOT_BE_USED
vsync_debug_set_sync(const char * format, ...)
{
	va_list args;

	va_start(args, format);

	_vsync_debug_set_slot(1, format, args);

	va_end(args);
}

static void MAY_NOT_BE_USED
vsync_debug_set_slot(const char * format, ...)
{
	va_list args;

	va_start(args, format);

	_vsync_debug_set_slot(0, format, args);

	va_end(args);
}

static void MAY_NOT_BE_USED
vsync_debug_set_enable(int bEnable)
{
	if (_vsync_debug == NULL)
		_vsync_debug_init();

	if (!_vsync_debug->initialized)
		return;

	_vsync_debug->isPrintEnabled = bEnable;
}

static void MAY_NOT_BE_USED
vsync_debug_master_init(void)
{
	int shmid = _vsync_debug_init();

	_vsync_debug->_shmid = shmid;

	if (sem_init((sem_t *) &_vsync_debug->sem, 1, 1) < 0)
	{
		fprintf(stderr, "\tError : sem_init\n");
                goto bail;
	}

	_vsync_debug->initialized = 1;

	return;
bail:
	fprintf(stderr, "VSYNC_DEBUG.... master Error\n");
}

static void MAY_NOT_BE_USED
vsync_debug_master_close(void)
{
	if (_vsync_debug == NULL)
		return;

	if (shmctl(_vsync_debug->_shmid, IPC_RMID, NULL) < 0)
	{
		fprintf(stderr, "\tError : shmctl(IPC_RMID)\n");
	}
	if (shmdt((void*)_vsync_debug) < 0)
	{
		fprintf(stderr, "\tError : shmdt\n");
	}

	_vsync_debug = NULL;
}

#endif /* _SYNC_DEBUG_H */

