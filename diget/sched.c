/*
 * OS Assignment #2
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>


#define PRINT_ERROR(f_, ...) fprintf(stderr, (f_), __VA_ARGS__)
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
    struct list* li = malloc(sizeof(struct list));
    li->size = 10;
    li->num_of_elements = 0;
    li->elements = malloc(sizeof(void*) * li->size);
    
    return li;
}

void list_delete(struct list* li) {
    free(li->elements);
    free(li);
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

void list_clear(struct list* li) {
    for (int i = 0; i < li->num_of_elements; i++) {
        free(li->elements[i]);
    }
    li->num_of_elements = 0;
}

void* list_last(struct list *li) {
    return li->elements[li->num_of_elements - 1];
}


typedef struct _tag_command_line {
    char id[16];
    int arrive_time;
    int service_time;
    int priority;
} command_line;


int run(const char*);
void fgets_except_linefeed(char*, int, FILE*);
command_line* parse_line(const char*);
int validate_line(const command_line*);
int command_line_sort_comp(const void*, const void*);
int start();
int algorithm_sjf();
int algorithm_srt();
int algorithm_rr();
int algorithm_pr();


int line_number = 0;
struct list* command_line_list;


int main(int argc, char **argv) {
    if (argc <= 1) {
        PRINT_ERROR("usage: %s input-file\n", argv[0]);
        return -1;
    }
    
    return run(argv[1]);
}


int run(const char* filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        PRINT_ERROR("failed to load input file %s\n", filename);
        return -1;
    }
    
    command_line_list = list_create();
    
    char line[256];
    while (!feof(fp)) {
        memset_zero(line);
        fgets_except_linefeed(line, sizeof(line), fp);
        line_number ++;
        
        command_line *cmd_line = parse_line(line);
        if (cmd_line == NULL) {
            continue;
        }
        
        if (!validate_line(cmd_line)) {
            list_push_back(command_line_list, cmd_line);
        }
    }
    
    fclose(fp);
    
    return start();
}


void fgets_except_linefeed(char* line, int size, FILE *fp) {
    fgets(line, size, fp);
    
    unsigned long len = strlen(line);
    if (len > 0) {
        if (*(line + len - 1) == '\n') {
            *(line + len - 1) = 0;
        }
    }
}


command_line* parse_line(const char* line) {
    if (strlen(line) == 0) {
        return NULL;
    }
    
    if (line[0] == '#') {
        return NULL;
    }
    
    char line_clone[256] = { 0, };
    strcpy(line_clone, line);
    
    char *id = strtok(line_clone, " ");
    char *arrive_time = strtok(NULL, " ");
    char *service_time = strtok(NULL, " ");
    char *priority = strtok(NULL, " ");
    
    command_line* cmd_line = malloc(sizeof(command_line));
    strcpy(cmd_line->id, id);
    cmd_line->arrive_time = atoi(arrive_time);
    cmd_line->service_time = atoi(service_time);
    cmd_line->priority = atoi(priority);
    
    return cmd_line;
}


int validate_line(const command_line* cmd_line) {
    // invalid process id format
    unsigned long len_id = strlen(cmd_line->id);
    for (int i = 0; i < len_id; i ++) {
        if (cmd_line->id[i] >= 'A' && cmd_line->id[i] <= 'Z') {
        } else if (cmd_line->id[i] >= '0' && cmd_line->id[i] <= '9') {
        } else {
            PRINT_ERROR("invalid process id \'%s\' in line %d, ignored\n", cmd_line->id, line_number);
            return -1;
        }
    }
    
    // duplicated process id
    for (int i = 0; i < command_line_list->num_of_elements; i ++) {
        command_line* elem = (command_line*)command_line_list->elements[i];
        if (strcmp(elem->id, cmd_line->id) == 0) {
            PRINT_ERROR("duplicate process id \'%s\' in line %d, ignored\n", cmd_line->id, line_number);
            return -1;
        }
    }
    
    // invalid arrive-time
    if (cmd_line->arrive_time < 0 || cmd_line->arrive_time > 30) {
        PRINT_ERROR("invalid arrive-time \'%d\' in line %d, ignored\n", cmd_line->arrive_time, line_number);
        return -1;
    }
    
    // invalid service-time
    if (cmd_line->service_time < 1 || cmd_line->service_time > 30) {
        PRINT_ERROR("invalid service-time \'%d\' in line %d, ignored\n", cmd_line->service_time, line_number);
        return -1;
    }
    
    // invalid priority
    if (cmd_line->priority < 1 || cmd_line->priority > 10) {
        PRINT_ERROR("invalid priority \'%d\' in line %d, ignored\n", cmd_line->priority, line_number);
        return -1;
    }
    
    return 0;
}


int command_line_sort_comp_arrive_time(const void* p, const void* q) {
    command_line* cmd_line1 = (command_line*)(*(void**)p);
    command_line* cmd_line2 = (command_line*)(*(void**)q);
    
    return cmd_line1->arrive_time - cmd_line2->arrive_time;
}


int command_line_sort_comp_id(const void* p, const void* q) {
    command_line* cmd_line1 = (command_line*)(*(void**)p);
    command_line* cmd_line2 = (command_line*)(*(void**)q);
    
    return strcmp(cmd_line1->id, cmd_line2->id);
}


int start() {
    qsort(command_line_list->elements,
          command_line_list->num_of_elements,
          sizeof(void*),
          command_line_sort_comp_id);
    
    if (algorithm_sjf() < 0) {
        return -1;
    }
    
    if (algorithm_srt() < 0) {
        return -1;
    }
    
    if (algorithm_rr() < 0) {
        return -1;
    }
    
    if (algorithm_pr() < 0) {
        return -1;
    }
    
    return 0;
}

struct list* clone_command_line_list() {
    struct list* command_line_list_clone = list_create();
    list_resize(command_line_list_clone, command_line_list->num_of_elements);
    command_line_list_clone->num_of_elements = command_line_list->num_of_elements;
    for (int i = 0; i < command_line_list->num_of_elements; i ++) {
        command_line* elem = command_line_list->elements[i];
        command_line* clone = malloc(sizeof(command_line));
        memcpy(clone, elem, sizeof(command_line));
        
        command_line_list_clone->elements[i] = clone;
    }
    
    return command_line_list_clone;
}

int end_all_jobs(struct list* job_list) {
    int finished = 0;
    for (int i = 0; i < job_list->num_of_elements; i ++) {
        command_line* cmd_line = job_list->elements[i];
        if (cmd_line->service_time <= 0) {
            finished ++;
        }
    }
    
    return finished == job_list->num_of_elements;
}


int exist_same_id(struct list* const job_list, const char* id) {
    for (int i = 0; i < job_list->num_of_elements; i ++) {
        command_line* cmd_line = job_list->elements[i];
        if (strcmp(cmd_line->id, id) == 0) {
            return 1;
        }
    }

    return 0;
}


void print_result(const char* type, command_line** job_by_time, int end_time) {
    printf("[%s]\n", type);
    for (int i = 0; i < command_line_list->num_of_elements; i ++) {
        command_line* cmd_line = command_line_list->elements[i];
        printf("%s ", cmd_line->id);
        for (int j = 0; j < end_time; j ++) {
            if (!job_by_time[j]) {
                continue;
            }
            
            if (strcmp(cmd_line->id, job_by_time[j]->id) == 0) {
                printf("*");
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    printf("\n");
}


int algorithm_sjf() {
    struct list* command_line_list_clone = clone_command_line_list();
    command_line** job_by_time = malloc(sizeof(command_line*) * 1024);
    
    qsort(command_line_list_clone->elements,
          (size_t) command_line_list_clone->num_of_elements,
          sizeof(void*),
          command_line_sort_comp_arrive_time);
    
    int time = 0;
    while (!end_all_jobs(command_line_list_clone)) {
        command_line* target = NULL;
        int min_service_time = 999;
        for (int i = 0; i < command_line_list_clone->num_of_elements; i ++) {
            command_line* cmd_line = command_line_list_clone->elements[i];
            if (cmd_line->arrive_time <= time && cmd_line->service_time > 0 && cmd_line->service_time < min_service_time) {
                target = cmd_line;
                min_service_time = cmd_line->service_time;
            }
        }
        
        for (int j = time; j <= time + target->service_time; j ++) {
            job_by_time[j] = target;
        }
        time += target->service_time;
        target->service_time = 0;
    }
    
    print_result("SJF", job_by_time, time);
    
    return 0;
}

int algorithm_srt() {
    struct list* command_line_list_clone = clone_command_line_list();
    command_line** job_by_time = malloc(sizeof(command_line*) * 1024);
    
    qsort(command_line_list_clone->elements,
          (size_t) command_line_list_clone->num_of_elements,
          sizeof(void*),
          command_line_sort_comp_arrive_time);
    
    int time = 0;
    while (!end_all_jobs(command_line_list_clone)) {
        command_line* target = NULL;
        int min_service_time = 999;
        for (int i = 0; i < command_line_list_clone->num_of_elements; i ++) {
            command_line* cmd_line = command_line_list_clone->elements[i];
            if (cmd_line->arrive_time <= time && cmd_line->service_time > 0 && cmd_line->service_time < min_service_time) {
                target = cmd_line;
                min_service_time = cmd_line->service_time;
            }
        }
        
        job_by_time[time++] = target;
        target->service_time--;
    }
    
    print_result("SRT", job_by_time, time);
    
    return 0;
}

int algorithm_rr() {
    struct list* command_line_list_clone = clone_command_line_list();
    command_line** job_by_time = malloc(sizeof(command_line*) * 1024);
    
    qsort(command_line_list_clone->elements,
          (size_t) command_line_list_clone->num_of_elements,
          sizeof(void*),
          command_line_sort_comp_arrive_time);
    
    
    struct list* round_queue = list_create();
    
    int next_turn[1024];
    int next_turn_count = 0;
    memset(next_turn, -1, sizeof(next_turn));
    
    int time = 0;
    int last_job_index = -1;
    while (!end_all_jobs(command_line_list_clone)) {
        // push job if not exist in queue
        for (int i = 0; i < command_line_list_clone->num_of_elements; i ++) {
            command_line* cmd_line = command_line_list_clone->elements[i];
            if (cmd_line->arrive_time <= time
                && cmd_line->service_time > 0) {
                if (!exist_same_id(round_queue, cmd_line->id)) {
                    int j;
                    for (j = 0; j < next_turn_count; j ++) {
                        if (next_turn[j] == i) {
                            break;
                        }
                    }

                    if (j == next_turn_count) {
                        next_turn[next_turn_count++] = i;
                    }
                }
            }
        }

        if (round_queue->num_of_elements == 0) {
            for (int i = 0; i < next_turn_count; i ++) {
                list_push_back(round_queue, command_line_list_clone->elements[next_turn[i]]);
            }

            memset(next_turn, -1, sizeof(next_turn));
            next_turn_count = 0;
        }

        last_job_index = (last_job_index + 1) % round_queue->num_of_elements;
        if (last_job_index == 0 && next_turn_count > 0) {
            for (int i = 0; i < next_turn_count; i ++) {
                list_push_back(round_queue, command_line_list_clone->elements[next_turn[i]]);
            }
            
            memset(next_turn, -1, sizeof(next_turn));
            next_turn_count = 0;
        }
        
        command_line* target = round_queue->elements[last_job_index];
        job_by_time[time++] = target;
        target->service_time--;
        
        if (target->service_time <= 0) {
            list_remove_at(round_queue, last_job_index);
            last_job_index--;
        }
    }
    
    print_result("RR", job_by_time, time);
    
    return 0;
}

int algorithm_pr() {
    struct list* command_line_list_clone = clone_command_line_list();
    command_line** job_by_time = malloc(sizeof(command_line*) * 1024);

    qsort(command_line_list_clone->elements,
          (size_t) command_line_list_clone->num_of_elements,
          sizeof(void*),
          command_line_sort_comp_arrive_time);

    int time = 0;
    while (!end_all_jobs(command_line_list_clone)) {
        int min_priority = 999;

        command_line* target = NULL;
        for (int i = 0; i < command_line_list_clone->num_of_elements; i ++) {
            command_line* cmd_line = command_line_list_clone->elements[i];
            if (cmd_line->arrive_time <= time
                && cmd_line->service_time > 0
                && cmd_line->priority < min_priority) {
                target = cmd_line;
                min_priority = cmd_line->priority;
            }
        }

        job_by_time[time++] = target;
        target->service_time--;
    }

    print_result("PR", job_by_time, time);

    return 0;
}