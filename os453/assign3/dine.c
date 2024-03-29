/*
 * trivial_pt.c is a re-make of asgn1's fork-testing program
 * done as as demonstration of pthreads.
 *
 * -PLN
 *
 * Requres the pthread library.  To compile:
 *   gcc -Wall -lpthread -o trivial_pt trivial_pt.c
 *
 * Revision History:
 *
 * $Log: trivial_pt.c,v $
 * Revision 1.1  2003-01-28 12:55:00-08  pnico
 * Initial revision
 *
 *
 *
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>
#define NUM_PHILOSOPHERS 5

static pthread_t *philosophers; /*array of philosopher threads*/
static pthread_mutex_t *forks; /*array of fork semaphores*/
static int *states; //0---changing 1----eating 2------thinking
static int *forksInUse; /*list of forks with respect to philosophers*/
static int numLoops = 1; /*number of times to eat and think*/
static pthread_mutex_t printing; /*synchronization semaphore in order to
stop all threads while a change in state is printed*/

/*generic print function that prints
the current state of the threads*/
void changeStatePrint(int id) {
   char *tmpString;
   int i = 0, b = 0;

   pthread_mutex_lock(&printing); /*lock threads from doing stuff
   while this function is currently printing*/
   while (i < NUM_PHILOSOPHERS)
   { /*iterate for the state of all philosophers*/
      if (states[i] == 1)
      { /*is eating*/
         tmpString = " Eat   ";
      }
      else if (states[i] == 2)
      { /*is thinking*/
         tmpString = " Think ";
      }
      else
      { /*is changing*/
         tmpString = "       ";
      }
      printf("| ");
      b = 0;
      while (b < NUM_PHILOSOPHERS)
      { /*loop for the forks in use for each philosopher*/
         if ((forksInUse[b] == i))
         {
            printf("%c", b + '0');
         }
         else
         {
            printf("-"); /*represents philosopher not 
             using a fork*/
         }
         b++;
      }
      printf("%s", tmpString); 
      i++;

   }
   printf("|\n");
   pthread_mutex_unlock(&printing); /*let the threads continue*/
}

/*dawdle function from the spec, which enables
a random duration for thinking and eating*/
void dawdle() {
   struct timespec tv;
   int msec = (int)(((double)random() / INT_MAX) * 1000);
   tv.tv_sec = 0;
   tv.tv_nsec = 1000000 * msec;
   if(-1 == nanosleep(&tv,NULL))
   {
      perror("nanosleep");
   }
}

/*error handling for lock and unlock*/
void checkErrorLockUnlock(int res, int whoami)
{
   if (res == -1)
   { /*handle error case*/
      fprintf(stderr, "Lock or Unlock %i: %s\n",whoami, strerror(errno));
      exit(EXIT_FAILURE);
   }
}
/*convenience function that locks and
unlocks in order to prevent threads
from continuing whenever the printing
function is in the process of printing*/
void waitPrint(int whoami) {
   checkErrorLockUnlock(pthread_mutex_lock(&printing), whoami);
   checkErrorLockUnlock(pthread_mutex_unlock(&printing), whoami);
}

/*thread representing the philosophers
and all of the logic for each state
they can be in. Requires many waitPrint() calls
so no thread can change a state at any point*/
void *threadPhilo(void *id) {
   int whoami = *((int*)id);
   int numIterations = numLoops;
   while (numIterations--)
   { /*iterate as specified by argv[1]*/
      if (whoami % 2 == 0)
         { /*if even*/
            waitPrint(whoami);
            checkErrorLockUnlock(pthread_mutex_lock(&forks[(whoami + 1) 
             % (NUM_PHILOSOPHERS)]), whoami); /*pick up right fork first*/
            waitPrint(whoami);
            forksInUse[(whoami + 1) % (NUM_PHILOSOPHERS)] = whoami; /*set 
            fork to used*/
            waitPrint(whoami);
            changeStatePrint(whoami);

            waitPrint(whoami);
            checkErrorLockUnlock(pthread_mutex_lock(&forks[(whoami) 
             % (NUM_PHILOSOPHERS)]), whoami); /*pick up left fork after*/
            waitPrint(whoami);
            forksInUse[(whoami) % (NUM_PHILOSOPHERS)] = whoami; /*set 
            fork to used*/
            waitPrint(whoami);
            changeStatePrint(whoami);
         }
         else
         { /*if odd*/
            waitPrint(whoami);
            checkErrorLockUnlock(pthread_mutex_lock(&forks[(whoami) 
             % (NUM_PHILOSOPHERS)]), whoami);
            waitPrint(whoami);
            forksInUse[(whoami) % (NUM_PHILOSOPHERS)] = whoami; /*set 
             fork to used*/
            waitPrint(whoami);
            changeStatePrint(whoami);
            waitPrint(whoami);
            checkErrorLockUnlock(pthread_mutex_lock(&forks[(whoami + 1) 
             % (NUM_PHILOSOPHERS)]), whoami);
            waitPrint(whoami);
            forksInUse[(whoami + 1) % (NUM_PHILOSOPHERS)] = whoami; /*set 
            fork to used*/
            changeStatePrint(whoami);
         }
         waitPrint(whoami);
         states[whoami] = 1;/*start eating*/
         changeStatePrint(whoami); /*print 
          the current state after switching to eating*/
         waitPrint(whoami);
         dawdle(); /*eat in progress*/
         waitPrint(whoami);
         states[whoami] = 0;/*changing*/
         changeStatePrint(whoami); /*print
          the current state after switching to changing*/

         if (whoami % 2 == 0)
         { /*if even*/
            waitPrint(whoami);
            checkErrorLockUnlock(pthread_mutex_unlock(&forks[(whoami + 1) 
             % (NUM_PHILOSOPHERS)]), whoami); /*put down right fork*/
            waitPrint(whoami);
            if (forksInUse[(whoami + 1) % (NUM_PHILOSOPHERS)] == whoami)
            {
               forksInUse[(whoami + 1) % (NUM_PHILOSOPHERS)] = -1; /*set 
               fork to used*/
            }
            
            changeStatePrint(whoami); /*print new fork update*/
            waitPrint(whoami);
            checkErrorLockUnlock(pthread_mutex_unlock(&forks[(whoami) 
             % (NUM_PHILOSOPHERS)]), whoami); /*put down left fork*/
            waitPrint(whoami);
            if (forksInUse[(whoami) % (NUM_PHILOSOPHERS)] == whoami)
            {
               forksInUse[(whoami) % (NUM_PHILOSOPHERS)] = -1; /*set 
                fork to used*/
            }
            waitPrint(whoami);
            changeStatePrint(whoami); /*print again*/
         }
         else
         { /*if odd*/
            waitPrint(whoami);
            checkErrorLockUnlock(pthread_mutex_unlock(&forks[(whoami) 
             % (NUM_PHILOSOPHERS)]), whoami); /*put down left fork*/
            waitPrint(whoami);
            if (forksInUse[(whoami) % (NUM_PHILOSOPHERS)] == whoami)
            {
               forksInUse[(whoami) % (NUM_PHILOSOPHERS)] = -1; /*set 
                fork to unused*/
            }
            waitPrint(whoami);
            changeStatePrint(whoami);
            waitPrint(whoami);
            checkErrorLockUnlock(pthread_mutex_unlock(&forks[(whoami + 1) 
             % (NUM_PHILOSOPHERS)]), whoami); /*put down right fork*/
            waitPrint(whoami);
            if (forksInUse[(whoami + 1) % (NUM_PHILOSOPHERS)] == whoami)
            {
               forksInUse[(whoami + 1) % (NUM_PHILOSOPHERS)] = -1; /*set 
                fork to unused*/
            }
            waitPrint(whoami);
            changeStatePrint(whoami);
         }
         waitPrint(whoami);
         states[whoami] = 2;/*start thinking*/
         waitPrint(whoami);
         changeStatePrint(whoami); /*print the current 
          state after switching to thinking*/
         waitPrint(whoami);
         dawdle(); /*eat in progress*/
         waitPrint(whoami);
         states[whoami] = 0;/*changing*/
         waitPrint(whoami);
         changeStatePrint(whoami); /*final print--done*/
   }
   return NULL;
}

/*convienience function to print the last
footer*/
void printFooter()
{
   int i = 0;
   int b = 0;

   while (i < NUM_PHILOSOPHERS)
   {
      printf("|========");
      b = 0;
      while (b < NUM_PHILOSOPHERS)
      {
         printf("=");
         b++;
      }
      i++;
   }
   printf("|\n");   
}

/*convenience function to set up the
header based on the number of philosophers*/
void printLineHeader() {
   int i = 0;
   int b;


   while (i < NUM_PHILOSOPHERS)
   {
      pthread_mutex_init(&forks[i], NULL); /*might as 
      well initialize in this header initialization loop*/
      printf("|========");
      b = 0;
      while (b < NUM_PHILOSOPHERS)
      { /*print a specific number of = spacers*/
         printf("=");
         b++;
      }
      i++;
   }
   printf("|\n"); 
   i = 0;
   while (i < NUM_PHILOSOPHERS)
   {
      printf("| ");
      b = 0;
      while (b < NUM_PHILOSOPHERS)
      { /*print a specific number of spaces*/
         printf(" ");
         b++;
      }
      printf("%c      ", i + 'A');
      i++;
   }
   printf("|\n");
   i = 0;
   while (i < NUM_PHILOSOPHERS)
   {
      printf("|========");
      b = 0;
      while (b < NUM_PHILOSOPHERS)
      { /*print a specific number of = spacers*/
         printf("=");
         b++;
      }
      i++;
   }
   printf("|\n"); 
}

/*initialize by setting up forks to unused*/
void initForks()
{
   int c = 0;
   while (c < NUM_PHILOSOPHERS)
   {
      forksInUse[c] = -1;
      c++;
   }

}

/*cleanup the semaphores used by forks and printing*/
void destroyMutex()
{
   int i = 0;
   while (i < NUM_PHILOSOPHERS)
   {
      pthread_mutex_destroy(&forks[i]);
      i++;
   }
}

/*allocates my stuff and enables dynamic arrays and scans in the
number of times to eat and think*/
int main(int argc, char *argv[]) {
   int i = 0;
   int res;
   int constantValues[NUM_PHILOSOPHERS]; /*used to pass
   a constant value to the thread create loop*/

   while (i < NUM_PHILOSOPHERS)
   { /*fill up constantValues with a number of different
   whoamis*/
      constantValues[i] = i;
      i++;
   }
   i = 0;
   if (argv[1])
   {
      numLoops = atoi(argv[1]);
   }

   /*allocate some stuff and initalize*/
   philosophers = calloc(NUM_PHILOSOPHERS, sizeof(pthread_t));
   forks = calloc(NUM_PHILOSOPHERS, sizeof(pthread_mutex_t));
   states = calloc(NUM_PHILOSOPHERS, sizeof(int));
   forksInUse = calloc(NUM_PHILOSOPHERS, sizeof(int));
   printLineHeader();
   pthread_mutex_init(&printing, NULL);

   initForks();
   i = 0;
   while (i < NUM_PHILOSOPHERS)
   { /*create all my threads with the constant whoami pointers*/
      res = pthread_create(
         &philosophers[i], /*identifier */
         NULL, /* no special properties */
         threadPhilo, /* call the function child() */
         (void *)(&constantValues[i]) /* pass the whoami */
      );
      if (res == -1)
      { /*handle error case*/
         fprintf(stderr, "Create error %i: %s\n", i, strerror(errno));
         exit(EXIT_FAILURE);
      }
      i++;
   }

   i = 0;
   while(i < NUM_PHILOSOPHERS)
   { /*wait for threads to finish*/
      if (philosophers[i])
      {
         res = pthread_join(philosophers[i], NULL);
         if (res != 0)
         { /*handle error case*/
            fprintf(stderr, "Wait/join error %i: %s\n", i, strerror(errno));
            exit(EXIT_FAILURE);
         }
      }
      i++;
   }
   printFooter();

   /*cleanup the mess*/
   destroyMutex();
   free(philosophers);
   free(forks);
   free(states);
   return 0;
}
