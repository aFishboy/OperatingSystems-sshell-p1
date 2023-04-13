#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
//next time implement cd. 
#define CMDLINE_MAX 512
#define MAX_ARGS 16

/*temporary need to fix
made this because he recommends making a data structure / struct to hold all the data needed from command line so it does
not need to be parsed again "The result of parsing the command line should be the instance of a data structure which contains 
all the information necessary to launch the specified command (so that the original command line does not have to be parsed again)."
Thought process was that worst case there will be 3 pipes given (because that is the max) and each command has 16 args
so while parsing if a pipe is encountered store the first command in cmd1 and then keep parsing to store next one. There probably is a better way 
to implemnent so it isnt so wastefull but not sure yet. */
struct cmdLineData{
    char *cmd1[MAX_ARGS+1];
    char *cmd2[MAX_ARGS+1];
    char *cmd3[MAX_ARGS+1];
    char *cmd4[MAX_ARGS+1];
};

/*Function: getCommandArgs
returns an int of the amount of arguements given on command line

Arguements: char *stringToTok is a pointer to the cmd
line string from fgets in main which we use to get tokens

char **args is a pointer to the arglist pointer in main, 
we use it to storethe args in an array of string used by execvp*/
int getCommandArgs(char *stringToTok, struct cmdLineData *currentCommand){
        int argc = 0;
        char *token = strtok(stringToTok, " "); // tokenize string from space delimiter 
        //loop while there are still valid tokens and are in bounds of MAX_ARGS
        while (token != NULL && argc < MAX_ARGS){ 
                //increment argc while also setting args to the token      
                currentCommand->cmd1[argc++] = token;   //may need to change to: args[argc++] = strdup(token); 
                token = strtok(NULL, " ");   //get next token  
        }
        currentCommand->cmd1[argc] = NULL;      //set final string in array to null
        return argc;

}

int main(void)
{
        /*command line will only ever have 512 chars so create array to hold the whole thing*/  
        char cmd[CMDLINE_MAX];  
        
        int argc;


        while (1) {
                char *nl;
                int retval;
                 

                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                struct cmdLineData object1;

                argc = getCommandArgs(cmd, &object1);    // call func to tokenize command line

                if (argc < 0){  //if command line is empty
                        continue;       //go to next shell command
                }

                if (!fork()) { /* Fork off child process */
                        execvp(object1.cmd1[0], object1.cmd1); /* Execute command */
                        perror("execv"); /* Coming back here is an error */
                        exit(1);
                } else {
                /* Parent */
                waitpid(-1, &retval, 0); /* Wait for child to exit */

                // /* Regular command */
                // retval = system(cmd);
                // fprintf(stderr, "Return status value for '%s': %d\n",
                //         cmd, retval);
                }
        }

        return EXIT_SUCCESS;
}
