#include <stdio.h>
#include <unistd.h>



typedef struct {
	void *blkPoint;
	struct Node *next;
	size_t size;
} Node;

static int stacksize = 0;
static int cursize = 0;
static void *curStack;

void *malloct(size_t size) {
	if (cursize >= stacksize)
	{
		curStack = sbrk(65536);
	}		  
}

int main() {
		
	return 0;
}

