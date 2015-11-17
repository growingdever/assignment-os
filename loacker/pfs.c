/*
 * OS Assignment #4
 *
 * @file pfs.c
 */

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>



void get_cmdline(int pid, char cmdline[1024]) {
    char path[1024] = { 0, };
    sprintf(path, "/proc/%d/cmdline", pid);

    FILE *fp = fopen(path, "r");
    if (fp == 0) {
        return 0;
    }

    char line[1024] = { 0, };
    fgets(line, sizeof(line), fp);

    fclose(fp);

    strcpy(cmdline, line);
}

time_t get_start_time(int pid) {
    char path[1024] = { 0, };
    sprintf(path, "/proc/%d", pid);

    struct stat file_stat;
    lstat(path, &file_stat);

    return file_stat.st_mtime;
}

int get_usage_of_memory(int pid) {
    char path[1024] = {0, };
    sprintf(path, "/proc/%d/status", pid);

    FILE *fp = fopen(path, "r");
    if (fp == 0) {
        return 0;
    }

    char buf[1024], buf2[1024];
    int mem;
    while (!feof(fp)) {
        fgets(buf, sizeof(buf), fp);
        sscanf(buf, "%s", buf2);
        if (strcmp(buf2, "VmSize:") == 0) {
            sscanf(buf, "%s %d", buf2, &mem);
            return mem;
        }
    }

    fclose(fp);

    return 0;
}


static int pfs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    int ret = -ENOENT;

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;

        ret = 0;
    } else if (strcmp(path, "/.") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;

        ret = 0;
    } else if (strcmp(path, "/..") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;

        ret = 0;
    } else {
        char buf[1024] = { 0, };
        memset(buf, 0, sizeof(buf));
        strcpy(buf, path);

        char* pid_str = strtok(buf, "-") + 1;

        int pid = atoi(pid_str);
        int memory_size = get_usage_of_memory(pid);
        long long start_time = get_start_time(pid);

        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = memory_size;
        stbuf->st_mtime = (time_t)start_time;

        ret = 0;
    }

    if (ret != 0) {
        printf("error : %s\n", path);
    }

    return ret;
}

static int pfs_readdir(const char *path,
                       void *buf,
                       fuse_fill_dir_t filler,
                       off_t offset,
                       struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    if (strcmp(path, "/") == 0) {
        DIR *dp = opendir ("/proc");
        if (dp != NULL) {
            int count = 99999;
            while (count > 0) {
                struct dirent *dir = readdir (dp);
                if (!dir) {
                    break;
                }

                char proc_path[1024] = { 0, };
                sprintf(proc_path, "/proc/%s", dir->d_name);

                int pid = atoi(dir->d_name);

                struct stat file_stat;
                lstat(proc_path, &file_stat);
                if (S_ISDIR(file_stat.st_mode) && pid != 0) {
                    char cmdline[1024] = { 0, };
                    get_cmdline(pid, cmdline);

                    if (strlen(cmdline) == 0) {
                        continue;
                    }

                    int i;
                    for (i = 0; i < strlen(cmdline); i ++) {
                        if (cmdline[i] == '/') {
                            cmdline[i] = '-';
                        }
                    }

                    int offset = 0;
                    if (cmdline[0] == '-') {
                        offset = 1;
                    }

                    char result[1024] = { 0, };
                    sprintf(result, "%d-%s", pid, cmdline + offset);

                    filler(buf, result, NULL, 0);

                    count--;
                }
            }
            closedir (dp);
        } else {
            return -ENOENT;
        }
    } else {
        return -ENOENT;
    }

    return 0;
}

static int pfs_unlink(const char *path) {
    return 0;
}

static int pfs_open(const char *path, struct fuse_file_info *fi)
{
    printf("pfs_open : %s\n", path);
    return -ENOENT;
}

static int pfs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    printf("pfs_read : %s\n", path);
    return -ENOENT;
}

static struct fuse_operations pfs_oper =
        {
                .getattr  = pfs_getattr,
                .readdir  = pfs_readdir,
                .unlink   = pfs_unlink,
                .open    = pfs_open,
                .read    = pfs_read,
        };

int main(int argc, char **argv) {
    return fuse_main (argc, argv, &pfs_oper);
}
