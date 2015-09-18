/*
 * OS Assignment #1
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>

#define equal_str(str1, str2) strcmp(str1, str2) == 0
#define memset_zero(var) memset(var, 0, sizeof(var))

#define FD_STDIN 0
#define FD_STDOUT 1
#define FD_STDERR 2

#define DELAY_EXEC_IN_MICRO_SECOND 10 * 1000


/*
 * util - list
 */
struct list {
	void **elements;
	int size;
	int num_of_elements;
};

struct list* list_create() {
	struct list* li = malloc(sizeof(struct list*));
	li->size = 10;
	li->num_of_elements = 0;
	li->elements = malloc(sizeof(void*) * li->size);
	
	return li;
}

void list_resize(struct list* li, int new_size) {
	void **new_elements = malloc(sizeof(void*) * new_size);
	for (int i = 0; i < li->num_of_elements; i ++) {
		new_elements[i] = li->elements[i];
	}
	
	free(li->elements);
	li->elements = new_elements;
	li->size = new_size;
}

void list_push_back(struct list *li, void *elem) {
	if (li->num_of_elements == li->size) {
		list_resize(li, li->size * 2);
	}
	
	li->elements[li->num_of_elements++] = elem;
}

void* list_pop_back(struct list *li) {
	return li->elements[li->num_of_elements--];
}

void* list_remove_at(struct list *li, int index) {
	if (li->num_of_elements <= index) {
		return NULL;
	}
	
	void *elem = li->elements[index];
	for (int i = index; i < li->num_of_elements; i++) {
		li->elements[i] = li->elements[i + 1];
	}
	li->num_of_elements--;
	
	return elem;
}

void* list_last(struct list *li) {
	return li->elements[li->num_of_elements - 1];
}


struct command_line {
	char id[32];
	char action[16];
	char pipe_id[32];
	char command[1024];

	int pipe_filedes[2];
	int piped;
	struct command_line* target_command_line;

	pid_t pid;
};
typedef struct command_line command_line;


void signal_handler(int signo);
int run(const char*);
int process_line(const char*);
int process_command_line(command_line*);
command_line* find_command_line_by_id(const char*);
int test_format(const char*);
int id_validate(const char*);
struct command_line* tokenizing_line(const char*);
char* paxtok (char*, char*);
char* trim_whitespace(char*);
char** build_argv(command_line*);
int piping_if_exist_pipe_id(command_line*);

int line_number;
struct list* command_line_list;


int main (int argc, char **argv) {
	if (argc <= 1) {
		fprintf (stderr, "usage: %s config-file\n", argv[0]);
		return -1;
	}

	struct sigaction sa;
	sa.sa_flags = 0;

	// SIGINT
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL); 

	// SIGCHLD
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGCHLD, &sa, NULL); 

	// SIGTERM
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGTERM, &sa, NULL); 
	
	return run(argv[1]);;
}

void signal_handler(int signo) {
	if (signo == SIGINT) {
		exit(1);
	} else if (signo == SIGCHLD) {
		int state;
		pid_t pid_child = waitpid(-1, &state, WNOHANG);
		for (int i = 0; i < command_line_list->num_of_elements; i++) {
			command_line* cmd_line = (command_line*)command_line_list->elements[i];
			if (cmd_line->pid == pid_child && equal_str(cmd_line->action, "respawn")) {
				process_command_line(cmd_line);
				break;
			}
		}
	} else if (signo == SIGTERM) {
		exit(1);
	}
}

int run(const char* filename) {
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("failed to open file.\n");
		return -1;
	}
	
	command_line_list = list_create();
	
	char line[256];
	while (!feof(fp)) {
		memset_zero(line);
		fgets(line, sizeof(line), fp);
		line_number ++;
		
		// remove LF
		if (line[strlen(line) - 1] == '\n') {
			line[strlen(line) - 1] = 0;
		}
		
		// pass blank line
		if (strlen(line) == 0) {
			continue;
		}
		
		// skip comment line
		if (line[0] == '#') {
			continue;
		}
		
		if (process_line(line) < 0) {
			continue;
		}
	}

	// little sleep before end program
	usleep(1000000);
	
	return 0;
}

int process_line(const char* line) {
	if (test_format(line) < 0) {
		return -1;
	}
	
	struct command_line* cmd_line = tokenizing_line(line);

	if (id_validate(cmd_line->id) < 0) {
		fprintf(stderr, "invalid id \'%s\' in line %d, ignored\n", cmd_line->id, line_number);
		return -1;
	}

	command_line* target_command_line = find_command_line_by_id(cmd_line->id);
	if (target_command_line != NULL) {
		fprintf(stderr, "duplicate id \'%s\' in line %d, ignored\n", cmd_line->id, line_number);
		return -1;
	}
	
	if (strlen(cmd_line->action) == 0 ||
		(! equal_str (cmd_line->action, "once")
			&& ! equal_str (cmd_line->action, "wait")
			&& ! equal_str (cmd_line->action, "respawn"))) {
		fprintf(stderr, "invalid action \'%s\' in line %d, ignored\n", cmd_line->action, line_number);
		return -1;
	}

	if (strlen(cmd_line->pipe_id) > 0) {
		if (find_command_line_by_id(cmd_line->pipe_id) == NULL) {
			if (id_validate(cmd_line->pipe_id) < 0) {
				fprintf(stderr, "invalid pipe-id \'%s\' in line %d, ignored\n", cmd_line->pipe_id, line_number);
			} else {
				fprintf(stderr, "unknown pipe-id \'%s\' in line %d, ignored\n", cmd_line->pipe_id, line_number);
			}
			
			return -1;
		}
	}

	if (strlen(cmd_line->command) == 0) {
		fprintf(stderr, "empty command in line %d, ignored\n", line_number);
		return -1;
	}

	list_push_back(command_line_list, cmd_line);

	process_command_line(cmd_line);

	return 0;
}

int process_command_line(command_line* cmd_line) {
	if (piping_if_exist_pipe_id(cmd_line) < 0) {
		fprintf(stderr, "pipe not allowed for already piped tasks in line %d, ignored\n", line_number);
		return -1;
	}

	int child_pid = fork();
	if (child_pid == 0) {
		usleep(DELAY_EXEC_IN_MICRO_SECOND);

		if (cmd_line->piped) {
			dup2(cmd_line->target_command_line->pipe_filedes[0], FD_STDIN);
			dup2(cmd_line->target_command_line->pipe_filedes[1], FD_STDOUT);
		} else {
			dup2(cmd_line->pipe_filedes[0], FD_STDIN);
			dup2(cmd_line->pipe_filedes[1], FD_STDOUT);
		}

		char **argv = build_argv(cmd_line);
		execvp(argv[0], argv);
		fprintf(stderr, "failed to execute command \'%s\': No such file or directory\n", argv[0]);
	}

	cmd_line->pid = child_pid;

	if (equal_str(cmd_line->action, "wait")) {
		int status;
		waitpid(child_pid, &status, 0);
	}
}

command_line* find_command_line_by_id(const char *id) {
	for (int i = 0; i < command_line_list->num_of_elements; i++) {
		command_line* elem = (command_line*)command_line_list->elements[i];
		if (equal_str(elem->id, id)) {
			return command_line_list->elements[i];
		}
	}

	return NULL;
}

int test_format(const char* line) {
	// format test
	int colon_count = 0;
	for (int i = 0; i < strlen(line); i ++) {
		if (line[i] == ':') {
			colon_count++;
		}
	}
	
	if (colon_count != 3) {
		fprintf(stderr, "invalid format in line %d, ignored\n", line_number);
		return -1;
	}
	
	return 0;
}

int id_validate(const char* id) {
	int length = strlen(id);
	for (int i = 0; i < length; i++) {
		if ((id[i] >= 'a' && id[i] <= 'z') 
			|| (id[i] >= '0' && id[i] <= '9' )) {
		} else {
			return -1;
		}
	}

	return 0;
}

struct command_line* tokenizing_line(const char* line) {
	command_line* cmd_line = (command_line*)malloc(sizeof(command_line));
	memset_zero(cmd_line->id);
	memset_zero(cmd_line->action);
	memset_zero(cmd_line->pipe_id);
	memset_zero(cmd_line->command);
	cmd_line->piped = 0;
	
	char id[32];
	char action[16];
	char pipe_id[32];
	char command[1024];
	
	char *line_clone = strdup(line);
	strcpy(id, paxtok(line_clone, ":"));
	strcpy(action, paxtok(NULL, ":"));
	strcpy(pipe_id, paxtok(NULL, ":"));
	strcpy(command, paxtok(NULL, ":"));
	
	strcpy(cmd_line->id, trim_whitespace(id));
	strcpy(cmd_line->action, trim_whitespace(action));
	strcpy(cmd_line->pipe_id, trim_whitespace(pipe_id));
	strcpy(cmd_line->command, trim_whitespace(command));
	
	free(line_clone);

	return cmd_line;
}

char *paxtok (char *str, char *seps) {
	static char *tpos, *tkn, *pos = NULL;
	static char savech;
	
	// Specific actions for first and subsequent calls.
	
	if (str != NULL) {
		// First call, set pointer.
		
		pos = str;
		savech = 'x';
	} else {
		// Subsequent calls, check we've done first.
		
		if (pos == NULL)
			return NULL;
		
		// Then put character back and advance.
		
		while (*pos != '\0')
			pos++;
		*pos++ = savech;
	}
	
	// Detect previous end of string.
	
	if (savech == '\0')
		return NULL;
	
	// Now we have pos pointing to first character.
	// Find first separator or nul.
	
	tpos = pos;
	while (*tpos != '\0') {
		tkn = strchr (seps, *tpos);
		if (tkn != NULL)
			break;
		tpos++;
	}
	
	savech = *tpos;
	*tpos = '\0';
	
	return pos;
}

char *trim_whitespace(char *str) {
	char *end;
	
	// Trim leading space
	while(isspace(*str)) str++;
	
	if(*str == 0)  // All spaces?
		return str;
	
	// Trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace(*end)) end--;
	
	// Write new null terminator
	*(end+1) = 0;
	
	return str;
}

char** build_argv(command_line* cmd_line) {
	int length = strlen(cmd_line->command);
	int argv_count = 1;
	for (int i = 0; i < length; i++) {
		if (cmd_line->command[i] == ' ') {
			argv_count++;
		}
	}

	char command_clone[1024];
	strcpy(command_clone, cmd_line->command);

	char** argv = malloc(sizeof(char*) * (argv_count + 1));
	argv[0] = strdup(strtok(command_clone, " "));

	char *ptr = NULL;
	int i = 1;
	while((ptr = strtok(NULL, " ")) != NULL) {
		argv[i++] = strdup(ptr);	
	}

	argv[i] = NULL;

	return argv;
}

int piping_if_exist_pipe_id(command_line* cmd_line) {
	pipe(cmd_line->pipe_filedes);

	dup2(cmd_line->pipe_filedes[0], FD_STDIN);
	dup2(cmd_line->pipe_filedes[1], FD_STDOUT);

	command_line* target_command_line = NULL;
	if (strlen(cmd_line->pipe_id) != 0) {
		target_command_line = find_command_line_by_id(cmd_line->pipe_id);
		if (!target_command_line->piped) {
			target_command_line->piped = 1;
			cmd_line->piped = 1;
			cmd_line->target_command_line = target_command_line;

			dup2(target_command_line->pipe_filedes[0], cmd_line->pipe_filedes[1]);
			dup2(target_command_line->pipe_filedes[1], cmd_line->pipe_filedes[0]);
		} else {
			fprintf(stderr, "pipe not allowed for already piped tasks in line %d, ignored\n", line_number);
			return -1;
		}
	}

	return 0;
}