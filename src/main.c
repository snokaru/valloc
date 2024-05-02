#include <stdio.h>
#include "alloc.h"


int main(void) {
	printf("Welcome to allocator!\n");
	void* addr1 = valloc(100);
	void* addr2 = valloc(300);
	void* addr3 = valloc(500);

	vfree(addr1);
	vfree(addr2);
	vfree(addr3);

	valloc(50);
	valloc(250);
	valloc(700);
	valloc(3000);
	valloc(50);
	valloc(10000);
}

