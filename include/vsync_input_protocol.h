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

#ifndef _SYNC_INPUT_LIB_H
#define _SYNC_INPUT_LIB_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#define SYNC_INPUT_SHM_KEY	0xDEAD2010

typedef struct {
	/* EVDEVMULTITOUCHh providing functions FBDEV will use */
	void (*sync)(int vsync_cnt);

	/* FBDEV providing functions EVDEVMULTITOUCHh will use */
	void * sync_arg;
	void (*start_sync)(void * sync_arg);
	void (*stop_sync)(void * sync_arg);

	/* for internal use only */
	int _shmid;

	int _reserved[16];
} VSYNC_INPUT_DISPATCH_TABLE;

static volatile VSYNC_INPUT_DISPATCH_TABLE *
vsync_input_init(void)
{
	VSYNC_INPUT_DISPATCH_TABLE * table = NULL;
	int shmid = 0;

	struct shmid_ds ds;
	
	shmid = shmget(SYNC_INPUT_SHM_KEY, sizeof(VSYNC_INPUT_DISPATCH_TABLE), IPC_CREAT);
	if (shmid < 0)
		goto bail;

	if (shmctl(shmid, IPC_STAT, &ds) < 0)
		goto bail;

	if (ds.shm_segsz < sizeof(VSYNC_INPUT_DISPATCH_TABLE))
		goto bail;

	table = shmat(shmid, NULL, 0);
	if (table == (void *) -1)
		goto bail;

	table->_shmid = shmid;

	return table;

bail:
	if (shmid >= 0)
		shmctl(shmid, IPC_RMID, NULL);

	if (table != NULL && table != (void *) -1)
		shmdt(table);

	return NULL;
}

static int
#ifdef __GNUC__
__attribute__ ((unused))
#endif
vsync_input_close(volatile VSYNC_INPUT_DISPATCH_TABLE * table)
{
	if (table == NULL)
		return -1;

	shmdt((void *) table);

	return 0;
}

static int
#ifdef __GNUC__
__attribute__ ((unused))
#endif
vsync_input_destroy(volatile VSYNC_INPUT_DISPATCH_TABLE * table)
{
	if (table == NULL)
		return -1;

	shmctl(table->_shmid, IPC_RMID, NULL);
	shmdt((void *) table);

	return 0;
}

#endif /* _SYNC_INPUT_LIB_H */
