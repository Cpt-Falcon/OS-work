/*header:
@author: Aubrey Russell, Chris Voncina
@program: os 453 Assignment 2
@Submit date: 1/1/2016
The goal of this lab is to teach us about threads with 
respect to lightweight processes and schedulers. The 
lightweight process program allow the user to create 
multiple threads that the lwp program selects to run
based on a provided function by swaping register files.
There are several useful utility functions to work 
with some lwp threads such as yield, which allows 
you to jump to the next process and finish the previous 
one later, and stop, which allows you to pause, return 
to where you called lwp start, and then resume where 
lwp left off bycalling stop. There is also set scheduler 
which enables you to select a new schedular and copy
all the new threads over to it. This program primarily
teaches us about the fundamental underpinnings of 
of how the operating system manages multiple threads
and how it is able to utilize schedulers to select
processes; this also initializes us (pun intended)
for assignment 3 where we implement a scheduler
because it shows us how the scheduler interacts
with threads.
*/

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <signal.h>
#include <sys/time.h>
#include "lwp.h"
#define NO_THREAD 0

static int size = 0;
struct s_list *headSched = NULL; /*top of linked list*/
struct s_list *currPosched = NULL;
struct s_list *tail = NULL; /*end node of linked list 
for ease of appending a node*/

typedef struct s_list /*for scheduler linked list*/
{
   thread data;
   struct s_list *next;
} s_list;

/*We created a custom struct here to make it easier for us to 
create a circular linked list. Th*/
typedef struct ContextNode
{
   int isPaused;
   context cntx;
   struct ContextNode *next;
} ContextNode;


static tid_t curId = 0;
static int isPaused = 0;
static context mainCntx;
static struct ContextNode *head = NULL;
static struct ContextNode *curPos = NULL;
static thread curThread = NULL;

/*This function initializes the circular linked list 
 * structure of type s_list.  */
void rr_init() 
{
   currPosched = malloc(sizeof(s_list));
   headSched = currPosched; /* initialized head to be beginning */
   currPosched->next = headSched; /* make circular linked */
}
/* this Function free's up all unused pointers 
 * */
void rr_shutdown()
{
   s_list *nextVal;
   while(currPosched != NULL) 
   {
      nextVal = currPosched->next;
      free(currPosched);
      currPosched = nextVal;
   }
}
 /* This function implements the function pointer in scheduler.
    The purpose of rr_admit is to initialize struct if hadn't been 
    also adds thread to the linked list.*/
void rr_admit (thread newT) 
{
   size++;
   if (!tail)
   {
      currPosched = headSched = tail = malloc(sizeof(s_list));
      /* create tail set it to the head of list */
   }
   else
   {
      tail->next = malloc(sizeof(s_list));
      tail = tail->next; /*move tail end of list */
   }
   tail->data = newT;
   tail->next = headSched;
}
  /* This function implements the function pointer of scheduler.
   * removes the  specific thread in the linked list that is passed
   * into the function.
   */
void rr_remove(thread victim)
{
   s_list *tempThread;
   s_list *prev;
   tempThread = headSched;
   if (size == 1) /*if 1 thread in list reset  to null instead head*/
   {
      currPosched->next = NULL; 
      return;
   }

   size--;
   while ( tempThread->next != headSched ) /*while not at head of list */
   {
      /*look for the thread to remove */
      if (tempThread->data && headSched->data->tid == victim->tid)
      {
         /*if thread is at head change index head and tail*/
         tail->next = headSched->next;
          /*when removed change tail and head to new index*/
         headSched = tail->next;
         return;
      }
      if (tempThread->data && tempThread->data->tid == victim->tid) 
      {
         /*check if tempThread isn't null for error check*/
       
         tempThread->next = tempThread->next->next; 
         break;
      }
      prev = tempThread;
      tempThread = tempThread->next;
      if (tempThread->data && tempThread->data->tid == 
       victim->tid && tempThread->next == headSched) 
       {
         /* end of linked list has the thread */
         prev->next = headSched;
         currPosched = headSched; 
      }
   }

}

/*This function is used traverse the linked list.
 and the returns the previous stored thread
 */

thread rr_next()
{
   thread threadToReturn;
   if(currPosched->next != NULL)
   {
      threadToReturn = currPosched->data; /*store the thread*/
      currPosched = currPosched->next; /*increment the pointer */
      return threadToReturn; /*return the stored thread */
   }
   else
      return NULL;
}
static struct scheduler rr_publish = 
{NULL, NULL, rr_admit, rr_remove, rr_next}; /*we had
to initialize our struct here because it depends on
functions above*/
scheduler theScheduler = &rr_publish;

/*This function is used as a return point for
the returning lwp threads; it either stops lwp
and returns to where lwp_start was called, or
it loads the next state of the program into the registers*/
void lwp_exit()
{
   theScheduler->remove(curThread);
   curThread = theScheduler->next(); /*increment to the current 
   thread to the next value thread*/

   if (curThread == NULL) /*check if no next thread*/
   {
      head = curPos = NULL;

      lwp_stop(); /*end lwp if no next thread*/
   }
   curPos->isPaused = 0;
   load_context(&(curThread->state)); /* load  
    data from reg file and write to registers*/
   return;
}

/*This function creates an lwp thread based on a stack size
and a provided function. It creates the stack which enables you 
to eventually swap rfiles and return to the desired function; It
 initializes the register file register base pointer and
stack pointer so that a return will pop it at some point. 
Create also initializes variables and the scheduler if its the 
first call and sets up a linked list of */
tid_t lwp_create(lwpfun data, void *argument, size_t stackSize) {
   tid_t tmptid;

   if (!head)
   {
      head = curPos = calloc(1, sizeof(ContextNode));
      curPos->next = head;
      if (theScheduler->init != NULL)
      {
         theScheduler->init(); /*init scheduler if there is 
          an init method*/
      }
   }

   while (curPos->next != head)
   {
      curPos = curPos->next;
   }
   curPos->cntx.state.fxsave = FPU_INIT; /*initialize floating 
   point registers*/
   curPos->cntx.tid = tmptid = curId;

   curPos->cntx.stack = calloc(stackSize, sizeof(unsigned long)); /*clear
   the stack of garbage and allocate data*/
   curPos->cntx.stack += stackSize; /*jump to the top of the stack*/
   if (argument)
   {
      curPos->cntx.state.rdi = (unsigned long)argument; /*initialize
      the destination index to contain the argument if it exists*/
   }

   curPos->cntx.stack--; /*decrement stack and add the function
   pointers in the correct order such that if popped it will
   load the program counter correctly*/
   *(curPos->cntx.stack) = (unsigned long)
    lwp_exit; /*return address here that pc will load*/
   curPos->cntx.stack--;
   *(curPos->cntx.stack) = (unsigned long)
    data; /*loads data function first when context gets loaded*/
   curPos->cntx.stack--;

   curPos->cntx.state.rbp = curPos->cntx.state.rsp = 
    (unsigned long)curPos->cntx.stack; /*initialize the
    register file to contain the base pointer and stack
    pointer for stack popping later*/
   curPos->isPaused = 1;
   curPos->cntx.stacksize = stackSize * sizeof(unsigned long);
   curId++;

   curPos->next = malloc(sizeof(ContextNode)); /*set up the next thread
   in the circular linked list*/
   curPos->next->next = head;
   theScheduler->admit(&(curPos->cntx)); /*admit a thread into the
   scheduler as you create so you can increment through with
   any scheduler*/
   curPos = head;
   return tmptid;

}
/*This function simply returns the id of the current thread
and its ID value*/
tid_t lwp_gettid() {
   if(curThread && curThread->tid)
      return curThread->tid;
   else
      return NO_THREAD;
}

/*pause the thread for later and move onto the next avaliable
scheduled thread*/
void lwp_yield() {
   thread tmpthread = curThread;
   curThread = theScheduler->next();
   if (curThread == NULL)
   { 
      swap_rfiles(&(tmpthread->state),&(mainCntx.state)); /*save the cur
      context and load the main so you save the current state but load
      back to where lwp start occured*/
      return; /*return to ensure stack popped*/
   }
   curPos->isPaused = 0;
   swap_rfiles(&(tmpthread->state), &(curThread->state)); /*save the
   current thread and load the next one*/
   return;

}

/*start the lwp program and run through the created
threads based on the order given by the scheduler*/
void lwp_start() {
   if (curId == 0) /*if no thread exists go back to where it was called*/
   {
      return;
   }
   context newContext;
   mainCntx.state.fxsave = FPU_INIT; /*initialize a mainCntx floating point*/

   if (isPaused)
   { /*if the program has paused, retrieve the context from 
   where it previously stopped*/
      isPaused = 0;
      newContext = mainCntx;
      swap_rfiles(&(mainCntx.state), &(newContext.state)); /*load 
      the previous stateand save the main context from where 
      it was called*/
      return;
   }
   curThread = theScheduler->next(); /*increment to the next thread*/
   swap_rfiles(&(mainCntx.state), &(curThread->state)); 

}

/*stop the current thread, save it, and return to where lwp_start
was called*/
void lwp_stop() {
   isPaused = 1;
   thread tempContext = calloc(1, sizeof(context)); /*create a
   temp context*/
   tempContext->state = mainCntx.state;
   swap_rfiles(&(mainCntx.state), &(tempContext->state)); /*copy the previous
   value of main context and into the registers and load the new context into 
   main context such that it doesn't corrupt the previous save context*/
   return;
}

/*set scheduler takes a new scheduler, initializes it, 
copies all the old threads to it, and sets the main scheduler 
function pointers to the new scheduler function 
pointers*/
void lwp_set_scheduler(scheduler fun) {
   thread temp;
   if(fun) 
   {
      if( fun->init) 
      {
         theScheduler->init = fun->init;
      }
      theScheduler->admit = fun->admit;
      theScheduler->remove = fun->remove;
      theScheduler->shutdown = fun->shutdown;
      theScheduler->next = fun->next;
      /*swap the function pointers and copy over the
      new threads in the while loop below*/
      while ((temp = theScheduler->next()))
      {
         theScheduler->remove(temp);
         fun->admit(temp);
      }
   }
}

/*just return the current scheduler*/
scheduler lwp_get_scheduler(void) {
   return theScheduler;
}

/*return the thread if there is a matching tid with 
the parameters, otherwise pass null*/
thread tid2thread(tid_t tid) {
   ContextNode *headTemp = head;

   if (tid < 0 || tid > curId)
   {
      return NULL;
   }

   while ( headTemp && headTemp->next && headTemp->next != head)
   { /*check until at the end of the list and not null*/

      if (headTemp->cntx.tid == tid)
      { /*if the two tids are equals then return*/
         return &(headTemp->cntx);
      }
      else
      {
         headTemp = headTemp->next;
      }
   }
   return NULL;
}