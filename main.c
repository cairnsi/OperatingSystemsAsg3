
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
#define MAXBGPROCESSES 200
bool ignoreAmp = false;
bool midPrompt = false;


/* struct for command*/
struct commandLine
{
    char** arguments;
    bool background;
    char* inputFile;
    char* outputFile;
};

/*expand the $$ variable*/
char* expandVariable(char* input) {
    //get the pid and convert to a string
    int pid = getpid();
    char* buffer = calloc(30, sizeof(char));
    sprintf(buffer, "%d", pid);
    int bufferLength = strlen(buffer);

    // look through the string and replace the $$. If found look again.
    bool lookagain = true;
    while (lookagain) {
        int inputLength = strlen(input);
        char* ptr = input;
        lookagain = false;
        //loop till you reach the end of the string
        while (*(ptr + 1) != 0) {
            //check if you reached $$
            if (*ptr == '$' && *(ptr + 1) == '$') {
                //if found look again
                lookagain = true;
                //set the $ to 0 for string cat
                *ptr = 0;
                *(ptr + 1) = 0;
                // create new string with the size of the input plus the pid plus the null char
                char* temp = calloc((bufferLength + inputLength - 1), sizeof(char));
                //concat the string together
                strcat(temp, input);
                strcat(temp, buffer);
                strcat(temp, ptr + 2);
                //release the old input memory
                free(input);
                input = temp;
                break;
            }
            // look at the next set of two characters
            ptr++;
        }
    }
    free(buffer);
    return input;
}

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
            //expand variable
            command->inputFile = expandVariable(command->inputFile);
        }
        else if (strcmp(token, ">") == 0) {
            //get the file name and set it as the output file
            token = strtok_r(NULL, " ", &saveptr);
            command->outputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(command->outputFile, token);
            //expand variable
            command->outputFile = expandVariable(command->outputFile);
        }
        else if (strcmp(token, "&") == 0) {
            //set the background boolean
            command->background = true;
        }
        else {
            // if the last & wasn't the end of file add it to the arguments
            if (command->background) {
                command->arguments[index] = calloc(2, sizeof(char));
                strcpy(command->arguments[index], "&");
                index++;
                command->background = false;
            }
            // if not tags caught, then it is an argument. Add it to the arguments array
            command->arguments[index] = calloc(strlen(token) + 1, sizeof(char));
            strcpy(command->arguments[index], token);
            //check for variable expansion
            command->arguments[index] = expandVariable(command->arguments[index]);
            index++;
        }

        // get next portion of command
        token = strtok_r(NULL, " ", &saveptr);
    }

    //make last argument NULL
    command->arguments[index] = NULL;

    return command;
}

/* signal handler for sigint. Do nothing*/
void handle_SIGINT(int signo) {
    
}

/* signal handler for SIGTSTP. Much of this was from the class signal handler module*/
void handle_SIGTSTP(int signo) {
    char* message; 
    // check what the ignore status is and change it
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
    //and the prompt if that is where you were
    if (midPrompt) {
        message = ": ";
        write(STDOUT_FILENO, message, 2);
        fflush(stdout);
    }
}

/* register signal handlers. Most of this is from the class module on signal handling*/
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
    SIGINT_action.sa_flags = SA_RESTART;

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
    SIGTSTP_action.sa_flags = SA_RESTART;

    // Install our signal handler
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}


/* set ignor control z*/
void ignorControlZ() {
    struct sigaction SIGTSTP_action = { { 0 } };
    // Fill out the SIGTSTP_action struct
    // Register ignor as the signal handler
    SIGTSTP_action.sa_handler = SIG_IGN;
    // Block all catchable signals while handle_SIGTSTP is running
    sigfillset(&SIGTSTP_action.sa_mask);
    // No flags set
    SIGTSTP_action.sa_flags = 0;

    // Install our signal handler
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}

/* set ignor control c*/
void ignorControlC() {
    struct sigaction SIGINT_action = { { 0 } };
    // Fill out the SIGINT_action struct
    // Register ignor as the signal handler
    SIGINT_action.sa_handler = SIG_IGN;
    // Block all catchable signals while handle_SIGINT is running
    sigfillset(&SIGINT_action.sa_mask);
    // No flags set
    SIGINT_action.sa_flags = 0;

    // Install our signal handler
    sigaction(SIGINT, &SIGINT_action, NULL);
}


/*run command*/
void runCommand(struct commandLine* command, char** status, int* backgroundProcess, struct commandLine** backgroundCommand) {
    int childStatus;
    int inputFile;
    //check if input file was given
    if (command->inputFile != NULL) {
        // Open input file if it exists
        inputFile = open(command->inputFile, O_RDONLY);
        if (inputFile == -1) {
            printf("cannot open \"%s\" for input\n", command->inputFile);
            fflush(stdout);
            free(*status);
            *status = calloc(15, sizeof(char));
            strcpy(*status, "exit value 1\n");
            return;
        }
    }

    int outputFile;
    //check if output file was given
    if (command->outputFile != NULL) {
        // Open input file if it exists
        outputFile = open(command->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (outputFile == -1) {
            printf("cannot open \"%s\" for output\n", command->outputFile);
            fflush(stdout);
            free(*status);
            *status = calloc(15, sizeof(char));
            strcpy(*status, "exit value 1\n");
            return;
        }
    }

    int devNull;
    //open dev null if running the background
    if (command->background && !ignoreAmp) {
        devNull = open("/dev/null", O_RDWR);
        if (outputFile == -1) {
            printf("cannot open dev/null\n");
            fflush(stdout);
            free(*status);
            *status = calloc(15, sizeof(char));
            strcpy(*status, "exit value 1\n");
            return;
        }
    }
    // Fork a new process
    pid_t spawnPid = fork();
    switch (spawnPid) {
    case -1:
        break;
    case 0:
        //set input file if there is one. Set it to stdin
        if (command->inputFile != NULL) {
            int result = dup2(inputFile, 0);
            if (result == -1) {
                printf("cannot set stdin to \"%s\"\n", command->inputFile);
                fflush(stdout);
                exit(EXIT_FAILURE);
            }
        }
        else if (command->background && !ignoreAmp) {
            //set dev/null if no input file and running in the background
            int result = dup2(devNull, 0);
            if (result == -1) {
                printf("cannot set stdin to /dev/null\n");
                fflush(stdout);
                exit(EXIT_FAILURE);
            }
        }
        //check output file and set it to stdout
        if (command->outputFile != NULL) {
            int result = dup2(outputFile, 1);
            if (result == -1) {
                printf("cannot set stdout to \"%s\"\n", command->inputFile);
                fflush(stdout);
                exit(EXIT_FAILURE);
            }
        }
        else if (command->background && !ignoreAmp) {
            //set dev/null for out put if running in background
            int result = dup2(devNull, 1);
            if (result == -1) {
                printf("cannot set stdout to /dev/null\n");
                fflush(stdout);
                exit(EXIT_FAILURE);
            }
        }
        //ignor control c if running in background
        if (command->background && !ignoreAmp) {
            ignorControlC();
        }

        //ignore control z
        ignorControlZ();
        execvp(command->arguments[0], command->arguments);
        exit(EXIT_FAILURE);
        break;
    default:
        //check if running int the background
        if (command->background && !ignoreAmp) {
            //show background pid
            printf("background pid is %d\n", spawnPid);
            fflush(stdout);
            //add pid to list to watch as well as the command
            for (int i = 0; i < MAXBGPROCESSES; i++) {
                if (backgroundProcess[i] == 0) {
                    backgroundProcess[i] = spawnPid;
                    backgroundCommand[i] = command;
                    break;
                }
            }
            break;
        }
        // In the parent process
        // Wait for child's termination if not in background
        // set the status based on the childStatus variable
        spawnPid = waitpid(spawnPid, &childStatus, 0);
        if (childStatus == 0) {
            free(*status);
            *status = calloc(15, sizeof(char));
            strcpy(*status, "exit value 0\n");
        }
        else if (childStatus == 2) {
            // catch control c
            printf("terminated by signal 2\n");
            fflush(stdout);
            free(*status);
            *status = calloc(23, sizeof(char));
            strcpy(*status, "terminated by signal 2\n");
        }
        else if (childStatus == 256) {
            //catch bad command
            printf("%s: no such file or directory\n", command->arguments[0]);
            fflush(stdout);
            free(*status);
            *status = calloc(15, sizeof(char));
            strcpy(*status, "exit value 1\n");
        }
        else {
            // catch any others
            free(*status);
            *status = calloc(15, sizeof(char));
            strcpy(*status, "exit value 1\n");
        }
        break;
    }
}

/* Check background commands*/
void checkBackgroundCommands(int* backgroundProcess, struct commandLine** backgroundCommand, char** status) {
    int childStatus;
    // look through all background commands
    for (int i = 0; i < MAXBGPROCESSES; i++) {
        //check if one is set at current spot
        if (backgroundProcess[i] != 0) {
            //see if it is done
            int waitStatus = waitpid(backgroundProcess[i], &childStatus, WNOHANG);
            struct commandLine* command = backgroundCommand[i];
            if (waitStatus > 0) {
                // if it is catch the bad command
                if (childStatus == 256) {
                    printf("%s: no such file or directory\n", command->arguments[0]);
                    fflush(stdout);
                    free(*status);
                    *status = calloc(15, sizeof(char));
                    strcpy(*status, "exit value 1\n");
                    backgroundProcess[i] = 0;
                    backgroundCommand[i] = NULL;
                }
                else if (childStatus != 0) {
                    //or tell what terminated it
                    printf("background pid %d is done: terminated by signal %d\n", backgroundProcess[i], childStatus);
                    fflush(stdout);
                    backgroundProcess[i] = 0;
                    backgroundCommand[i] = NULL;
                }
                else {
                    // lastly report it closed succusfully if exit is 0
                    printf("background pid %d is done : exit value 0\n", backgroundProcess[i]);
                    fflush(stdout);
                    backgroundProcess[i] = 0;
                    backgroundCommand[i] = NULL;
                }
            }
        }
    }
}

/* kill all running child processes. Used on exit*/
void killAll(int* backgroundProcess) {
    // check all the running background processes and kill them
    for (int i = 0; i < MAXBGPROCESSES; i++) {
        if (backgroundProcess[i] != 0) {
            kill(backgroundProcess[i], 15);
        }
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
    //set up array for storing background processes
    int* backgroundProcess = calloc(MAXBGPROCESSES, sizeof(int));
    struct commandLine** backgroundCommand = calloc(MAXBGPROCESSES, sizeof(struct commandLine*));
    //set up status
    char** status = malloc(sizeof(char*));
    *status = calloc(15, sizeof(char));
    strcpy(*status, "exit value 0\n");
    //register status
    registerSigHandlers();
    bool askAgain = true;
    //run outer loop asking if user would like to specify a file
    do {
        printf(": ");
        fflush(stdout);
        //allocate max input plus 1 for null at the end
        char* input = calloc(MAXINPUT+1, sizeof(char));
        //get user input
        midPrompt = true;
        fgets(input, MAXINPUT, stdin);
        midPrompt = false;
        //parse the command
        struct commandLine* command = createCommand(input);
        //check if there was a real command
        if (command->arguments[0] != NULL && command->arguments[0][0] != '#') {
            if (strcmp(command->arguments[0], "exit") == 0) {
                //exit the script
                askAgain = false;
            }
            else if (strcmp(command->arguments[0], "status") == 0) {
                //display status
                printf(*status);
                fflush(stdout);
            }
            else if (strcmp(command->arguments[0], "cd") == 0) {
                //check if directory should change to home
                if (command->arguments[1] == NULL) {
                    //change to HOME path variable
                    chdir(getenv("HOME"));
                }
                else {
                    //change to specified path
                    chdir(command->arguments[1]);
                }
            }
            else {
                // run the given command
                runCommand(command, status, backgroundProcess, backgroundCommand);
            }
        }
        free(input);
        //check if background commands have completed
        checkBackgroundCommands(backgroundProcess, backgroundCommand, status);

    } while (askAgain);
    //on exit kill any currently running processes
    killAll(backgroundProcess);

    return EXIT_SUCCESS;
}
