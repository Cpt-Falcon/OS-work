/*header:
@author: Aubrey Russell
@program: os 453 Lab 1
@Submit date: 1/8/2016
The goal of this first program is to hardcode, using C system calls, the 
unix shell logic for the following bash command: “ls | sort -r > outfile
”. In order to accomplish this, one has to use two fork statements and
create a pipe so that ls can send its output to sort, which can then
write its reverse sroted contents to an output file. In order to work with the
unix bash commands via execlp, this code uses dup2 to replace stdin and 
stdout in each fork with the corresponding pipe that the commands need to
read and write from. The program then closes all the unused pipes, waits for
the children to end, and handles every error case that could occur from one
of the many system calls used in this code.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

/*checkStatus- convenience function to see if the result of an error 
is less than zero, which implies a failure. Then, if there was an 
error, include a custom string with perror to know what the 
error is referring to exactly. Finally, exit the process with the exit value
of the error. */

void checkStatus(char *str, int statVal) {
    int status;
    if (statVal < 0)
    {
        perror(str);
        wait(&status); /*wait for processes to finish 
        just in case to prevent zombies at all costs before exit*/
        wait(&status);
        exit(statVal);
    }
}

int main(int argc, char **argv) {
    int lsSortPipe[2];
    int pid1, pid2, status, outFile;

    pipe(lsSortPipe); /*create the main pipe where ls is writing to sort*/
    outFile = open("./outfile", O_CREAT | 
     O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    checkStatus("outfile", outFile); /*check if file opened correctly*/
    if ((pid1 = fork()) == -1)	/*fork for ls error state*/
    {
        perror("ls fork failure");
        exit(EXIT_FAILURE);	
    }
    
    else if (pid1 == 0)	/*child process ls*/
    {
        close(lsSortPipe[0]);
        dup2(lsSortPipe[1], STDOUT_FILENO);  /*duplicate the pipe to 
         standard out so ls will write to the correct fd*/
        checkStatus("Exec sort failure", 
         execlp("ls", "ls", (char *)NULL)); /*verify exec worked*/
        close(lsSortPipe[1]);
        return 0;
    }

    else /*parent*/
    {
        if ((pid2 = fork()) == -1)	/*fork for sort error state*/
        {
            perror("Sort fork failure");
            exit(EXIT_FAILURE);
        }
        
        else if (pid2 == 0)	/*child process sort*/
        {
            close(lsSortPipe[1]);
            dup2(lsSortPipe[0], STDIN_FILENO); /*duplicate 
             the pipe to standard in*/
            dup2(outFile, STDOUT_FILENO); /*duplicate the pipe to 
             standard out for the output file*/

            checkStatus("Exec sort failure", 
             execlp("sort", "sort", "-r", (char *)NULL)); /*verify 
             exec worked*/
            close(outFile);
            close(lsSortPipe[0]);
            return 0;
        }

        else /*parent again*/
        {
            close(lsSortPipe[0]); /*close unused pipes*/
            close(lsSortPipe[1]);
            wait(&status); /*wait for both forks to 
            finish and report error if it exists*/
            checkStatus("Wait error", status);
            wait(&status);
            checkStatus("Wait error", status);
        }
    }
    return 0;

}
