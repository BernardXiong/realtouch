/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-08-08     Bernard      the first version
 */
#include <dfs.h>
#include <dfs_fs.h>

const struct dfs_mount_tbl mount_table[] =
{
	{"flash0", "/", "elm", 0, 0},
	{"sd0", "/SD", "elm", 0, 0},
	{RT_NULL, RT_NULL, RT_NULL, 0, 0},
};

