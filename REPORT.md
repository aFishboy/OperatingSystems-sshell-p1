# Introduction
In this sshell project there were many difficulties, but the major parts came  
down to parsing user commands, implementing system calls (fork, wait, exec),  
recursively implementing pipelines with multiple commands and debugging what had     
been implemented.  

# Workload
Yutian was responsible for the following three parts:
* Piping and Redirection
* Parse user input
* Execution of commands  

While Sam was responsible for:
* Builtin commands
* Extra Features
* Debugging for autograder

# Implementation  
## Early Decisions
From the beginning, we knew parsing would be at the core of the project so we  
decided to both try to implement phase 1 and 2 individually and take the best  
parts from each other to add to our final design. Sam tried to use the strtok()  
function to parse the command line using delimiters while Yutian parsed  
character by character. Once we had each implemented the skeleton of the sshell,  
we decided parsing by character was going to be more practical and allow for  
more flexibility moving forward.

### Parsing
For parsing user input, there are four features implemented:  
splitting commands, replacing environment variables in commands, filling  
command pointer arrays, and checking whether redirected files can be opened.  
Split command: By traversing the entire command string and identifying spaces  
and metacharacters in the string, each parameter is stored as a string  
separately. Finally, the original single command string is converted into a  
string array arg_list.  

Replace environment variables: The program will traverse the entire arg_list  
array, match the $ character and check whether the parameter is a valid  
environment variable. If not, return FAIL to end command. Otherwise, replace  
from the env_var array stored in the global variable and proceed to the next  
step.  

Filling command pointer arrays: The purpose is to make it more convenient to  
use execvp() and other system calls later. This function mainly stores the  
string pointer in the arg_list array into the argv array, separates the  
metacharacters from the command, and fills NULL in the corresponding position.  
If there’s redirection in the command, the program will check whether the  
target file can be opened and return FAIL if not.  

### Storage
We also needed to decide how this information would be stored so we looked into      
the tradeoffs between dynamically allocating memory vs a static design.    
Ultimately we decided a 2d array was the correct choice for our needs for a few    
reasons. First, we knew what the max size the array would have be so there was   
no worry about running out of space while running the program. There was also  
the fact that a linked list came with a bigger overhead to maintain. One issue  
we had with the array was when more memory was allocated than needed due to a    
short command such as "exit." We felt that memory waste was negligible and the  
array was the right choice.

There was also the issue of how to indicate the end of command and the beginning    
of a new one. Since we knew the we could only get three pipes and one  
redirection on the command line, we decided storing  meta characters in an array  
in their respective order would be best. For example if the command  
"echo test | wc > file" was given we could break the command line into 3  
commands and two meta characters. By storing this information we no longer  
needed to parse the original command line and could execute any of the  
specified commands.

## Phase 3-6
Once the project had been laid out and we had agreed upon how to store the data,  
we moved further into the project. Since the parsing was working and we were  
able to execute simple commands and arguments, Yutian began working on piping  
while Sam started the builtin commands.  

### Execution
For phase 1 and 2 execution of a command was simple, but a few design decisions  
were made in regards to how to implement the exec function. The two obvious  
answers were between execvp() and execlp(), but we weren't sure at first so we  
did some testing with each and found that execvp() was more flexible because we  
could pass it an array of strings that could be constructed dynamically at  
runtime because we didn't know the number of arguments.

Now that we needed to handle more diverse command lines we needed to distinguish  
the different type of  commands we needed to process. We decided there are three  
steps for how to execute a command. The first step is to check whether it is a  
built-in command. The second step is to check whether it is a single command.  
Lastly check whether it has multi commands. We implemented this using a few  
functions. The program calls the checkBuiltinCmd() function to check built-in  
commands. If there is a built-in command, execute the corresponding code and  
return SUCCESS to end the command. If there is no built-in command, check the  
array of meta characters to see whether multiple commands had been given. If  
meta_cnt is 0 we know it has to be a single command it is executed by calling  
the singleCmd() function. Otherwise, the multiCmd() function is called to  
execute multi commands.  

### Builtin
The builtin commands were relatively straightforward and required comparing  
the first argument of the command line array to builtin commands. This was  
accomplished with a simple string compare function. Once a command was  
recognized as a builtin command the proper actions could be taken to execute  
them.

### Piping and Redirection
Any command line with a pipe or redirection will be passed to the multiCmd    
function. The core of the multiCmd() function is to implement the pipeline    
through recursion. During the recursive process, the current and next command,  
current and next meta character, struct Argument arg, CompletedInfo, and the  
file descriptor generated by the pipeline in the previous layer, will pass by.  
Every time the function is executed, it will check whether the current parameter  
is the last parameter according to the parameter passed by the function. And use  
this as the termination condition of recursion.

The reason a recursive approach was taken to implement multi commands is because  
it was more natural to use recursion for this problem. When the process forks   
and the child executes, the parent needs to decide if they are the final  
command. If they are not the end of the command call multiCmd again and repeat.  
If it is not the last parameter, a pipeline will be created and the current  
command will be executed first. At this time, the previous command is connected  
to one end of the pipeline as a child process, and the parent process will not  
execute the command, but finds the next command through the NULL mark in the  
argv array, and then recurses to the next level. Thus, the communication of the  
process is implemented based on the file descriptor passed by from the previous  
level. If > is encountered, the program will consider this to be the last  
command and will stop the recursion.  

### Extra features
The final features added to the sshell were redirection of standard error and  
the addition of environment variables.

The first step to adding the combined redirection was to modify the parser to  
recognize the new meta characters "|&" and ">&". We had already accounted for  
meta characters that were not only chars, so our array of meta characters didn't  
need any modification. Then the piping and redirection funtions needed to be    
modified to recognize the new meta characters. If a combined redirection was    
given, standard out and error would be redirected to the next command or file.

Environment variables also required modifying the parser. As specified in the  
documentation, if a variable is not set it will be initialized to an empty  
string. We did this through the use of an array of size 26 to hold a value for  
each possible variable (a-z.) If the set keyword was found, it was treated as a  
pseudo built in command. It would then set the value of the following argument  
in the environment variable array equal to the third argument. This variable  
also need to be checked to be a valid variable name. Now while parsing, if $ was  
found and had a valid variable name it then set the argv array to the value of  
the variable.

## Rewriting the Code
Around phase 3 we realized things were getting unorganized and the documentation  
recommended the implementation of structs and other data structures. We decided  
that taking what had implemented so far, but using structs to organize data and  
splitting up tasks into individual functions would help us as time went on. We    
decided on two struts. One to hold the data of the command line such as the  
arguments and their count as well as the meta characters and their count. Our  
other struct was responsible for the completion info. This was a major help  
because after every command is executed the command line needs to print the  
status of what had been executed.  

## Debugging and Testing
Debugging and testing was a program long endeavor. Our philosophy was to try and  
implement a feature quickly and once it was working mostly then try to revise.  
For example, parsing at the beginning was relatively simple and just required  
splitting arguments between spaces. Eventually, we needed to catch the case  
when meta characters were given with no spaces and so on. Our first goal was to  
compare our output with commands seen in the documentation once that was done we  
tried to come up with edge cases to test our program on and then compare them to  
the sshell_ref.

## Sources
Book:  
Computer Systems：A Programmer’s Perspective

wait()/waitpid()：  
http://t.csdn.cn/AUP6u  
https://linux.die.net/man/2/waitpid  

exec():  
http://t.csdn.cn/L17Bt  
https://www.geeksforgeeks.org/exec-family-of-functions-in-c/#  

open():    
http://t.csdn.cn/3320z  
https://man7.org/linux/man-pages/man2/open.2.html  

dup2():  
https://www.geeksforgeeks.org/dup-dup2-linux-system-call/

strcpy():  
https://www.tutorialspoint.com/c_standard_library/c_function_strcpy.htm  

structs:  
https://www.w3schools.com/c/c_structs.php  

tester.sh removing quotes in arguments:    
[https://unix.stackexchange.com/why-is-echo-ignoring-my-quote-characters](https://unix.stackexchange.com/questions/24647/why-is-echo-ignoring-my-quote-characters)   
https://stackoverflow.com/questions/12886208/c-unix-shell-execvp-echoing-quotes  