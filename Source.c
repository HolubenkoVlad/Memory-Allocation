#include <stdlib.h>

#define BNDR 4
#define BNDR_SIZE 8
#define MIN_BLOCK 12
#define RESULT(block) (int)block+BNDR

typedef struct block {
	unsigned int size;
	unsigned int next;
	unsigned int previous;
}BLOCK;

typedef struct header {
	unsigned int size;
}HEADER;

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

int coalescing(BLOCK*, BLOCK*);

void coalescing_blocks(BLOCK*, BLOCK*);


int main() {
	char region[128];
	memory_init(region, 128);
	char* pointer = (char*)memory_alloc(8);
	if (pointer)
		memset(pointer, 0, 8);
	char* pointer1 = (char*)memory_alloc(12);
	if (pointer1)
		memset(pointer1, 0, 12);
	char* pointer2 = (char*)memory_alloc(8);
	if (pointer2)
		memset(pointer2, 0, 8);
	char* pointer3 = (char*)memory_alloc(12);
	if (pointer3)
		memset(pointer3, 0, 12);
	memory_free(pointer);
	memory_free(pointer2);
	//memory_free(pointer1);
	char* pointer4 = (char*)memory_alloc(20);
	if (pointer4)
		memset(pointer4, 0, 20);
	memory_free(pointer3);
	char* pointer6 = (char*)memory_alloc(24);
	if (pointer6)
		memset(pointer6, 0, 24);
	/*char* pointer5 = (char*)memory_alloc(36);
	if (pointer5)
		memset(pointer5, 0, 36);*/
//	memory_free(pointer4);
	return 0;
}

void memory_init(void* ptr, unsigned int size) {
	start = ptr;
	*(unsigned int*)ptr = BNDR_SIZE;
	HEADER* header = (HEADER*)((int)ptr + BNDR);
	header->size = size;
	BLOCK* block = ((BLOCK*)((int)ptr + BNDR_SIZE));
	block->size = size - BNDR_SIZE-BNDR;
	block->next = 0;
	block->previous = 0;
}

void* best_fit(unsigned int size) {
	void* temporary = NULL;
	for (BLOCK* free_block = (BLOCK*)((char*)start + *(int*)start); free_block != 0; ) {
		if (free_block->size == size) {
			connector(free_block);
			return RESULT(free_block);
		}
		else if (free_block->size > size)
			if (temporary == NULL|| free_block->size < ((BLOCK*)temporary)->size)
					temporary = free_block;
		if (free_block->next == 0)
			free_block = NULL;
		else free_block = (char*)free_block + (free_block->next);
	}
	return split(temporary, size);
}

void connector(BLOCK* block) {
	((BLOCK*)((char*)block - block->previous))->next = block->previous + block->next;
	if (block->previous == 0) {
		new_start(block);
		((BLOCK*)((char*)block + block->next))->previous = 0;
	}
	else {
		((BLOCK*)((char*)block + block->next))->previous = block->next + block->previous;
		if (block->next == 0)
			new_end(block);
	}
}

void new_start(BLOCK* block) {
//	if (block->next == 0)
		*(unsigned int*)(start) = (char*)block - (char*)start;
	//else
	//	*(unsigned int*)((int)start) += block->next;
}

void new_end(BLOCK* block) {
	((BLOCK*)((int)block - (block->previous)))->next = 0;
}

void* split(BLOCK* block, unsigned int size) {
	if (block->size - BNDR - size < MIN_BLOCK) {
		connector(block);
		return RESULT(block);
	}
	BLOCK* new_block = (int)block + size + BNDR;
	new_block->size = block->size - size - BNDR;
	if (block->next != 0) {
		new_block->next = block->next - size - BNDR;
		((BLOCK*)((char*)new_block + new_block->next))->previous = new_block->next;
	}
	else
		new_block->next = 0;
	block->size = size;
	if (block->previous == 0) {
		new_start(new_block);
		new_block->previous = 0;
	}
	else {
		new_block->previous = block->previous + block->size+BNDR;
		((BLOCK*)((char*)new_block - new_block->previous))->next = new_block->previous;
		/*if (block->next == 0)
			new_end(block);*/
	}
	return RESULT(block);
}

void* memory_alloc(unsigned int size) {
	return best_fit(size);
}

int memory_free(void* valid_ptr) {
	(char*)valid_ptr -= BNDR;
	BLOCK* temp = NULL;
	int numberTemp;
	if (valid_ptr < ((char*)start + *(int*)start)) {
		numberTemp = ((char*)start + *(int*)start) - (char*)valid_ptr;
		*(unsigned int*)start = (char*)valid_ptr - (char*)start;
		temp = ((BLOCK*)((char*)valid_ptr + numberTemp));
		if (coalescing(temp, (BLOCK*)valid_ptr) == 1) {
			((BLOCK*)valid_ptr)->next = numberTemp;
			((BLOCK*)valid_ptr)->previous = 0;
			temp->previous = (char*)temp - (char*)valid_ptr;
		}
	}
	else {
		for (BLOCK* free_block = (BLOCK*)((char*)start + *(int*)start); free_block != 0; ) {
			if (free_block < valid_ptr && valid_ptr<(BLOCK*)((int)free_block+free_block->next)) { 
				if (coalescing((BLOCK*)valid_ptr, free_block)==1)
					coalescing_blocks((BLOCK*)valid_ptr, free_block);
				break;
			}
			if (free_block->next == 0)
				free_block = NULL;
			else 
				free_block = (BLOCK*)((int)free_block + (free_block->next));
		}
	}
}

void coalescing_blocks(BLOCK* block, BLOCK* free_block) {
	block->previous = (char*)block - (char*)free_block;
	block->next = (char*)((int)free_block + free_block->next)-(char*)block;
	((BLOCK*)((int)free_block + free_block->next))->previous = (char*)((int)free_block + free_block->next) - (char*)block;
	free_block->next = block->previous;
}
    
int coalescing(BLOCK* block, BLOCK* free_block) {
	BLOCK* temp = NULL;
	if ((BLOCK*)((int)free_block + free_block->size+BNDR) == block) {
		free_block->size += block->size + BNDR;
		if ((BLOCK*)((int)free_block + free_block->next) == (BLOCK*)((int)block + block->size + BNDR)) {
			temp = (BLOCK*)((int)free_block + free_block->next);
			free_block->size += temp->size + BNDR;
			if (temp->next == 0)
				free_block->next = 0;
			else {
				free_block->next = free_block->next + temp->next;
				((BLOCK*)((int)free_block + free_block->next))->previous = free_block->next;
			}
		}
		return 0;
	}
	else if ((BLOCK*)((int)block + free_block->size + BNDR)== (BLOCK*)((int)free_block + free_block->next)) {
		block->next = block->size + ((BLOCK*)((int)block + block->size))->next;
		block->previous = ((BLOCK*)((int)block + block->size))->next - block->size;
		block->size += ((BLOCK*)((int)free_block + free_block->next))->size + BNDR;
		((BLOCK*)((char*)block - block->previous))->next = block->previous;
		((BLOCK*)((char*)block + block->next))->previous = block->next;
		return 0;
	}
	return 1;
}
