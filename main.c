
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
#include <signal.h>
#include <sys/wait.h> // for waitpid

#define MAXINPUT 2048
bool ignoreAmp = false;

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

    // parse command
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

/* signal handler for sigint. Much of this was from the class signal handler module*/
void handle_SIGINT(int signo) {
    
    
}

/* signal handler for SIGTSTP. Much of this was from the class signal handler module*/
void handle_SIGTSTP(int signo) {
    char* message; 
    if (ignoreAmp) {
        message = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 30);
        fflush(stdout);
        ignoreAmp = false;
    }
    else {
        message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 50);
        fflush(stdout);
        ignoreAmp = true;
    }

}

/* register signal handlers*/
void registerSigHandlers() {
    // Initialize SIGINT_action struct to be empty
    struct sigaction SIGINT_action = { { 0 } };

    // Fill out the SIGINT_action struct
    // Register handle_SIGINT as the signal handler
    SIGINT_action.sa_handler = handle_SIGINT;
    // Block all catchable signals while handle_SIGINT is running
    sigfillset(&SIGINT_action.sa_mask);
    // No flags set
    SIGINT_action.sa_flags = 0;

    // Install our signal handler
    sigaction(SIGINT, &SIGINT_action, NULL);

    // Initialize SIGTSTP_action struct to be empty
    struct sigaction SIGTSTP_action = { { 0 } };

    // Fill out the SIGTSTP_action struct
    // Register handle_SIGTSTP as the signal handler
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    // Block all catchable signals while handle_SIGTSTP is running
    sigfillset(&SIGTSTP_action.sa_mask);
    // No flags set
    SIGTSTP_action.sa_flags = 0;

    // Install our signal handler
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}

/*run command*/
void runCommand(struct commandLine* command, char* status) {
    int childStatus;

    // Fork a new process
    pid_t spawnPid = fork();

    switch (spawnPid) {
    case -1:
        break;
    case 0:
        execvp(command->arguments[0], command->arguments);
        
        exit(EXIT_FAILURE);
        break;
    default:
        // In the parent process
        // Wait for child's termination
        spawnPid = waitpid(spawnPid, &childStatus, 0);
        if (childStatus == 0) {
            free(status);
            status = calloc(15, sizeof(char));
            strcpy(status, "exit value 0\n");
        }
        else {
            free(status);
            status = calloc(15, sizeof(char));
            strcpy(status, "exit value 1\n");
        }
        break;
    }
}

/*
*       main function for smallsh terminal
*       this code will drive a terminal that will take in bash commmands and 
*       process them similar to a terminal
*       gcc --std=gnu99 -o smallsh main.c
*/
int main(int argc, char* argv[])
{
    //set up status
    char* status = calloc(15, sizeof(char));
    strcpy(status, "exit value 0\n");
    //register status
    registerSigHandlers();
    bool askAgain = true;
    //run outer loop asking if user would like to specify a file
    do {
        printf(": ");
        fflush(stdout);
        //allocate max input plus 1 for null at the end
        char* input = calloc(MAXINPUT+1, sizeof(char));
        fgets(input, MAXINPUT, stdin);
        struct commandLine* command = createCommand(input);
        if (command->arguments[0] != NULL && command->arguments[0][0] != '#') {
            if (strcmp(command->arguments[0], "exit") == 0) {
                askAgain = false;
            }
            else if (strcmp(command->arguments[0], "status") == 0) {
                printf(status);
                fflush(stdout);
            }
            else if (strcmp(command->arguments[0], "cd") == 0) {

            }
            else {
                runCommand(command, status);
            }
        }
        free(input);
    } while (askAgain);
    

    return EXIT_SUCCESS;
}
