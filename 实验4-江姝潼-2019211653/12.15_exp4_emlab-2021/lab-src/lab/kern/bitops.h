/*
 * bitops.h
 *
 *      Author: ljp
 */

#ifndef BITOPS_H_
#define BITOPS_H_


/* t=target variable, n=bit number to act upon 0..n */
#define BIT_SET(t,n) ((t) |= (1UL<<(n)))
#define BIT_CLEAR(t,n) ((t) &= ~(1UL<<(n)))
#define BIT_FLIP(t,n) ((t) ^= (1UL<<(n)))
#define BIT_CHECK(t,n) ((t) & (1UL<<(n)))
/* t=target variable, m=mask */
#define BITMASK_SET(t,m) ((t) |= (m))
#define BITMASK_CLEAR(t,m) ((t) &= (~(m)))
#define BITMASK_FLIP(t,m) ((t) ^= (m))
#define BITMASK_CHECK_ALL(t,m) (((t) & (m)) == (m)) // warning: evaluates m twice #define BITMASK_CHECK_ANY(t,m) ((t) & (m))
#define MODIFY_REG(reg, clearmask, val) ((reg) = (((reg) & (~(clearmask))) | val)))

#define BIT_MASK_FROM_TO(n1, n2) (((unsigned) -1 >> (31 - (n2))) & ~((1U << (n1)) - 1))
/*lv[n1..n2] = val*/
#define BIT_RANGE_ASSIGN(reg, val, n1, n2) ((reg) = ((reg) & ~(BIT_MASK_FROM_TO((n1),(n2)))) | ((((val) << (n1)) & (BIT_MASK_FROM_TO((n1),(n2))))))

#define BIT(nr)			(1UL << (nr))


/**
 * fls32 - find last (most-significant) bit set in the word x
 *
 * fls32(0) = 0, fls32(1) = 1, fls32(0x80000000) = 32.
 */
static inline int fls32(int x)
{
	int r = 32;

	if (!x) {
		return 0;
	}
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

#endif /* BITOPS_H_ */
