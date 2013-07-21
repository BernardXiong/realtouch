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
 * 2009-01-05     Bernard      the first version
 */

/**
 * @addtogroup STM32
 */
/*@{*/

#include <stdio.h>
#include <rtthread.h>
#include <board.h>
#include <components.h>

const struct dfs_mount_tbl mount_table[] = 
{
	{"flash0", "/", "elm", 0, 0},
	{"sd0", "/SD", "elm", 0, 0},
	{RT_NULL, RT_NULL, RT_NULL, 0, 0},
};

void rt_init_thread_entry(void *parameter)
{
    rt_platform_init();

    /* initialization RT-Thread Components */
    rt_components_init();

	codec_hw_init("i2c1");
	/* set device for shell */
	finsh_set_device(RT_CONSOLE_DEVICE_NAME);

	realtouch_ui_init();
}

int rt_application_init(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("init",
                           rt_init_thread_entry, RT_NULL,
                           2048, RT_THREAD_PRIORITY_MAX/3, 20);
    if (tid != RT_NULL)
        rt_thread_startup(tid);

    return 0;
}

/*@}*/

