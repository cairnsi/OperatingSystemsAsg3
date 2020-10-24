
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>   // for execv

struct movie
{
    char* command;
    char** arguments;
    double rating;
    struct movie* next;
};

/*
*       main function for smallsh terminal
*       this code will drive a terminal that will take in bash commmands and 
*       process them similar to a terminal
*       gcc --std=gnu99 -o smallsh main.c
*/
int main(int argc, char* argv[])
{
    bool askAgain = true;
    //run outer loop asking if user would like to specify a file
    /*do {
        printf(": ");
        char* input = calloc(2048, sizeof(char));
        scanf("%s", input);
        

        free(input);
    } while (askAgain);*/
    
    char* newargv[] = { "/bin/echo", "pwd", "(10", "points", "for being in the HOME dir)", NULL };
    execv(newargv[0], newargv);
    /* exec returns only on error */
    perror("execv");
    exit(EXIT_FAILURE);

    return EXIT_SUCCESS;
}
