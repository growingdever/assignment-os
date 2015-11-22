/*
 * OS Assignment #5
 *
 * @file person.c
 */

#include "person.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <signal.h>


const char* file_path;
char origin_data[sizeof(Person)];


static void print_usage(const char *prog) {
    fprintf(stderr, "usage: %s [-f file] [-w] [-s value] attr_name\n", prog);
    fprintf(stderr, "  -f file  : person file, default is './person.dat'\n");
    fprintf(stderr, "  -w       : watch mode\n");
    fprintf(stderr, "  -s value : set the value for the given 'attr_name'\n");
}

void create_if_not_exist(const char* path) {
    if (access(path, F_OK) != -1) {
        // exist
    } else {
        // not exist
        int fd = open(path, O_RDWR | O_CREAT, 0777);

        Person* person = malloc(sizeof(Person));
        memset(person, 0x00, sizeof(Person));

        write(fd, person, sizeof(Person));

        close(fd);
    }
}

void regist_watcher_self(const char* path) {
    pid_t pid = getpid();

    int fd = open(path, O_RDWR);
    char* person = mmap(0, sizeof(Person), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    int i;
    for (i = 0; i < NOTIFY_MAX; i ++) {
        pid_t* elem = (pid_t*)(person + sizeof(pid_t) * i);
        if (*elem == 0) {
            *elem = pid;
            break;
        }
    }

    if (i == NOTIFY_MAX) {
        *(pid_t*)(person) = pid;
    }

    munmap(person, sizeof(Person));
    close(fd);
}

void unregist_watcher_self(const char* path) {
    pid_t pid = getpid();

    int fd = open(path, O_RDWR);
    char* person = mmap(0, sizeof(Person), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    int i;
    for (i = 0; i < NOTIFY_MAX; i ++) {
        pid_t* elem = (pid_t*)(person + sizeof(pid_t) * i);
        if (*elem == pid) {
            break;
        }
    }

    // move right side values
    for (; i < NOTIFY_MAX; i ++) {
        pid_t* p = (pid_t*)(person + sizeof(pid_t) * i);
        *p = *(p + 1);
    }
    *((pid_t*)(person + sizeof(pid_t) * NOTIFY_MAX)) = 0;

    munmap(person, sizeof(Person));
    close(fd);
}

void signal_handler_modify_attr(int signo, siginfo_t* si) {
    if(si->si_code == SI_QUEUE && signo == SIGUSR1) {
        size_t offset = (size_t) si->si_value.sival_int;

        int fd = open(file_path, O_RDWR);
        char* person = mmap(0, sizeof(Person), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        if (offset == person_get_offset_of_attr("age") || offset == person_get_offset_of_attr("gender")) {
            // read integer value
            int* origin_value = (int *) (origin_data + offset);
            int* new_value = (int *) (person + offset);
            fprintf(stdout, "%s: \'%d\' from \'%d\'\n", person_lookup_attr_with_offset(offset), *new_value, *origin_value);
        } else {
            // read string value
            char* origin_value = origin_data + offset;
            char* new_value = person + offset;
            fprintf(stdout, "%s: \'%s\' from \'%s\'\n", person_lookup_attr_with_offset(offset), new_value, origin_value);
        }
        fflush(stdout);

        // re-copy
        for (int i = 0; i < sizeof(Person); i ++) {
            *(origin_data + i) = *(person + i);
        }

        munmap(person, sizeof(Person));
        close(fd);
    } else {
    }
}

void signal_handler_int_or_term(int signo) {
    unregist_watcher_self(file_path);
    exit(0);
}

void watch(const char* path) {
    regist_watcher_self(path);


    // temp
    struct sigaction sa_old;

    struct sigaction sa_usr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_SIGINFO;
    sa_usr1.sa_handler = signal_handler_modify_attr;
    sigaction(SIGUSR1, &sa_usr1, &sa_old);

    struct sigaction sa_int;
    sa_int.sa_handler = signal_handler_int_or_term;
    sigemptyset(&sa_int.sa_mask);
    sigaction(SIGINT, &sa_int, &sa_old);

    struct sigaction sa_term;
    sa_int.sa_handler = signal_handler_int_or_term;
    sigemptyset(&sa_int.sa_mask);
    sigaction(SIGTERM, &sa_int, &sa_old);


    // init
    {
        int fd = open(file_path, O_RDWR);
        char* person = mmap(0, sizeof(Person), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        for (int i = 0; i < sizeof(Person); i ++) {
            *(origin_data + i) = *(person + i);
        }

        munmap(person, sizeof(Person));
        close(fd);
    }


    // start monitoring
    while(1) {
        sleep(1);
    }
}

void get_watchers(const char* path, pid_t watchers[NOTIFY_MAX]) {
    int fd = open(path, O_RDWR);
    char* person = mmap(0, sizeof(Person), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    // copy watchers's pid to argument
    for (int i = 0; i < NOTIFY_MAX; i ++) {
        pid_t pid = *((pid_t*)(person + sizeof(pid_t) * i));
        watchers[i] = pid;
    }

    munmap(person, sizeof(Person));
    close(fd);
}

void set_data_pid(const char* path, int index, pid_t value) {
    int fd = open(path, O_RDWR);
    char* person = mmap(0, sizeof(Person), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    *(pid_t*)(person + sizeof(pid_t) * index) = value;

    munmap(person, sizeof(Person));
    close(fd);
}

void set_data_int(const char* path, int offset, int value) {
    int fd = open(path, O_RDWR);
    char* person = mmap(0, sizeof(Person), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    *(int*)(person + offset) = value;

    munmap(person, sizeof(Person));
    close(fd);
}

void set_data_string(const char* path, int offset, const char* src) {
    int fd = open(path, O_RDWR);
    char* person = mmap(0, sizeof(Person), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    char* dst = person + offset;

    // fill zero
    memset(dst, 0, strlen(dst));

    // copy
    while((*dst++ = *src++) != 0);

    munmap(person, sizeof(Person));
    close(fd);
}

void value_update(const char* path, const char* attr, const char* value) {
    if (person_get_offset_of_attr(attr) == -1) {
        printf("invalid attr name \'%s\'", attr);
        return;
    }

    int offset = (int)person_get_offset_of_attr(attr);
    {
        if (offset == person_get_offset_of_attr("age") || offset == person_get_offset_of_attr("gender")) {
            set_data_int(path, offset, atoi(value));
        } else {
            set_data_string(path, offset, value);
        }
    }


    pid_t watchers[NOTIFY_MAX] = { 0, };
    get_watchers(path, watchers);


    union sigval sv;
    sv.sival_int = offset;

    // notifying
    for (int i = 0; i < NOTIFY_MAX; i ++) {
        if (watchers[i] != 0 && sigqueue(watchers[i], SIGUSR1, sv) == -1) {
            printf("fail to send signal to watcher(%d)\n", watchers[i]);
            set_data_pid(path, i, 0);
        }
    }
}

int main(int argc, char **argv) {
    file_path = "./person.dat";
    const char* set_value = NULL;
    int watch_mode = 0;

    /* Parse command line arguments. */
    const char* attr_name;
    while (1) {
        int opt;

        opt = getopt(argc, argv, "ws:f:");
        if (opt < 0) {
            break;
        }

        switch (opt) {
            case 'w':
                watch_mode = 1;
                break;
            case 's':
                set_value = optarg;
                break;
            case 'f':
                file_path = optarg;
                break;
            default:
                print_usage(argv[0]);
                return -1;
        }
    }
    if (!watch_mode && optind >= argc) {
        print_usage(argv[0]);
        return -1;
    }
    attr_name = argv[optind];

    printf("start : %d\n", getpid());

    create_if_not_exist(file_path);

    /* not yet implemented */
    if (watch_mode) {
        watch(file_path);
    } else {
        value_update(file_path, attr_name, set_value);
    }

    return 0;
}
