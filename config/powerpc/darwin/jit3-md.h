
#ifndef __powerpc_darwin_jit3_md_h
#define __powerpc_darwin_jit3_md_h

#include "config.h"
#include "powerpc/jit.h"

/*
 * Flush newly generated instructions from the data cache and invalidate the
 * same blocks in the instruction cache.
 *
 * See section 5.2.5.2 of the PowerPC Programming Environments manual and
 * numerous usenet discussions on google.
 *
 * code_start - The start of the code, no special alignment required.
 * code_end - The end of the code.
 */
#define FLUSH_DCACHE darwin_flush_cache
static inline void darwin_flush_cache(void *code_start, void *code_end)
{
	/*
	 * The size of a block in the cache, we need to align the flushed
	 * addresses to these blocks.
	 */
	static int cache_block_size = 32; // bytes
	uintp curr, last;
	
	curr = (uintp)code_start;
	last = (uintp)code_end;
	curr = (curr - cache_block_size) & ~(cache_block_size - 1);
	last = (last + cache_block_size) & ~(cache_block_size - 1);
	for( ; curr < last; curr += cache_block_size )
	{
		asm("dcbst 0, %0\n"
		    "sync\n"
		    "icbi 0, %0\n"
		    "sync\n"
		    "isync\n" : : "r" (curr));
	}
}

#endif
