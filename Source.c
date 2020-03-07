#include <stdlib.h>

#define BNDR 4
#define BNDR_SIZE 8
#define MIN_BLOCK 12

typedef struct block {
	unsigned int size;
	unsigned int next;
	unsigned int previous;
}BLOCK;

typedef struct boundary {
	unsigned int size;
}BOUNDARY;

void* start;

void* memory_alloc(unsigned int size);

int memory_free(void* valid_ptr);

int memory_check(void* ptr);

void memory_init(void* ptr, unsigned int size);

void* split(BLOCK*, unsigned int size);

void* best_fit(unsigned int size);

void new_start(BLOCK*);

void connector(BLOCK*);

void new_end(BLOCK*);

void* coalescing(BLOCK*, BLOCK*);


int main() {
	char region[64];
	memory_init(region, 64);
	char* pointer = (char*)memory_alloc(8);
	if (pointer)
		memset(pointer, '1', 8);
	char* pointer1 = (char*)memory_alloc(12);
	if (pointer1)
		memset(pointer1, '1', 12);
	memory_free(pointer);
	return 0;
}

void memory_init(void* ptr, unsigned int size) {
	start = ptr;
	*(unsigned int*)ptr = BNDR_SIZE;
	BOUNDARY* header = (BOUNDARY*)((int)ptr + BNDR);
	header->size = size;
	BLOCK* block = ((BLOCK*)((int)ptr + BNDR_SIZE));
	block->size = size - BNDR_SIZE;
	block->next = 0;
	block->previous = 0;
	BOUNDARY* footer = (BOUNDARY*)((char*)ptr + size - BNDR);
	footer->size = block->size;
}

void* best_fit(unsigned int size) {
	void* temporary = NULL;
	for (BLOCK* free_block = (BLOCK*)((char*)start + *(int*)start); free_block != 0; ) {
		if (free_block->size == size) {
			connector(free_block);
			return free_block;
		}
		else if (free_block->size > size)
			if (free_block->size < temporary || temporary == NULL)
				temporary = free_block;
		if (free_block->next == 0)
			free_block = NULL;
		else free_block = free_block + (free_block->next);
	}
	return split(temporary, size);
}

void connector(BLOCK* block) {
	(block - block->previous)->next = block->previous + block->next;
	if (block->previous == 0) {
		new_start(block);
		(block + block->next)->previous = 0;
	}
	else {
		(block + block->next)->previous = block->next + block->previous;
		if (block->next == 0)
			new_end(block);
	}

}

void new_start(BLOCK* block) {
	if (block->next == 0)
		*(unsigned int*)((int)start) += block->size + BNDR_SIZE;
	else
		*(unsigned int*)((int)start) += block->next;
}

void new_end(BLOCK* block) {
	(block - (block->previous))->next = 0;
}

void* split(BLOCK* block, unsigned int size) {
	if (block->size - BNDR_SIZE - size < MIN_BLOCK)
		return block;
	BLOCK* new_block = (int)block + size + BNDR_SIZE;
	new_block->size = block->size - size - BNDR_SIZE;
	if (block->next != 0)
		new_block->next = block->next - block->size;
	else
		new_block->next = 0;
	((BOUNDARY*)((int)new_block + new_block->size - BNDR))->size = new_block->size;
	block->size = size;
	((BOUNDARY*)((int)block + BNDR + block->size))->size = size;
	if (block->previous == 0) {
		new_start(block);
		new_block->previous = 0;
	}
	else {
		new_block->previous = block->previous + block->size;
		if (block->next == 0)
			new_end(block);
	}
	return (int)block+BNDR;
}

void* memory_alloc(unsigned int size) {
	return best_fit(size);
}

int memory_free(void* valid_ptr) {
	int k;
	if (valid_ptr < ((char*)start + *(int*)start)) {
		k = (char*)start - (char*)valid_ptr;
	}
	for (BLOCK* free_block = (BLOCK*)((char*)start + *(int*)start); free_block != 0; ) {
		if (free_block < valid_ptr) {
			coalescing((BLOCK*)valid_ptr, free_block);
		}
		if (free_block->next == 0)
			free_block = NULL;
		else free_block = free_block + (free_block->next);
	}

}

void* coalescing(BLOCK* block, BLOCK* free_block) {
	if ((block + block->size+BNDR) == free_block) {
		free_block->size += block->size + BNDR_SIZE;
		((BOUNDARY*)(block + block->size))->size = free_block->size;
		if ((free_block + free_block->next) == (block + block->size + BNDR)) {
			free_block->size +=(free_block + free_block->next)->size;
			free_block->next = free_block->next + (free_block + free_block->next)->next;
			((BOUNDARY*)(free_block + free_block->size))->size = free_block->size;
		}
		return free_block;
	}
	if ((free_block + free_block->next) == (block + block->size + BNDR)) {
		block->size += (free_block + free_block->next)->size;
		block->next = block->size + (block+block->size)->next;
		((BOUNDARY*)(block + block->size))->size = block->size;
	}
	return block;
}
