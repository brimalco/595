#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "tokenizer.h"

#define INPUT_SIZE 1024

pid_t childPid = 0;

char *tok = NULL;

char * right = NULL;

char * left = NULL;

void executeShell(int timeout);

void writeToStdout(char *text);

void alarmHandler(int sig);

void sigintHandler(int sig);

char **getCommandFromInput();

void registerSignalHandlers();

void killChildProcess();

void redirectionsSTDOUTtoFile(char* tokenAfterOut);

void redirectionsSTDINtoFile(char* tokenAfterIn);

int main(int argc, char **argv) {
    registerSignalHandlers();

    int timeout = 0;
    if (argc == 2) {
        timeout = atoi(argv[1]);
    }

    if (timeout < 0) {
        writeToStdout("Invalid input detected. Ignoring timeout value.\n");
        timeout = 0;
    }

    while (1) {
        executeShell(timeout);
    }

    return 0;
}

/*
*redirects to write an output
*/
void redirectionsSTDOUTtoFile(char* tokenAfterOut){
    int new_stdout = open(tokenAfterOut, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if(new_stdout == -1){
        perror("Invalid standard output redirect: ");
        exit(EXIT_FAILURE);
    }
    dup2(new_stdout,STDOUT_FILENO);
    close(new_stdout);

}


/*
*redirects to read an input
*/
void redirectionsSTDINtoFile(char* tokenAfterIn){
    //close(STDIN_FILENO);
    int new_stdin = open(tokenAfterIn, O_RDONLY, 0644);
    if( new_stdin == -1){
        perror("Invalid standard input redirect: ");
        exit(EXIT_FAILURE);
    }
    dup2(new_stdin,STDIN_FILENO);
    close(new_stdin);
}

/* Sends SIGKILL signal to a child process.
 * Error checks for kill system call failure and exits program if
 * there is an error */
void killChildProcess() {
    if (kill(childPid, SIGKILL) == -1) {
        perror("Error in kill");
        exit(EXIT_FAILURE);
    }
}


/* Signal handler for SIGINT. Catches SIGINT signal (e.g. Ctrl + C) and
 * kills the child process if it exists and is executing. Does not
 * do anything to the parent process and its execution */
void sigintHandler(int sig) {
    if (childPid != 0) {
        killChildProcess();
    }
}


/* Registers SIGALRM and SIGINT handlers with corresponding functions.
 * Error checks for signal system call failure and exits program if
 * there is an error */
void registerSignalHandlers() {
    if (signal(SIGINT, sigintHandler) == SIG_ERR) {
        perror("Error in signal");
        exit(EXIT_FAILURE);
    }
}

/* Prints the shell prompt and waits for input from user.
 * Takes timeout as an argument and starts an alarm of that timeout period
 * if there is a valid command. It then creates a child process which
 * executes the command with its arguments.
 *
 * The parent process waits for the child. On unsuccessful completion,
 * it exits the shell. */
void executeShell(int timeout) {
    char **commandArray;
    right = (char *) calloc(2, sizeof(char*)); 
    strcpy(right, ">");
    left = (char *) calloc(2, sizeof(char*)); 
    strcpy(left, "<");
    int status;
    char minishell[] = "penn-sh> ";
    writeToStdout(minishell);

    commandArray = getCommandFromInput();

    if (commandArray[0] != NULL) {
        childPid = fork();

        if (childPid < 0) {
            free(commandArray);
            perror("Error in creating child process");
            exit(EXIT_FAILURE);           
        }

        if (childPid == 0) { //child process 
            char ** args = NULL;
            args = (char **) calloc(15, sizeof(char*)); 

            int i = 0;
            int m = 0;
            int redirOut=0; //redirect count for >
            int redirIn=0; //redirect count for <
            int totalRedirects = 0;
            while(commandArray[m] != NULL){
                if((strcmp(commandArray[m], right) == 0)){ 
                    //check for redirections count
                    if((redirOut > 0) || (totalRedirects > 1)){
                        perror("Invalid");
                        exit(EXIT_FAILURE); 
                    }
                    m++;
                    redirectionsSTDOUTtoFile(commandArray[m]);
                    m++;
                    redirOut++;
                    totalRedirects++;
                    
                }else if((strcmp(commandArray[m], left) == 0)){ 
                    //check for redirections count
                    if((redirIn > 0) || (totalRedirects > 1)){
                        perror("Invalid");
                        exit(EXIT_FAILURE); 
                    }
                    m++;
                    redirectionsSTDINtoFile(commandArray[m]);
                    m++;
                    redirIn++;
                    totalRedirects++;
                  
                }else{

                    args[i] = commandArray[m];
                    i++;
                    m++;
                }
                
                
            }
            args[i] = NULL;

            if (execvp(args[0], args) == -1) {
                free(right);
                free(left);
                //int i = 0;
                //while(commandArray[i] != NULL){
                    //free(commandArray[i]);
                //}
                //free(tok);
                free(commandArray);
                perror("Error in execvp");
                exit(EXIT_FAILURE);
            }

        } else { //parent process
            do {
                if (wait(&status) == -1) {
                    free(right);
                    free(left);
                    //int i = 0;
                    //while(commandArray[i] != NULL){
                    //free(commandArray[i]);
                    //}
                    //free(tok);
                    free(commandArray);
                    perror("Error in child process termination");
                    exit(EXIT_FAILURE);
                } 
                
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            childPid = 0;
        }
    }
    
    free(right);
    free(left);
    //int i = 0;
    //while(commandArray[i] != NULL){
        //free(commandArray[i]);
    //}
    //free(tok);
    free(commandArray);

    // must free each element of the array then free the array itself
    
}

/* Writes particular text to standard output */
void writeToStdout(char *text) {
    if (write(STDOUT_FILENO, text, strlen(text)) == -1) {
        perror("Error in write");
        exit(EXIT_FAILURE);
    }
}

/* Reads input from standard input till it reaches a new line character.
 * Checks if EOF (Ctrl + D) is being read and exits penn-shredder if that is the case
 * Otherwise, it checks for a valid input and adds the characters to an input buffer.
 *
 * From this input buffer, the first 1023 characters (if more than 1023) or the whole
 * buffer are assigned to command and returned. An \0 is appended to the command so
 * that it is null terminated */
char **getCommandFromInput() {
    TOKENIZER *tokenizer = NULL;
    int br;

    char* inputBuffer = NULL;
    inputBuffer = (char *) calloc(INPUT_SIZE+1, sizeof(char)); 
    char ** commandArray = NULL;
    commandArray = (char **) calloc(15, sizeof(char*)); 
    inputBuffer[INPUT_SIZE] = '\0';
   
    br = read(STDIN_FILENO, inputBuffer, INPUT_SIZE);

    inputBuffer[INPUT_SIZE-1] = '\0';	   /* ensure that string is always null-terminated */
    int i = 0;
    
        //EOF(Ctrl-D) without text
        if (br == 0){
            free(right);
            free(left);
            free(inputBuffer);
            free(tok);
            if (tokenizer != NULL){
                free_tokenizer( tokenizer ); /* free memory */
            }
            free(commandArray); 
            exit(EXIT_SUCCESS);  
        }
  
        inputBuffer[br-1] = '\0';   /* remove trailing \n */
        //printf( "Parsing '%s'\n", inputBuffer );
        tokenizer = init_tokenizer( inputBuffer );
        while( (tok = get_next_token( tokenizer )) != NULL ) {
            //printf( "Got token '%s'\n", tok );
            commandArray[i] = tok;
            i++;
        //free( tok );    /* free the token now that we're done with it */
        }
        free_tokenizer( tokenizer ); /* free memory */
        free(inputBuffer);


    return commandArray;
}