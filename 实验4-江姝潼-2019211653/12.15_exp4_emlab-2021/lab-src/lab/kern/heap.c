/*
 * heap.c
 * Based on Memory allocator by Kernighan and Ritchie,
 * The C programming Language, 2nd ed.  Section 8.7.
 */

#include <stddef.h>
#include "kern_api.h"
#include "heap.h"


typedef long Align;

union header {
	struct {
		union header *ptr;
		unsigned int size;
	} s;
	Align x;
};

typedef union header Header;

static unsigned char heap_mem[CONFIG_HEAP_SIZE];
static unsigned char *break_address = heap_mem;

static Header base; /* empty list to get started */
static Header *freep = NULL; /* start of kfree list */

static DEFINE_MUTEX(heap_lock);
#define HEAP_LOCK() mutex_take(&heap_lock)
#define HEAP_UNLOCK() mutex_give(&heap_lock)

static void *sbrk(unsigned int nbytes)
{
	if (break_address + nbytes >= heap_mem
	    && break_address + nbytes < heap_mem + CONFIG_HEAP_SIZE) {
		unsigned char *previous_pb = break_address;
		break_address += nbytes;
		return (void *) previous_pb;
	}
	return (void *) -1;
}

void *kmalloc(unsigned int nbytes)
{
	Header *p, *prevp;
	unsigned int nunits;
	void *cp;

	nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;

	HEAP_LOCK();

	if ((prevp = freep) == NULL) {
		base.s.ptr = freep = prevp = &base;
		base.s.size = 0;
	}

	for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {
		if (p->s.size >= nunits) {
			if (p->s.size == nunits) {
				prevp->s.ptr = p->s.ptr;
			} else {
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}
			freep = prevp;

			HEAP_UNLOCK();
			return (void *)(p + 1);
		}

		if (p == freep) {
			cp = sbrk(nunits * sizeof(Header));
			if (cp == (void *) -1) {
				HEAP_UNLOCK();
				return NULL;
			} else {
				p = (Header *) cp;
				p->s.size = nunits;
				kfree((void *) (p + 1));
				p = freep;
			}
		}
	}
}

void kfree(void *ap)
{
	Header *bp, *p;
	bp = (Header *) ap - 1;

	HEAP_LOCK();

	for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr) {
		if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
			break;
	}

	if (bp + bp->s.size == p->s.ptr) {
		bp->s.size += p->s.ptr->s.size;
		bp->s.ptr = p->s.ptr->s.ptr;
	} else {
		bp->s.ptr = p->s.ptr;
	}

	if (p + p->s.size == bp) {
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	} else {
		p->s.ptr = bp;
	}

	freep = p;

	HEAP_UNLOCK();
}


void heap_init(void) {
	mutex_init(&heap_lock);
}
