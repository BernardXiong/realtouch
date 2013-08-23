#include <rtthread.h>

#ifdef RT_USING_MEMHEAP
static struct rt_memheap stm32f4xx_cmm;

int stm32f4xx_ccm_init(void)
{
	return rt_memheap_init(&stm32f4xx_cmm, "ccm", 
		(void*)0x10000000, 64 * 1024);
}
INIT_BOARD_EXPORT(stm32f4xx_ccm_init);

void *ccm_malloc(int size)
{
	return rt_memheap_alloc(&stm32f4xx_cmm, size);
}

void ccm_free(void *rmem)
{
	rt_memheap_free(rmem);
}

void *ccm_realloc(void *rmem, int newsize)
{
	return rt_memheap_realloc(&stm32f4xx_cmm, rmem, newsize);
}

#endif

