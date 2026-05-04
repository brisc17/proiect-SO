#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#define PID_FILE ".monitor_pid"

void handle_signint(int sig){
    printf("\n[monitor] SIGINT received. Shutting down...\n");

    unlink(PID_FILE);
    exit(0);
}

void handle_sigusr1(int sig){
    printf("[monitor] SIGUSR1 received: a new report was added\n");
}

int main(){
    int fd=open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd<0){
        perror("open .monitor_pid error");
        return 1;
    }
}

