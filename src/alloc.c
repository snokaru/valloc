#include "alloc.h"

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

// might not be typical, but here, a word is 64-bit and a dword is 128-bit
// dwords only really matter for alignment requirements
// all sizes are expressed in bytes
#define PAGE_SIZE 4096l
#define WORD_SIZE 8
#define DWORD_SIZE 2 * WORD_SIZE
#define MIN_ALLOC 1

#define ALIGNED(a) ((a) != (((a) >> 4) << 4)) ? (((a) >> 4) + 1) << 4 : (a)

typedef uint64_t word;
typedef uint8_t byte;

static byte* pbrk = NULL;

word block_get_size(word block) {
	return (block >> 4) << 4;
}

int block_is_allocated(word block) {
	return block & 0b1;
}

word* block_get_next(word* block) {
	word size = block_get_size(*block);
	return (word*)((byte*)block + size);
}

word block_create(word size, int is_allocated) {
	word block_header = size;
	block_header |= is_allocated;
	return block_header;
}

int block_is_last(word block) {
	return block_get_size(block) == 0 && block_is_allocated(block) == 1;
}

word* block_get_prev(word* heap_start, word* block) {
	word* prev_block = NULL;
	word* curr_block = heap_start;
	while (curr_block != block) {
		prev_block = curr_block;
		curr_block = block_get_next(curr_block);
	}
	return prev_block;
}

void block_coalesce(word* block) {
	word block_size = block_get_size(*block);

	word* next_block = block_get_next(block);
	if (next_block && !block_is_allocated(*next_block)) {
		block_size += block_get_size(*next_block);
	}

	word* prev_block = block_get_prev((word*)(pbrk + WORD_SIZE), block);
	if (prev_block && !block_is_allocated(*prev_block)) {
		block = prev_block;
		block_size += block_get_size(*prev_block);
	}

	*block = block_create(block_size, 0);
}

void heap_check() {
	word* block = (word*)(pbrk + WORD_SIZE);
	while (!block_is_last(*block)) {
		int is_aligned = (word)(block + 1) == (((word)(block + 1) >> 4) << 4);
		word size = block_get_size(*block);
		int is_allocated = block_is_allocated(*block);
		printf("heapcheck: block %p, size: %ld, allocated: %d, aligned: %d\n", 
			block, size, is_allocated, is_aligned);

		block = block_get_next(block);
	}
}

void heap_initialize() {
	pbrk = sbrk(PAGE_SIZE);
	printf("heainit: heap starts at: %p, ends at: %p, total size: %ld\n", 
		pbrk, pbrk + PAGE_SIZE, PAGE_SIZE);

	// NOTE: two spare words, one to keep alignemnt of actually allocated memory
	// another one will be used as header to mark the end of the free list
	word first_block_size = PAGE_SIZE - 2 * WORD_SIZE;
	printf("heapinit: first free block size: %ld\n", first_block_size);
	word* first_block_addr = (word*)(pbrk + WORD_SIZE);
	*first_block_addr = block_create(first_block_size, 0);

	word* last_block_addr = (word*)(pbrk + PAGE_SIZE - WORD_SIZE);
	printf("heapinit: last free block at: %p\n", last_block_addr);
	*last_block_addr = block_create(0, 1);
}

void heap_extend() {
	void* prev = sbrk(PAGE_SIZE);
	printf("heapextend: heap starts at: %p, ends at: %p, total size: %ld\n", 
		pbrk, prev + PAGE_SIZE, ((byte*)prev + PAGE_SIZE) - pbrk);

	word* block = (word*)(pbrk + WORD_SIZE);
	while (!block_is_last(*block)) {
		block = block_get_next(block);
	}
	*block = block_create(PAGE_SIZE, 0);

	word* last_block = block_get_next(block);
	*last_block = block_create(0, 1);

	block_coalesce(block);
}

void* valloc(size_t size) {
	if (size < MIN_ALLOC) {
		size = MIN_ALLOC;
	}
	int aligned_size = ALIGNED(size + WORD_SIZE); // + WORD_SIZE for header
	printf("valloc: allocating %ld bytes\n", size);

	if (!pbrk) {
		heap_initialize();
		heap_check();
	}

	if (size == 0) {
		heap_check();
		return NULL;
	}

	word* block = (word*)(pbrk + WORD_SIZE);
	while (!block_is_last(*block)) {
		word block_size = block_get_size(*block);
		word* next_block = block_get_next(block);

		if (!block_is_allocated(*block) && aligned_size < block_size) {
			*block = block_create(aligned_size, 1);

			word* next_alloc_block = block_get_next(block);
			if (next_alloc_block != next_block) {
				*next_alloc_block = block_create(block_size - aligned_size, 0);
			}

			heap_check();
			return block + 1;
		}

		block = next_block;
	}

	heap_extend();
	return valloc(size);
};


void vfree(void* addr) {
	word* block = (word*)addr - 1;
	printf("valloc: freeing addr %p (block %p)\n", addr, block);

	block_coalesce(block);
	heap_check();
}
