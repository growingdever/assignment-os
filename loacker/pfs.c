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
#include <signal.h>


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
        // path is const char*
        char buf[1024] = { 0, };
        memset(buf, 0, sizeof(buf));
        strcpy(buf, path);

        // parse pid
        int pid = atoi(strtok(buf, "-") + 1);

        char path[1024] = { 0, };
        sprintf(path, "/proc/%d", pid);

        struct stat file_stat;
        lstat(path, &file_stat);

        // regular file
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        // fill times
        stbuf->st_atime = file_stat.st_atime;
        stbuf->st_mtime = file_stat.st_mtime;
        stbuf->st_ctime = file_stat.st_ctime;
        // fill uid and gid
        stbuf->st_uid = file_stat.st_uid;
        stbuf->st_gid = file_stat.st_gid;
        // fill memory size
        stbuf->st_size = get_usage_of_memory(pid);

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
            while (1) {
                struct dirent *dir = readdir (dp);
                if (!dir) {
                    break;
                }

                char proc_path[1024] = { 0, };
                sprintf(proc_path, "/proc/%s", dir->d_name);

                int pid = atoi(dir->d_name);

                struct stat file_stat;
                lstat(proc_path, &file_stat);

                // fill only directories(processes)
                // if result of atoi function is 0, that is invalid file
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

                    // make formatted file name
                    char result[1024] = { 0, };
                    sprintf(result, "%d-%s", pid, cmdline + offset);

                    filler(buf, result, NULL, 0);
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
    // path is const char*
    char buf[1024] = { 0, };
    memset(buf, 0, sizeof(buf));
    strcpy(buf, path);

    // parse pid
    int pid = atoi(strtok(buf, "-") + 1);

    kill(pid, SIGKILL);

    return 0;
}

static struct fuse_operations pfs_oper =
        {
                .getattr  = pfs_getattr,
                .readdir  = pfs_readdir,
                .unlink   = pfs_unlink,
        };

int main(int argc, char **argv) {
    return fuse_main (argc, argv, &pfs_oper);
}
