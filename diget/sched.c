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
#define MEMSET(var, val) memset(var, val, sizeof(var))


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


typedef struct _tag_job {
    char id[16];
    int arrive_time;
    int service_time;
    int curr_service_time;
    int priority;
    int end_time;
} job;


int run(const char*);
void fgets_except_linefeed(char*, int, FILE*);
job* parse_line(const char*);
int validate_line(const job*);
int job_sort_comp(const void*, const void*);
int start();
int algorithm_sjf();
int algorithm_srt();
int algorithm_rr();
int algorithm_pr();


int line_number = 0;
struct list* job_list;


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
    
    job_list = list_create();
    
    char line[256];
    while (!feof(fp)) {
        MEMSET(line, 0);
        fgets_except_linefeed(line, sizeof(line), fp);
        line_number ++;
        
        job *new_job = parse_line(line);
        if (new_job == NULL) {
            continue;
        }
        
        if (!validate_line(new_job)) {
            list_push_back(job_list, new_job);
        }
    }
    
    fclose(fp);

    printf("\n");
    
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


job* parse_line(const char* line) {
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
    
    job* new_job = malloc(sizeof(job));
    strcpy(new_job->id, id);
    new_job->arrive_time = atoi(arrive_time);
    new_job->service_time = new_job->curr_service_time = atoi(service_time);
    new_job->priority = atoi(priority);
    
    return new_job;
}


int validate_line(const job* target_job) {
    // invalid process id format
    unsigned long len_id = strlen(target_job->id);
    for (int i = 0; i < len_id; i ++) {
        if (target_job->id[i] >= 'A' && target_job->id[i] <= 'Z') {
        } else if (target_job->id[i] >= '0' && target_job->id[i] <= '9') {
        } else {
            PRINT_ERROR("invalid process id \'%s\' in line %d, ignored\n", target_job->id, line_number);
            return -1;
        }
    }
    
    // duplicated process id
    for (int i = 0; i < job_list->num_of_elements; i ++) {
        job* elem = (job*)job_list->elements[i];
        if (strcmp(elem->id, target_job->id) == 0) {
            PRINT_ERROR("duplicate process id \'%s\' in line %d, ignored\n", target_job->id, line_number);
            return -1;
        }
    }
    
    // invalid arrive-time
    if (target_job->arrive_time < 0 || target_job->arrive_time > 30) {
        PRINT_ERROR("invalid arrive-time \'%d\' in line %d, ignored\n", target_job->arrive_time, line_number);
        return -1;
    }
    
    // invalid service-time
    if (target_job->curr_service_time < 1 || target_job->curr_service_time > 30) {
        PRINT_ERROR("invalid service-time \'%d\' in line %d, ignored\n", target_job->curr_service_time, line_number);
        return -1;
    }
    
    // invalid priority
    if (target_job->priority < 1 || target_job->priority > 10) {
        PRINT_ERROR("invalid priority \'%d\' in line %d, ignored\n", target_job->priority, line_number);
        return -1;
    }
    
    return 0;
}


int job_sort_comp_arrive_time(const void* p, const void* q) {
    job* j1 = (job*)(*(void**)p);
    job* j2 = (job*)(*(void**)q);
    
    return j1->arrive_time - j2->arrive_time;
}


int job_sort_comp_id(const void* p, const void* q) {
    job* j1 = (job*)(*(void**)p);
    job* j2 = (job*)(*(void**)q);
    
    return strcmp(j1->id, j2->id);
}


int start() {
    qsort(job_list->elements,
          job_list->num_of_elements,
          sizeof(void*),
          job_sort_comp_id);
    
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

struct list* clone_job_list() {
    struct list* job_list_clone = list_create();
    list_resize(job_list_clone, job_list->num_of_elements);
    job_list_clone->num_of_elements = job_list->num_of_elements;
    for (int i = 0; i < job_list->num_of_elements; i ++) {
        job* elem = job_list->elements[i];
        job* clone = malloc(sizeof(job));
        memcpy(clone, elem, sizeof(job));
        
        job_list_clone->elements[i] = clone;
    }
    
    return job_list_clone;
}

int end_all_jobs(struct list* li) {
    int finished = 0;
    for (int i = 0; i < li->num_of_elements; i ++) {
        job* elem = li->elements[i];
        if (elem->curr_service_time <= 0) {
            finished ++;
        }
    }
    
    return finished == li->num_of_elements;
}


int exist_same_id(struct list* const li, const char* id) {
    for (int i = 0; i < li->num_of_elements; i ++) {
        job* elem = li->elements[i];
        if (strcmp(elem->id, id) == 0) {
            return 1;
        }
    }

    return 0;
}


void print_result(const char* type, job** job_by_time, int end_time, const struct list* job_list) {
    int total_turn_arount_time = 0;
    int total_waiting_time = 0;
    for (int i = 0; i < job_list->num_of_elements; i ++) {
        job* elem = job_list->elements[i];
        total_turn_arount_time += elem->end_time - elem->arrive_time;
        total_waiting_time += (elem->end_time - elem->arrive_time) - elem->service_time;
    }

    printf("[%s]\n", type);
    for (int i = 0; i < job_list->num_of_elements; i ++) {
        job* elem = job_list->elements[i];
        printf("%s ", elem->id);
        for (int j = 0; j < end_time; j ++) {
            if (!job_by_time[j]) {
                continue;
            }
            
            if (strcmp(elem->id, job_by_time[j]->id) == 0) {
                printf("*");
            } else {
                printf(" ");
            }
        }
        printf(" \n");
    }
    printf("CPU TIME: %d\n", end_time);
    printf("AVERAGE TURNAROUND TIME: %.2f\n", 
        1.0f * total_turn_arount_time / job_list->num_of_elements);
    printf("AVERAGE WAITING TIME: %.2f\n", 
        1.0f * total_waiting_time / job_list->num_of_elements);

    printf("\n");
}


int algorithm_sjf() {
    struct list* job_list_clone = clone_job_list();
    job* job_by_time[1024];
    
    qsort(job_list_clone->elements,
          (size_t) job_list_clone->num_of_elements,
          sizeof(void*),
          job_sort_comp_arrive_time);
    
    int time = 0;
    while (!end_all_jobs(job_list_clone)) {
        job* target = NULL;
        int min_curr_service_time = 999;
        for (int i = 0; i < job_list_clone->num_of_elements; i ++) {
            job* elem = job_list_clone->elements[i];
            if (elem->arrive_time <= time 
                && elem->curr_service_time > 0 
                && elem->curr_service_time < min_curr_service_time) {
                target = elem;
                min_curr_service_time = elem->curr_service_time;
            }
        }
        
        for (int j = time; j <= time + target->curr_service_time; j ++) {
            job_by_time[j] = target;
        }
        time += target->curr_service_time;
        target->curr_service_time = 0;
        target->end_time = time;
    }
    
    print_result("SJF", job_by_time, time, job_list_clone);
    
    return 0;
}

int algorithm_srt() {
    struct list* job_list_clone = clone_job_list();
    job* job_by_time[1024];
    
    qsort(job_list_clone->elements,
          (size_t) job_list_clone->num_of_elements,
          sizeof(void*),
          job_sort_comp_arrive_time);
    
    int time = 0;
    while (!end_all_jobs(job_list_clone)) {
        job* target = NULL;
        int min_curr_service_time = 999;
        for (int i = 0; i < job_list_clone->num_of_elements; i ++) {
            job* elem = job_list_clone->elements[i];
            if (elem->arrive_time <= time 
                && elem->curr_service_time > 0 
                && elem->curr_service_time < min_curr_service_time) {
                target = elem;
                min_curr_service_time = elem->curr_service_time;
            }
        }
        
        job_by_time[time++] = target;
        target->curr_service_time--;
        if (target->curr_service_time <= 0) {
            target->end_time = time;
        }
    }
    
    print_result("SRT", job_by_time, time, job_list_clone);
    
    return 0;
}

int algorithm_rr() {
    struct list* job_list_clone = clone_job_list();
    job* job_by_time[1024];
    
    qsort(job_list_clone->elements,
          (size_t) job_list_clone->num_of_elements,
          sizeof(void*),
          job_sort_comp_arrive_time);
    
    
    struct list* round_queue = list_create();
    
    int next_turn[1024];
    int next_turn_count = 0;
    MEMSET(next_turn, -1);
    
    int time = 0;
    int last_job_index = -1;
    while (!end_all_jobs(job_list_clone)) {
        // push job if not exist in queue
        for (int i = 0; i < job_list_clone->num_of_elements; i ++) {
            job* elem = job_list_clone->elements[i];
            if (elem->arrive_time <= time
                && elem->curr_service_time > 0) {
                if (!exist_same_id(round_queue, elem->id)) {
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

        // load new jobs to queue if queue is empty
        if (round_queue->num_of_elements == 0) {
            for (int i = 0; i < next_turn_count; i ++) {
                list_push_back(round_queue, job_list_clone->elements[next_turn[i]]);
            }

            MEMSET(next_turn, -1);
            next_turn_count = 0;
        }

        last_job_index = (last_job_index + 1) % round_queue->num_of_elements;
        // load new jobs to queue if every round is rotated
        if (last_job_index == 0 && next_turn_count > 0) {
            for (int i = 0; i < next_turn_count; i ++) {
                list_push_back(round_queue, job_list_clone->elements[next_turn[i]]);
            }
            
            MEMSET(next_turn, -1);
            next_turn_count = 0;
        }
        
        job* target = round_queue->elements[last_job_index];
        job_by_time[time++] = target;
        target->curr_service_time--;
        
        // remove ended job from round queue
        if (target->curr_service_time <= 0) {
            target->end_time = time;

            list_remove_at(round_queue, last_job_index);
            last_job_index--;
        }
    }
    
    print_result("RR", job_by_time, time, job_list_clone);
    
    return 0;
}

int algorithm_pr() {
    struct list* job_list_clone = clone_job_list();
    job* job_by_time[1024];

    qsort(job_list_clone->elements,
          (size_t) job_list_clone->num_of_elements,
          sizeof(void*),
          job_sort_comp_arrive_time);

    int time = 0;
    while (!end_all_jobs(job_list_clone)) {
        int min_priority = 999;

        job* target = NULL;
        for (int i = 0; i < job_list_clone->num_of_elements; i ++) {
            job* elem = job_list_clone->elements[i];
            if (elem->arrive_time <= time
                && elem->curr_service_time > 0
                && elem->priority < min_priority) {
                target = elem;
                min_priority = elem->priority;
            }
        }

        job_by_time[time++] = target;
        target->curr_service_time--;
        if (target->curr_service_time <= 0) {
            target->end_time = time;
        }
    }

    print_result("PR", job_by_time, time, job_list_clone);

    return 0;
}