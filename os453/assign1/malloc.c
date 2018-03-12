/*header:
@author: Aubrey Russell
@program: os 453 Assignment 1
@Submit date: 1/16/2016
The goal of this lab is to rewrite the c stdlib library ourselves
in order to both get familiar with several unix system calls
and resource allocation. Resource allocation and management will
most likely be a very important topic in os 453 and so its
pertinent to be able to understand the stdlib concepts to gain
a better understanding of how the operating system works on
a fundamental level. 
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#define STACK_INC 65536 /*64k increment for sbrk*/
#define BOUNDARY 16 /*round up boundary so everything is divisible by 16*/
#define NODE_SIZE_ROUNDED 32 /*upper limit for Node 
type so that its divisible by 16*/
#define DBG_MAL "DEBUG_MALLOC" /*debug malloc string*/

typedef struct Node { /*Node structure for linked list of data blocks*/
	void *blkPoint;
	struct Node *next;
	char isFreed;
	int size;
} Node;

static int stacksize = 0; /*reresents the current stack size increment*/
static int cursize = 0; /*current amount of data used 
out of the calls made to sbrk*/
static char *curStack = NULL; /*convenient character pointer for */
static Node *head = NULL;
static Node *curpos = NULL;


/*this function is used to round up a size to a specified
 boundary in order to guaruntee each block of memory starts
 at a location divisible by 16*/
int roundUp(size_t size) {
	return (((int)(size / BOUNDARY)) + 1) * BOUNDARY; 
}

/*do some initialization stuff and increment the stack if needed*/
int initSbrk(int newSize) {

	if (newSize > STACK_INC) /*case for requested allocation 
	value larger than the standard 64k increment*/
	{
		if ((intptr_t)(curStack = sbrk(roundUp(newSize))) == -1)
		{ /*err checking for sbrk*/
			return 0;
		}
		;
		stacksize += roundUp(newSize);
	}
	else if (BOUNDARY * (cursize + newSize) >= stacksize) /*case 
	where a new increment would exceed the current stack size*/
	{
		if (curStack == NULL) /*make sure to initialize the stack*/
		{
			if ((intptr_t)(curStack = sbrk(STACK_INC)) == -1)
			{ /*err checking for sbrk*/
				return 0;
			}
		}

		else
		{
			sbrk(STACK_INC);
		}
		stacksize += STACK_INC;
	}
	return 1;

}

/*if the block is not null, free the block by simply changing a flag*/
void free(void *rmBlk) {
	if(getenv(DBG_MAL))
	{
		fprintf(stderr, "MALLOC: free(%p)\n", rmBlk);
	}

	if (rmBlk)
	{
		Node *freeBlock = (Node *)(((char *)rmBlk) - 
		 NODE_SIZE_ROUNDED); /*the  header comes 
		before the block of memory so increment 
		backwards by the node size to get to the 
		header block*/
		freeBlock->isFreed = 1;
	}

}

/*this function deals with fragmentation by iterating through 
the current stack from the beginning and either places new
data at freed locations with enough size or by combining
adjacent cells that have enough memory*/
Node *checkAvaliable(size_t size, Node *blkHead) {
	Node *memListIter = blkHead, *tmpmemListIter;
	int tempSize = 0;
				
	while (memListIter->next) 
	{
		if (size <= memListIter->size && memListIter->isFreed) /*
		if size is sufficient and its free then just return 
		the block*/
		{
			return memListIter;
		}
		if (size <= memListIter->size && memListIter->
		 isFreed && memListIter->next->isFreed)
		{ /*condition to see if there are adjacent blocks*/

			tempSize = memListIter->size;
			tmpmemListIter = memListIter;
			while (tmpmemListIter && size > tempSize)
			{ /*loop through the adjacent cells 
			to combine n freed cells*/
				tmpmemListIter = tmpmemListIter->next;
				tempSize += NODE_SIZE_ROUNDED 
				 + tmpmemListIter->size;
			}

			if (size <= tempSize) /*make sure adjacent 
			cells have enough size*/
			{
				memListIter->size = tempSize;
				memListIter->next = tmpmemListIter;
				return memListIter;			
			}

		}
		memListIter = memListIter->next;
	}
	return NULL;
}

/*allocates a block of data based on the provided stack and can also
initialize the stack if need be*/
void *malloc(size_t size) {

	void *blkRet;
	Node *tempNode;

	size = roundUp(size + NODE_SIZE_ROUNDED);
	if (!initSbrk(size))
	{
		errno = ENOMEM;
		return NULL;
	}

	if (!curpos || !head)
	{
		head = curpos = (Node *)curStack;
	}
	else if ((tempNode = checkAvaliable(size, head))) /*look 
	for adjacent or avaliable blocks*/
	{
		tempNode->isFreed = 0;
		return tempNode->blkPoint;
	}
	/*this next part of code moves the current position 
	of the stack and sets up a new block of memory*/
	curpos->size = size;
	curpos->blkPoint = curStack + NODE_SIZE_ROUNDED;
	blkRet = curpos->blkPoint;
	cursize += size + NODE_SIZE_ROUNDED;
	curStack += size + NODE_SIZE_ROUNDED;

	curpos->isFreed = 0;
	curpos->next = (Node *)curStack;
	tempNode = curpos;
	curpos = curpos->next;
	curpos->next = NULL;
	if(getenv(DBG_MAL))
	{
		fprintf(stderr, "MALLOC: malloc(%zu) => (ptr=%p, size=%zu)\n", 
		 size, blkRet, size);
	}
	return blkRet;

}

/*similar to malloc except takes in number of elements
 and the size and then sets the entire block of memory to zeroes*/
void *calloc(size_t nitems, size_t size) {

	char *tempBlk = malloc(nitems * size);
	memset(tempBlk, 0, nitems * size); /*set the memory to zero 
	based on the size of the block*/
	if(getenv(DBG_MAL))
	{
		fprintf(stderr, 
		"MALLOC: calloc(%zu,%zu) => (ptr=%p, size=%zu)\n", 
		 nitems, size, tempBlk, nitems * size);
	}
	return (void *)tempBlk;
}

/*realloc attempts to resize a function by expansion and if it can't 
it copies memory from the old block and moves it to the top of the
stack with the new size*/
void *realloc(void *blkPoint, size_t size) {
	Node *adjustBlock = (Node *)(((char *)blkPoint) - NODE_SIZE_ROUNDED); /*
	this gets the head of a block of memory for information and flags*/
	Node *tempNode;
	
	if(getenv(DBG_MAL))
	{
		fprintf(stderr, 
		"MALLOC: realloc(%p,%zu) => (ptr=%p, size=%zu)\n"
		 , blkPoint, size, adjustBlock, size);
	}

	if (blkPoint == NULL) /*same thing as malloc when blkPoint is null*/
	{
		return malloc(size);
	}
	size = roundUp(size + NODE_SIZE_ROUNDED);

	if (!initSbrk(size))
	{
		errno = ENOMEM;
		return NULL;
	}

	if (!curpos || !head)
	{
		head = curpos = (Node *)curStack;
		return curpos;
	}
	else if (size == adjustBlock->size) /*this 
	checks and returns if the size is identical and would do nothing*/
	{
		return adjustBlock->blkPoint;
	}

	else if (size + NODE_SIZE_ROUNDED < adjustBlock->size)
	{ /*handles the contraction case*/
		tempNode = adjustBlock->next;
		adjustBlock->next = (Node *)((char *)adjustBlock + size); /*
		create a new Node that can be used for the block you've 
		just contracted*/
		adjustBlock->next->next = tempNode; /*fix the linked list*/
		adjustBlock->next->size = 
		 adjustBlock->size - (NODE_SIZE_ROUNDED + size); /*calculate 
		the new size for the new block*/

		adjustBlock->next->blkPoint = 
		 (((char *)adjustBlock->next) + NODE_SIZE_ROUNDED); /*set 
		 the blk pointer to the correct memory block*/
		adjustBlock->size = size;
		return adjustBlock->blkPoint;
	}
	else if ((tempNode = checkAvaliable(size, adjustBlock))) /*deal 
	with fragmentation*/
	{
		tempNode->isFreed = 0;
		return tempNode->blkPoint;
	}

	else /*case where memory needs to be moved to the 
	top of the stack/no room for exapnsion*/
	{

		tempNode = (Node *)curStack;
		tempNode->size = size;
		tempNode->blkPoint = (curStack + NODE_SIZE_ROUNDED);
		tempNode->next = NULL;
		memcpy(tempNode->blkPoint, blkPoint, (adjustBlock->
		 size <= size ? size : adjustBlock->size)); /*copy memory 
		 from old block and write it to the new block at the top 
		 of the stack based on smaller size*/
		
		curpos = tempNode;
		cursize += size + NODE_SIZE_ROUNDED;
		curStack += size + NODE_SIZE_ROUNDED;
		free(blkPoint);
		return tempNode->blkPoint;
	}

}