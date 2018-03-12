/*header:
@author: Aubrey Russell
@program: os 453 asgn 6
@Submit date: 3/11/2016
* This program is designed to print the statment 
* Hello, World!\n to the terminal using the 
* standard c library and a simple main function.
* The goal of this program is to allow us to reflect
* on the vast amount of complexity it takes to execute
* this code. At the end of the program, main simply 
* returns with status 0 representing success.
* 
*/
#include <stdio.h> /*include the standard io libraries
in order to allow for printing and normal program operation
*/

/*main portion of source code that runs the */
int main(int argc, char **argv) {
    /*print Hello, world!\n using the printf function*/
    printf("Hello, world!\n");
    /*exit with status 0 for success*/
    return 0;
}
