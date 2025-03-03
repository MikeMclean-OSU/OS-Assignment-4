#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define INPUT_LENGTH 2048
#define MAX_ARGS 512

int fg_process = -1;
int sigtstp_flag = 0; //0 = not foregorund only mode, 1 = foreground only mode


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

void handle_SIGINT(int signo){
	if (fg_process > -1){
		kill(fg_process, SIGTERM);
		fg_process = -1;
	}
}

void handle_SIGTSTP(int signo){
	char* exit_message = "\nExiting foreground-only mode\n";
	char* enter_message = "\nEntering foreground-only mode (& is now ignored)\n";
	
	if(sigtstp_flag){
		sigtstp_flag = 0;
		write(STDOUT_FILENO, exit_message, 29);
	}else{
		sigtstp_flag = 1;
		write(STDOUT_FILENO, enter_message, 49);
	}
}

int main()
{
	int status;
	pid_t id;
	int childStatus;
	struct command_line *curr_command;
	
	//Signal handler for SIGTERM
	struct sigaction SIGTERM_action = {0};
	SIGTERM_action.sa_handler = SIG_DFL;
	sigfillset(&SIGTERM_action.sa_mask);
	SIGTERM_action.sa_flags = 0;
	sigaction(SIGTERM, &SIGTERM_action, NULL);

	//Signal handler for SIGINT
	struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = handle_SIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);
	
	//Signal handler for SIGTSTP
	struct sigaction SIGTSTP_action = {0};
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);


	while(true)
	{
		id = waitpid(-1, &status, WNOHANG);
		while (id > 0){
			if (WIFSIGNALED(status)){
				printf("background pid %d is done: terminated by signal %d\n", id, WTERMSIG(status));
				fflush(stdout);
			}else{
				printf("background pid %d is done: exit value %d\n", id, WEXITSTATUS(status));
				fflush(stdout);
			}
			id = waitpid(-1, &status, WNOHANG);
		}

		curr_command = parse_input();
		char *token = curr_command->argv[0];

		if (!token || token[0] == '#'){
			continue;
		} 
		
		if (!strcmp(token, "exit")){
			kill(0, SIGTERM);
			break;
		
		}else if (!strcmp(token, "cd")){
			if (!curr_command->argv[1]){
				chdir(getenv("HOME"));
				continue;
			}
			
			if (chdir(curr_command->argv[1]) != 0){
				perror("Directory Change Failed");
				continue;
			} else{
				chdir(curr_command->argv[1]);
				continue;
			}
		
		}else if (!strcmp(token, "status")){
			if(WIFSIGNALED(childStatus)){
				printf("terminated by signal %d\n", WTERMSIG(childStatus));
				fflush(stdout);
			}else{
				printf("exit value %d\n", WEXITSTATUS(childStatus));
				fflush(stdout);
			}
			continue;
		}

		pid_t childpid = fork();
		
		if(childpid == -1){
			perror("fork() failed!");
			exit(1);
		}else if(childpid == 0){
			//child case
			if (!curr_command->is_bg){
				fg_process = getpid();
			}
			//Redirect output if file is given
			if (curr_command->output_file){
				int fd_out = open(curr_command->output_file, O_WRONLY | O_CREAT, 0640);
				int redirect_out = dup2(fd_out, 1);
			}
			//Redirect input if file is given
			if (curr_command->input_file){
				int fd_in = open(curr_command->input_file, O_RDONLY, 0640);
				if (fd_in == -1){
					perror("Error");
					exit(1);
				}
				int redirect_in = dup2(fd_in, 0);
			}
			//Redirect output of background process if no file specified
			if (curr_command->is_bg && !curr_command->output_file && !sigtstp_flag){
				int fd_out = open("/dev/null", O_WRONLY);
				int redirect_out = dup2(fd_out, 1); 
			}
			//Redirect input of background process if no file specified
			if (curr_command->is_bg && !curr_command->input_file && !sigtstp_flag){
				int fd_in = open("/dev/null", O_RDONLY);
				int redirect_out = dup2(fd_in, 0); 
			}
			
			
			execvp(token, curr_command->argv);
			perror("Error");
			exit(2);
			
		}else{
			//Parent case
			if (curr_command->is_bg && !sigtstp_flag){
				printf("Background pid is %d\n", childpid);
				fflush(stdout);
				childpid = waitpid(childpid, &childStatus, WNOHANG);
			}else{
				childpid = waitpid(childpid, &childStatus, 0);
				if (WIFSIGNALED(childStatus)){
					printf("terminated by signal %d\n", WTERMSIG(childStatus));
					fflush(stdout);
				}
			}
		}	
		
		free(curr_command->argv);
		if (curr_command->input_file){
			free(curr_command->input_file);
		}
		if (curr_command->output_file){
			free(curr_command->output_file);
		}
	}
	return EXIT_SUCCESS;
}