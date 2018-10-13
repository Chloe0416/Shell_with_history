// Shell starter file
// You may make any changes to any part of this file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define HISTORY_DEPTH 10

char history[HISTORY_DEPTH][COMMAND_LENGTH];
int totalCommands;
bool isSignal;

/**
 * Command Input and Processing
 */

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */
int tokenize_command(char *buff, char *tokens[])
{
	int token_count = 0;
	_Bool in_token = false;
	int num_chars = strnlen(buff, COMMAND_LENGTH);
	for (int i = 0; i < num_chars; i++) {
		switch (buff[i]) {
		// Handle token delimiters (ends):
		case ' ':
		case '\t':
		case '\n':
			buff[i] = '\0';
			in_token = false;
			break;

		// Handle other characters (may be start)
		default:
			if (!in_token) {
				tokens[token_count] = &buff[i];
				token_count++;
				in_token = true;
			}
		}
	}
	tokens[token_count] = NULL;
	return token_count;
}

int processCommand(char* buff, char* tokens[], _Bool* in_background){
	//Add command in buff to history excluding !!, !n, \0 and empty space.
	if (buff[0] != '!' && buff[0] != '\0' && buff[0] != ' ' && !isSignal) {
		if(totalCommands < HISTORY_DEPTH)
			strcpy(history[totalCommands], buff);
		else{
			for (int i = 1; i < HISTORY_DEPTH; i++)
				strcpy(history[i-1], history[i]);
			strcpy(history[HISTORY_DEPTH - 1], buff);
		}
		totalCommands++;
	}

	// Tokenize (saving original command string)
	int token_count = tokenize_command(buff, tokens);
	if (token_count == 0) {
		return 0;
	}

	// Extract if running in background:
	if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
		*in_background = true;
		tokens[token_count - 1] = 0;
	}

	return 1;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of tokens).
 * in_background: pointer to a boolean variable. Set to true if user entered
 *       an & as their last token; otherwise set to false.
 */
int read_command(char *buff, char *tokens[], _Bool *in_background)
{
	*in_background = false;

	// Read input
	int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);

	if ((length < 0) && (errno !=EINTR)) {
		perror("Unable to read command from keyboard. Terminating.\n");
		exit(-1);
	}

	// Null terminate and strip \n.
	buff[length] = '\0';
	if (buff[strlen(buff) - 1] == '\n') {
		buff[strlen(buff) - 1] = '\0';
	}
	
	return processCommand(buff, tokens, in_background);
}


void execute_command(char *tokens[], _Bool in_background)
{
	if (strcmp(tokens[0], "exit") == 0){
		exit(0);
	} 
	else if (strcmp(tokens[0], "pwd") == 0){
		char buff[100];
		if (!getcwd(buff,100))
			write(STDOUT_FILENO, strerror(errno), strlen(strerror(errno)));
		else
			write(STDOUT_FILENO, buff, strlen(buff));
	} 
	else if (strcmp(tokens[0], "cd") == 0) {
		if (chdir(tokens[1]) == -1)
			write(STDOUT_FILENO, "Invaild directory", strlen("Invaild directory"));
	} 
	else if (strcmp(tokens[0], "type") == 0) {
		if (strcmp(tokens[1], "exit") == 0 || strcmp(tokens[1], "pwd") == 0 
				|| strcmp(tokens[1], "cd") == 0 || strcmp(tokens[1], "type") == 0){
			write(STDOUT_FILENO, tokens[1], strlen(tokens[1]));
			write(STDOUT_FILENO, " is a shell300 builtin", strlen(" is a shell300 builtin"));
		} 
		else {
			write(STDOUT_FILENO, tokens[1], strlen(tokens[1]));
			write(STDOUT_FILENO, " is external to shell300", strlen(" is external to shell300"));
		}
	} 
	else if (strcmp(tokens[0], "\0") == 0 || isSignal){
		return;
	} 
	else if (strcmp(tokens[0], "history") == 0) {
		char numStr[50];
		int printSize = totalCommands;
		if (totalCommands > HISTORY_DEPTH)
			printSize = HISTORY_DEPTH;
		for (int i = 0; i < printSize; i++) {
			sprintf(numStr, "%d", totalCommands - printSize + i + 1);
			strcat(numStr, "\t"); //append the numStr with a tab space
			write(STDOUT_FILENO, numStr, strlen(numStr));
			write(STDOUT_FILENO, history[i], strlen(history[i]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
	}
	else if (strcmp(tokens[0], "!!") == 0) {
		int commandNum = HISTORY_DEPTH - 1;
		if (totalCommands < HISTORY_DEPTH)
			commandNum = totalCommands - 1;
		char command[COMMAND_LENGTH];
		strcpy(command, history[commandNum]);
		char *temp_tokens[NUM_TOKENS];
		_Bool is_in_background = false;

		write(STDOUT_FILENO, command, strlen(command));
		write(STDOUT_FILENO, "\n", strlen("\n"));
	
		processCommand(command, temp_tokens, &is_in_background);
		execute_command(temp_tokens, is_in_background);
	}
	else if (tokens[0][0] == '!') {
		int commandNum = atoi(&tokens[0][1]); //change char to int
		if (totalCommands-commandNum >= HISTORY_DEPTH || commandNum > totalCommands || commandNum != (int)commandNum)
			write(STDOUT_FILENO, "Unknown history command", strlen("Unknown history command"));
		else {
			char command[COMMAND_LENGTH];
			strcpy(command, history[HISTORY_DEPTH-1-(totalCommands-commandNum)]);
			char *temp_tokens[NUM_TOKENS];
			_Bool is_in_background = false;
	
			write(STDOUT_FILENO, command, strlen(command));
			write(STDOUT_FILENO, "\n", strlen("\n"));

			processCommand(command, temp_tokens, &is_in_background);
			execute_command(temp_tokens, is_in_background);
		}
	}
	else {
		pid_t pid = fork();
		if (pid == 0) {

			if (execvp(tokens[0], tokens) == -1) {
				write(STDOUT_FILENO, "Unknown command", strlen("Unknown command"));
				exit(0);
			}
		}
		else if (pid < 0){
			perror("fork Failed");
			exit(-1);
		}
		
		if(!in_background) {
			if (waitpid(pid, NULL, 0) == -1)
				perror("Error with waiting for child");
		}
	}
	// Cleanup any previously exited background child processes (The zombies)
	while (waitpid(-1, NULL, WNOHANG) > 0)
		; // do nothing.
	
}

void handle_SIGINT()
{
	isSignal = true;
	write(STDOUT_FILENO, "\n", strlen("\n"));
	if (totalCommands == 0)
		return;
	else{
		char numStr[50];
		int printSize = totalCommands;
		if (totalCommands > HISTORY_DEPTH)
			printSize = HISTORY_DEPTH;
		for (int i = 0; i < printSize; i++) {
			sprintf(numStr, "%d", totalCommands - printSize + i + 1);
			strcat(numStr, "\t"); //append the numStr with a tab space
			write(STDOUT_FILENO, numStr, strlen(numStr));
			write(STDOUT_FILENO, history[i], strlen(history[i]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
	}
}


/**
 * Main and Execute Commands
 */
int main(int argc, char* argv[])
{
	/* set up the signal handler */
	struct sigaction handler;
	handler.sa_handler = handle_SIGINT;
	handler.sa_flags = 0;
	sigemptyset(&handler.sa_mask);
	sigaction(SIGINT, &handler, NULL);

	char input_buffer[COMMAND_LENGTH];
	char *tokens[NUM_TOKENS];
	totalCommands = 0;
	isSignal = false;

	while (true) {
		// Get command
		// Use write because we need to use read() to work with
		// signals, and read() is incompatible with printf().
		char buff[100];
		if (!getcwd(buff,100))
			write(STDOUT_FILENO, strerror(errno), strlen(strerror(errno)));
		else
			write(STDOUT_FILENO, buff, strlen(buff));
		write(STDOUT_FILENO, "$ ", strlen("$ "));
		_Bool in_background = false;

		if(read_command(input_buffer, tokens, &in_background))
			execute_command(tokens, in_background);
		
		// DEBUG: Dump out arguments:
		for (int i = 0; tokens[i] != NULL; i++) {
			write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
			write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
		if (in_background) {
			write(STDOUT_FILENO, "Run in background.", strlen("Run in background."));
		}
		if (isSignal) 
			isSignal = false;

		/**
		 * Steps For Basic Shell:
		 * 1. Fork a child process
		 * 2. Child process invokes execvp() using results in token array.
		 * 3. If in_background is false, parent waits for
		 *    child to finish. Otherwise, parent loops back to
		 *    read_command() again immediately.
		 */	
	}
	return 0;
}

