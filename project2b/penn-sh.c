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

pid_t childPid1 = 0;

pid_t childPid2 = 0;

char *tok = NULL;

void executeShell(int timeout);

void writeToStdout(char *text);

void alarmHandler(int sig);

void sigintHandler(int sig);

char **getCommandFromInput();

void registerSignalHandlers();

void killChildProcess();

void redirectionsSTDOUTtoFile(char* tokenAfterOut);

void redirectionsSTDINtoFile(char* tokenAfterIn);

char **createArrayOfTokensBeforePipe(char** commandArray);

char **createArrayOfTokensAfterPipe(char** commandArray);

char **redirectionsPipeWriterProcess(char** args1, char* out, char*in, char** commandArray1);

char **redirectionsPipeReaderProcess(char** args2, char* out, char*in, char** commandArray2);

void noPipe(int status, char* out, char* in, char** commandArray); //breaks out to project2a if there isn't a pipe in commandArray

void yesPipe(int status, char* out, char* in, char** commandArray); //breaks out to project2b if there is a pipe in commandArray

void freeArray(char** array); //frees each item of the commandArray

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
* Frees all items of a given array
*/
void freeArray(char** array){
    int i = 0;

    while(array[i] != NULL){
        free(array[i]);
        i++;
    }

    free(array);
}

/*
*Redirects to write an output
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
*Redirects to read an input
*/
void redirectionsSTDINtoFile(char* tokenAfterIn){
    int new_stdin = open(tokenAfterIn, O_RDONLY, 0644);
    if( new_stdin == -1){
        perror("Invalid standard input redirect: ");
        exit(EXIT_FAILURE);
    }
    dup2(new_stdin,STDIN_FILENO);
    close(new_stdin);
}

/*
* Creates an array of tokens before the pipe
*/
char **createArrayOfTokensBeforePipe(char** commandArray){
    char** before = (char **) calloc(15, sizeof(char*)); 
    char *pipeSym = (char *) calloc(2, sizeof(char*)); //pipe symbol
    strcpy(pipeSym, "|");

    int i = 0;
    if(commandArray!= NULL){
        while(commandArray[i] != NULL){
            if((strcmp(commandArray[i], pipeSym) == 0)){
                break;
            }
            before[i] = commandArray[i];
            i++;       
        }
    }
    free(pipeSym);
    //freeArray(commandArray);
    return before;
}

/*
* Creates an array of tokens after the pipe
*/
char **createArrayOfTokensAfterPipe(char** commandArray){
    char** after = (char **) calloc(15, sizeof(char*)); 
    char *pipeSym = (char *) calloc(2, sizeof(char*)); //pipe symbol
    strcpy(pipeSym, "|");

    int i = 0;
    int j = 0;
    if(commandArray != NULL){
        while(commandArray[i] != NULL){
            if((strcmp(commandArray[i], pipeSym) == 0)){
                i++;
                break;
            }  
            i++;      
        }

        while(commandArray[i] != NULL){
            after[j] = commandArray[i];
            i++;
            j++;
        }  
    }

     

    free(pipeSym);
    //freeArray(commandArray);
    return after;
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
    char* out = (char *) calloc(2, sizeof(char*)); //redirect out (right arrow)
    strcpy(out, ">");
    char*  in = (char *) calloc(2, sizeof(char*)); //redirect in (left arrow)
    strcpy(in, "<");
    char *pipeSym = (char *) calloc(2, sizeof(char*)); //pipe symbol
    strcpy(pipeSym, "|");
    int i = 0;
    int flag = 0;
    int status = 0;
    char minishell[] = "penn-sh> ";
    writeToStdout(minishell);

    char **commandArray = getCommandFromInput();

    if (commandArray[0] != NULL) {

        while (commandArray[i] != NULL) {
            if(strcmp(commandArray[i], pipeSym) == 0){             
               flag++;
            }
            i++; 
        }

        if (flag == 0){
            noPipe(status, out, in, commandArray);
        } else if (flag == 1) {
            yesPipe(status, out, in, commandArray);
        } else{
            free(out);
            free(in);
            free(pipeSym);
            freeArray(commandArray);
            perror("Invalid! Too many pipes");
            exit(EXIT_FAILURE); 
        }
        
    } else {
        free(out);
        free(in);
        free(pipeSym);
        free(commandArray);
    }

}

/* Writes particular text to standard output */
void writeToStdout(char *text) {
    if (write(STDOUT_FILENO, text, strlen(text)) == -1) {
        perror("Error in write");
        exit(EXIT_FAILURE);
    }
}

/*
* Writer process for the pipe
*/
char** redirectionsPipeWriterProcess(char** args1, char* out, char*in, char** commandArray1){
    int i = 0;
    int m = 0;
    int totalRedirects = 0;
    while(commandArray1[m] != NULL){
        if((strcmp(commandArray1[m], out) == 0)){ 
                perror("Invalid! Can't redirect out then pipe");
                exit(EXIT_FAILURE);                           
        }else if((strcmp(commandArray1[m], in) == 0)){          
            if(totalRedirects > 1){ //invalid if there are more than 1 redirects
                perror("Invalid! Too many redirect ins");
                exit(EXIT_FAILURE); 
            }
            m++;
            redirectionsSTDINtoFile(commandArray1[m]);
            m++;
            totalRedirects++;                        
        }else{
                args1[i] = commandArray1[m];
                i++;
                m++;
        }
    }
    args1[i] = NULL;

    return args1;
}

/*
* Reader process for the pipe
*/
char** redirectionsPipeReaderProcess(char** args2, char* out, char*in, char** commandArray2){   
    int i = 0;
    int m = 0;
    int totalRedirects = 0;
    while(commandArray2[m] != NULL){
        if((strcmp(commandArray2[m], out) == 0)){ 
            if(totalRedirects > 1){ //invalid if there are more than 1 redirects
                perror("Invalid! Too many redirect outs");
                exit(EXIT_FAILURE); 
            }
            m++;
            redirectionsSTDOUTtoFile(commandArray2[m]);
            m++;
            totalRedirects++;           
        } else if((strcmp(commandArray2[m], in) == 0)){ 
            perror("Invalid! Can't redirect in after a pipe");
            exit(EXIT_FAILURE);           
        } else{
            args2[i] = commandArray2[m];
            i++;
            m++;
        }        
    }
    args2[i] = NULL;

    return args2;
}

/*This code is executed if the user didn't use a pipe symbol
* Basically it's project2a
*/
void noPipe(int status, char* out, char* in, char **commandArray){
    childPid = fork();

    if (childPid < 0) {
        free(out);
        free(in);
        freeArray(commandArray);
        perror("Error in creating child process");
        exit(EXIT_FAILURE);           
    }

    if (childPid == 0) { //child process 
        char ** args = (char **) calloc(15, sizeof(char*));
        int i = 0;
        int m = 0;
        int redirOut=0; //redirect count for >
        int redirIn=0; //redirect count for <
        int totalRedirects = 0;
        while(commandArray[m] != NULL){
            if((strcmp(commandArray[m], out) == 0)){ 
                //check for redirections count
                if((redirOut > 0) || (totalRedirects > 1)){
                    free(out);
                    free(in);
                    freeArray(commandArray);
                    perror("Invalid");
                    exit(EXIT_FAILURE); 
                }
                m++;
                redirectionsSTDOUTtoFile(commandArray[m]);
                m++;
                redirOut++;
                totalRedirects++;
                
            }else if((strcmp(commandArray[m], in) == 0)){ 
                //check for redirections count
                if((redirIn > 0) || (totalRedirects > 1)){
                    free(out);
                    free(in);
                    freeArray(commandArray);
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
            free(out);
            free(in);
            freeArray(args);
            freeArray(commandArray);           
            perror("Error in execvp");
            exit(EXIT_FAILURE);
        }

    } 

        do {
            if (wait(&status) == -1) {
                free(out);
                free(in);
                free(commandArray);
                perror("Error in child process termination");
                exit(EXIT_FAILURE);
            } 
            
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        childPid = 0;
    

    free(out);
    free(in);
    //freeArray(commandArray);
}

/*This code is executed if the user enters a pipe symbol
* This is the core of project2b
*/
void yesPipe(int status, char* out, char* in, char **commandArray){
//project2b portion
    char** commandArray1 = (char **) calloc(15, sizeof(char*)); 
    commandArray1 = createArrayOfTokensBeforePipe(commandArray);
    char** commandArray2 = (char **) calloc(15, sizeof(char*)); 
    commandArray2 = createArrayOfTokensAfterPipe(commandArray);
    int fd[2];
    pipe(fd);

    childPid1 = fork();

    if (childPid1 < 0) {
        free(out);
        free(in);
        freeArray(commandArray1);
        freeArray(commandArray2);
        perror("Error in creating child process");
        exit(EXIT_FAILURE);           
    }
    // writer process for pipe
    if (childPid1 == 0) { //child1 process 
        char ** args1 = (char **) calloc(15, sizeof(char*));
        args1 = redirectionsPipeWriterProcess(args1, out, in, commandArray1);
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        
        if (execvp(args1[0], args1) == -1) {
            free(out);
            free(in);
            freeArray(args1);
            freeArray(commandArray1);
            freeArray(commandArray2);
            perror("Error in execvp");
            exit(EXIT_FAILURE);
        }  
    }   

    childPid2 = fork();
    
    if (childPid2 < 0) {
        free(out);
        free(in);
        freeArray(commandArray1);
        freeArray(commandArray2);
        perror("Error in creating child process");
        exit(EXIT_FAILURE);           
    }

    // reader process for pipe
    if (childPid2 == 0){ //child2 process
        char ** args2 = (char **) calloc(15, sizeof(char*));
        args2 = redirectionsPipeReaderProcess(args2, out, in, commandArray2);
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        
        if (execvp(args2[0], args2) == -1) {
            free(out);
            free(in);
            freeArray(args2);
            freeArray(commandArray1);
            freeArray(commandArray2);
            perror("Error in execvp");
            exit(EXIT_FAILURE);
        }

    }
    int status2;
    close(fd[0]);
    close(fd[1]);

    //for (int i = 0; i < 2; i++){
        do {
            if ((waitpid(childPid1, &status, 0) == -1) || (waitpid(childPid2, &status2, 0) == -1)) {
                free(out);
                free(in);
                freeArray(commandArray1);
                freeArray(commandArray2);
                perror("Error in children process termination");
                exit(EXIT_FAILURE);
            } 


        } while (!WIFEXITED(status) && !WIFSIGNALED(status) && !WIFEXITED(status2) && !WIFSIGNALED(status2));

    //}
    childPid1 = 0;
    childPid2 = 0;
    freeArray(commandArray1);
    freeArray(commandArray2);
    
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
    char* inputBuffer = (char *) calloc(INPUT_SIZE+1, sizeof(char)); 
    char ** commandArray = (char **) calloc(15, sizeof(char*)); 
    inputBuffer[INPUT_SIZE] = '\0';
   
    br = read(STDIN_FILENO, inputBuffer, INPUT_SIZE);

    inputBuffer[INPUT_SIZE-1] = '\0';	   /* ensure that string is always null-terminated */
    int i = 0;
    
    //EOF(Ctrl-D) without text
    if (br == 0){
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
    }
    free_tokenizer( tokenizer ); /* free memory */
    free(inputBuffer);


    return commandArray;
}