#pragma once
#include "stdafx.hpp"

#ifndef __riscv
static std::atomic_uint _next_thread;
#endif

uint32_t inline fchthrd()
{
#ifdef __riscv
	uint32_t value = 0;
	// asm volatile("fchthrd %0\n\t" : "=r" (value));
	asm volatile(".insn i 0x0000b, 0, %0, x0, 0\n\t" : "=r" (value));
	
	return value;
#else
	return _next_thread++;
#endif
}

#ifndef __riscv
void reset_fchthrd()
{
 	_next_thread = 0;
}
#endif

inline void ebreak()
{
	#ifdef __riscv
	asm volatile
	(
		"ebreak\n\t"
	);
	#endif
}
