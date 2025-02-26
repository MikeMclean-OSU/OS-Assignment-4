#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define INPUT_LENGTH 2048
#define MAX_ARGS 512


struct command_line
{
	char *argv[MAX_ARGS + 1];
	int argc;
	char *input_file;
	char *output_file;
	bool is_bg;
};


struct command_line *parse_input()
{
	char input[INPUT_LENGTH];
	struct command_line *curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));

	// Get input
	printf(": ");
	fflush(stdout);
	fgets(input, INPUT_LENGTH, stdin);

	// Tokenize the input
	char *token = strtok(input, " \n");
	while(token){
		if(!strcmp(token,"<")){
			curr_command->input_file = strdup(strtok(NULL," \n"));
		} else if(!strcmp(token,">")){
			curr_command->output_file = strdup(strtok(NULL," \n"));
		} else if(!strcmp(token,"&")){
			curr_command->is_bg = true;
		} else{
			curr_command->argv[curr_command->argc++] = strdup(token);
		}
		token=strtok(NULL," \n");
	}
	return curr_command;
}

int main()
{

	struct command_line *curr_command;
	struct sigaction SIGTERM_action = {0};

	SIGTERM_action.sa_handler = SIG_DFL;
	sigfillset(&SIGTERM_action.sa_mask);
	sigaction(SIGTERM, &SIGTERM_action, NULL);

	while(true)
	{
		curr_command = parse_input();
		char *token = curr_command->argv[0];
		if (!token || token[0] == '#'){
			continue;
			
		} else if (!strcmp(token, "exit")){
			kill(0, SIGTERM);
			break;
		
		} else if (!strcmp(token, "cd")){
			if (!curr_command->argv[1]){
				printf("%s", getenv("PATH"));
				//chdir(getenv("PATH"));
				continue;
			}
			
			// if (chdir(curr_command->argv[1]) != 0){
			// 	perror("Directory Change Failed");
			// 	continue;
			// } else{
				
			// }
		
		} else if (!strcmp(token, "status")){
			printf("status command entered\n");
		
		
		}

	}
	return EXIT_SUCCESS;
}