
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
#include <unistd.h>   

#define MAXINPUT 2048

struct commandLine
{
    char** arguments;
    bool background;
    char* inputFile;
    char* outputFile;
};

/* Parse the current line
*/
struct commandLine* createCommand(char* currLine)
{
    struct commandLine* command = malloc(sizeof(struct commandLine));

    // For use with strtok_r
    char* saveptr;

    //create memory for arguments; 512 + 1 for the command and 1 for null;
    command->arguments = calloc(514, sizeof(char*));
    command->background = false;
    command->inputFile = NULL;
    command->outputFile = NULL;
    int index = 0;

    //get rid of trailing new line
    currLine[strlen(currLine) - 1] = 0;
    // Get the first element
    char* token = strtok_r(currLine, " ", &saveptr);

    while (token != NULL) {
        //check for input output and background chars
        if (strcmp(token, "<") == 0) {
            //get the file name and set it as the input file
            token = strtok_r(NULL, " ", &saveptr);
            command->inputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(command->inputFile, token);
        }
        else if (strcmp(token, ">") == 0) {
            //get the file name and set it as the output file
            token = strtok_r(NULL, " ", &saveptr);
            command->outputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(command->outputFile, token);
        }
        else if (strcmp(token, "&") == 0) {
            //set the background boolean
            command->background = true;
        }
        else {
            // if not tags caught, then it is an argument. Add it to the arguments array
            command->arguments[index] = calloc(strlen(token) + 1, sizeof(char));
            strcpy(command->arguments[index], token);
            index++;
        }

        // get next portion of command
        token = strtok_r(NULL, " ", &saveptr);
    }

    //make last argument NULL
    command->arguments[index] = NULL;

    return command;
}

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
    do {
        printf(": ");
        //allocate max input plus 1 for null at the end
        char* input = calloc(MAXINPUT+1, sizeof(char));
        fgets(input, MAXINPUT, stdin);
        struct commandLine* command = createCommand(input);

        askAgain = false;
        free(input);
    } while (askAgain);
    

    return EXIT_SUCCESS;
}
