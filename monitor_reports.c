#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>


#define PID_FILE ".monitor_pid"

pid_t pid;//pid-ul copilului



void child_print(int sig){
    write(STDOUT_FILENO,
        "[monitor] SIGUSR1 received: a new report was added\n", 51);
}

void child_process(){
    struct sigaction sa;

    sa.sa_handler=child_print;
    sa.sa_flags=0;
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGUSR1,&sa,NULL)<0){
        perror("sigaction error");
        exit(1);
    }
    while(1)
        pause();
}

void handle_signint(int sig){
    write(STDOUT_FILENO,
        "\n[monitor] SIGINT received. Shutting down...\n", 45);

    unlink(PID_FILE);
    kill(pid,SIGTERM);
    exit(0);
}

void parent(){
    struct sigaction sa, sal;

    sa.sa_handler=handle_signint;
    sa.sa_flags=0;
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGINT,&sa,NULL)<0){
        perror("sigaction error");
        unlink(PID_FILE);
        exit(1);
    }

    while(1)
        pause();


}

int main(){
    int fd=open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd<0){
        perror("open .monitor_pid error");
        return 1;
    }

    char buf[32];
    int len=snprintf(buf,sizeof(buf),"%d\n",(int)getpid());
    write(fd,buf,len);
    close(fd);

    printf("[monitor] Started. PID=%d written to '%s'\n",
           (int)getpid(), PID_FILE);
    printf("[monitor] Waiting for signals... (Ctrl+C to stop)\n");

    if((pid=fork())<0){
        perror("fork error");
        exit(1);
    }
    if(pid==0)
        child_process();
    parent();
    
    return 0;
}



