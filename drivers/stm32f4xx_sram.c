#include <rtthread.h>

#ifdef RT_USING_MEMHEAP
#include <board.h>

#ifdef __CC_ARM
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define STM32_SRAM_BEGIN    (&Image$$RW_IRAM1$$ZI$$Limit)
#elif __ICCARM__
#pragma section="HEAP"
#define STM32_SRAM_BEGIN    (__segment_end("HEAP"))
#else
extern int __bss_end;
#define STM32_SRAM_BEGIN    (&__bss_end)
#endif

static struct rt_memheap stm32f4xx_sram;

int stm32f4xx_sram_init(void)
{
	return rt_memheap_init(&stm32f4xx_sram, "sram", 
		(void*)STM32_SRAM_BEGIN, STM32_SRAM_END - (rt_uint32_t)STM32_SRAM_BEGIN);
}
INIT_BOARD_EXPORT(stm32f4xx_sram_init);

void *sram_malloc(rt_size_t size)
{
	return rt_memheap_alloc(&stm32f4xx_sram, size);
}

void sram_free(void *rmem)
{
	rt_memheap_free(rmem);
}

void *sram_realloc(void *rmem, rt_size_t newsize)
{
	return rt_memheap_realloc(&stm32f4xx_sram, rmem, newsize);
}

#endif

