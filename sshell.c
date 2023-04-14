#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
//next time implement cd. 
#define CMDLINE_MAX 512
#define MAX_ARGS 16

struct cmdLineData{
    char *cmdData[MAX_ARGS+1];
    char symbolAfterCmd;
    struct cmdLineData *next;
};

struct cmdLineData *head = NULL;
struct cmdLineData *tail = NULL;

void listAppend(){
	struct cmdLineData* newNode = malloc(sizeof(struct cmdLineData));
	newNode->next = NULL;
	if (head == NULL){
		head = newNode;
		tail = newNode;
	}
	else{
		tail->next = newNode;
		tail = newNode;
	}
}

void print_list() {
    struct cmdLineData* current = head;
    while (current != NULL) {
        for (int i = 0; i < MAX_ARGS; i++) {
            if (current->cmdData[i] != NULL) {
                printf("%s -> ", current->cmdData[i]);
            }
        }
        printf("\n %c\n", current->symbolAfterCmd);
        current = current->next;
    }
}

void deallocate(struct cmdLineData* head){
	struct cmdLineData* currentNode;
	while (head != NULL){
		currentNode = head;
		head = head->next;
		free(currentNode);
		printf("deallocated\n");
	}
}


/*Function: getCommandArgs
returns an int of the amount of arguements given on command line

Arguements: char *stringToTok is a pointer to the cmd
line string from fgets in main which we use to get tokens

char **args is a pointer to the arglist pointer in main, 
we use it to storethe args in an array of string used by execvp*/
int getCommandArgs(char *stringToTok){
        int argc = 0;
        const char* delimiter = " "; 
        char buffer[32];
        char *token = strtok(stringToTok, delimiter); // tokenize string from space delimiter 
        //loop while there are still valid tokens and are in bounds of MAX_ARGS
        while (token != NULL && argc < MAX_ARGS){ 
                char *delim_pos = strchr(token, '|');
                if (!delim_pos) delim_pos = strchr(token, '>');

                if (delim_pos) {
                        // if so, copy the part of the token before the delimiter
                        strncpy(buffer, token, delim_pos - token);
                        buffer[delim_pos - token] = '\0';
                        if (buffer[0] != '\0'){
                                tail->cmdData[argc++] = buffer;   
                        }
                        tail->symbolAfterCmd = token[delim_pos - token];
                        listAppend();
                        argc = 0;
                } else {
                        // otherwise, treat the token as a regular token
                        tail->cmdData[argc++] = token;   //may need to change to: args[argc++] = strdup(token); 
                        tail->symbolAfterCmd = '\0';

                }
                //increment argc while also setting args to the token    
                
                token = strtok(NULL, delimiter);   //get next token  
        }
        tail->cmdData[argc] = NULL;      //set final string in array to null
        print_list();
        return argc;

}

int main(void){
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


                listAppend();
                
                argc = getCommandArgs(cmd);    // call func to tokenize command line

                if (argc < 0){  //if command line is empty
                        continue;       //go to next shell command
                }

                if (!fork()) { /* Fork off child process */
                        execvp(head->cmdData[0], head->cmdData); /* Execute command */
                        perror("execv"); /* Coming back here is an error */
                        exit(1);
                } else {
                /* Parent */
                waitpid(-1, &retval, 0); /* Wait for child to exit */
                deallocate(head);
                head = NULL;
                tail = NULL;
                }
        }
        
        return EXIT_SUCCESS;
}
