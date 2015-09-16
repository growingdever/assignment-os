/*
 * OS Assignment #1
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>

#define equal_str(str1, str2) strcmp(str1, str2) == 0
#define memset_zero(var) memset(var, 0, sizeof(var))


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
};
typedef struct command_line command_line;


int run(const char*);
int process_line(const char*);
int test_format(const char*);
struct command_line* tokenizing_line(const char*);
char* paxtok (char*, char*);
char* trim_whitespace(char*);

int line_number;
struct list* command_line_list;


int main (int argc, char **argv) {
	if (argc <= 1) {
		fprintf (stderr, "usage: %s config-file\n", argv[0]);
		return -1;
	}
	
	return run(argv[1]);;
}

int run(const char* filename) {
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("failed to open file.\n");
		return -1;
	}
	
	command_line_list = list_create();
	
	char line[256];
	while (1) {
		memset_zero(line);
		fgets(line, sizeof(line), fp);
		line_number ++;
		
		// remove LF
		line[strlen(line) - 1] = 0;
		if (strlen(line) == 0) {
			continue;
		}
		
		// skip comment line
		if (line[0] == '#') {
			continue;
		}
		
		if (process_line(line) < 0) {
			return -1;
		}
	}
	
	return 0;
}

int process_line(const char* line) {
	if (test_format(line) < 0) {
		return -1;
	}
	
	struct command_line* cmd_line = tokenizing_line(line);
	list_push_back(command_line_list, cmd_line);
	
	printf("%s : %s : %s : %s\n", cmd_line->id, cmd_line->action, cmd_line->pipe_id, cmd_line->command);
	
	if (strlen(cmd_line->action) == 0 ||
		(! equal_str (cmd_line->action, "once")
			&& ! equal_str (cmd_line->action, "wait")
			&& ! equal_str (cmd_line->action, "respawn"))) {
		fprintf(stderr, "invalid action \'%s\' in line %d, ignored", cmd_line->action, line_number);
		return -1;
	}
	
	// int is_background = 0;
	// for (int i = strlen(command) - 1; i >= 0; i --) {
	// 	if (command[i] == '&') {
	// 		is_background = 1;
	// 		break;
	// 	}
	
	// 	if (command[i] != ' ') {
	// 		break;
	// 	}
	// }
	
	// if (fork() == 0) {
	// 	// child process run codes inside here
	// 	int ret = execve(argv[0], &argv[0], NULL);
	// 	printf("errno(%d)\n", errno);
	// 	return -1;
	// }
	
	// // if not background mode, wait for child process
	// if (!is_background) {
	// 	int status;
	// 	pid_t ret = wait(&status);
	// }
	
	return 0;
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

struct command_line* tokenizing_line(const char* line) {
	command_line* cmd_line = (command_line*)malloc(sizeof(command_line));
	memset_zero(cmd_line->id);
	memset_zero(cmd_line->action);
	memset_zero(cmd_line->pipe_id);
	memset_zero(cmd_line->command);
	
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