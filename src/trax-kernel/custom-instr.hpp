#pragma once
#include "stdafx.hpp"

#ifndef __riscv
static std::atomic_uint _next_thread;
#endif

#ifdef __riscv
	#define SBI_CALL(which, arg0, arg1, arg2) ({                \
		register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);   \
		register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);   \
		register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);   \
		register uintptr_t a7 asm ("a7") = (uintptr_t)(which);  \
		asm volatile ("ecall"                                   \
				: "+r" (a0)                                     \
				: "r" (a1), "r" (a2), "r" (a7)                  \
				: "memory");                                    \
		a0;                                                     \
	})
#else
	#define SBI_CALL(which, arg0, arg1, arg2)
#endif

#define SBI_CALL_0(which) SBI_CALL(which, 0, 0, 0)
#define SBI_CALL_1(which, arg0) SBI_CALL(which, arg0, 0, 0)
#define SBI_CALL_2(which, arg0, arg1) SBI_CALL(which, arg0, arg1, 0)

void inline putlabel(uint32_t label) {
	SBI_CALL_1(0x0, label);
}

void inline putuint(uint32_t i) {
    SBI_CALL_1(0x1, i);
}

void inline putint(int32_t i) {
    SBI_CALL_1(0x2, i);
}

void inline putfloat(float f) {
    SBI_CALL_1(0x3, f);
}

void inline putuvec2(const rtm::uvec2 &uv2) {
    SBI_CALL_2(0x10, uv2[0], uv2[1]);
}

void inline putvec2(const rtm::vec2 &v2) {
    SBI_CALL_2(0x11, v2[0], v2[1]);
}

void inline putuvec3(const rtm::uvec3 &uv3) {
    SBI_CALL(0x20, uv3[0], uv3[1], uv3[2]);
}

void inline putvec3(const rtm::vec3 &v3) {
    SBI_CALL(0x21, v3[0], v3[1], v3[2]);
}

// uint32_t inline fchthrd()
// {
// #ifdef __riscv
// 	uint32_t value = 0;
// 	asm volatile("fchthrd %0\n\t" : "=r" (value));
// 	return value;
// #else
// 	return _next_thread++;
// #endif
// }

uint32_t inline fchthrd()
{
#ifdef __riscv
	uint32_t value = 0;
	asm volatile(".insn i 0x000b, 0, %0, x0, 0\n\t" : "=r" (value));
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

rtm::vec4 inline sample2d(Texture2D* texture, rtm::vec2 uv)
{
#ifdef __riscv
	uint32_t addr = (uint64_t)texture;
	register float src0 asm("f0") = *(float*)&addr;
	register float src1 asm("f1") = uv.x;
	register float src2 asm("f2") = uv.y;

	register float dst0 asm("f28");
	register float dst1 asm("f29");
	register float dst2 asm("f30");
	register float dst3 asm("f31");

	asm volatile(".insn i 0xb, 0x6, %0, %4, 0\n\t" : "=f" (dst0), "=f" (dst1), "=f" (dst2), "=f" (dst3) : "f" (src0), "f" (src1), "f" (src2));
	return rtm::vec4(dst0, dst1, dst2, dst3);
#else
	return texture->sample(uv);
#endif
}

inline bool sphisect(const rtm::Sphere& sphere, const rtm::Ray &ray, rtm::Hit &hit)
{
#ifdef __riscv
	register float f0  asm("f0")  = ray.o.x;
	register float f1  asm("f1")  = ray.o.y;
	register float f2  asm("f2")  = ray.o.z;
	register float f3  asm("f3")  = ray.t_min;
	register float f4  asm("f4")  = ray.d.x;
	register float f5  asm("f5")  = ray.d.y;
	register float f6  asm("f6")  = ray.d.z;
	register float f7  asm("f7")  = ray.t_max;
	register float f8  asm("f8")  = sphere.center.x;
	register float f9  asm("f9")  = sphere.center.y;
	register float f10 asm("f10") = sphere.center.z;
	register float f11 asm("f11") = sphere.radius;
	register float t   asm("f28");

	asm volatile(
		".insn u 0xb, %0, 0x18\n\t"
		: "=f"(t)
		: "f"(f0), "f"(f1), "f"(f2), "f"(f3), "f"(f4), "f"(f5), "f"(f6), "f"(f7),
		  "f"(f8), "f"(f9), "f"(f10), "f"(f11)
		// : "memory"
	);
	
	hit.t = t;
	return hit.t < ray.t_max;
#else
	return rtm::intersect(sphere, ray, hit);
#endif
}
