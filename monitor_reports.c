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

pid_t child_pid;//pid-ul copilului

int pipe_fd=-1;//write-end-ul primit de la hub_mon

void pipe_write(const char *type,const char *msg){
    char buf[512];
    int len=snprintf(buf,sizeof(buf),"MSG:%s:%s\n",type,msg);
    write(pipe_fd,buf,len);
}

int check_existing_monitor(){
    int fd=open(PID_FILE,O_RDONLY);
    if(fd<0) 
        return 0;

    char buf[32];
    memset(buf,0,sizeof(buf));
    read(fd,buf,sizeof(buf)-1);
    close(fd);

    pid_t existing=(pid_t)atoi(buf);
    if(existing<=0)
        return 0;
    if(kill(existing,0)==0){
        //procesul exista-scriem eroarea prin pipe
        char msg[64];
        snprintf(msg,sizeof(msg),"already running pid=%d\n",(int)existing);
        pipe_write("error",msg);
        return 1;
    }
    return 0;//fisierul exista dar nu mai traieste asa ca il ignoram
}



void child_print(int sig){
    pipe_write("normal",
        "[monitor] SIGUSR1 received: a new report was added\n");
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
    pipe_write("exit",
        "\n[monitor] SIGINT received. Shutting down...\n", 45);

    unlink(PID_FILE);
    if (child_pid>0) kill(child_pid, SIGTERM);
    //inchiderea pipe_fd trimite EOF catre hub_mon 
    close(pipe_fd);
    exit(0);
}

void parent(){
    struct sigaction sa, sal;

    sa.sa_handler=handle_signint;
    sa.sa_flags=0;
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGINT,&sa,NULL)<0){
        perror("sigaction error");
        exit(1);
    }

    while(1)
        pause();


}
//argv[1] write-end-ul pipe0ului de la hub_mon
int main(int argc,char *argv[]){
    if (argc < 2) {
        fprintf(stderr,"monitor_reports: missing pipe fd argument\n");
        return 1;
    }
    pipe_fd=atoi(argv[1]);

    if(check_existing_monitor()){
        close(pipe_fd);
        return 1;
    }
    pipe_write("normal","[monitor] Started. Waiting for signals... (Ctrl+C to stop)");

    child_pid=fork();
    if(child_pid<0){
        perror("fork error");
        close(pipe_fd);
        exit(1);
    }
    if(child_pid==0)
        child_process();

    int fd=open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd<0){
        perror("open .monitor_pid error");
        close(pipe_fd);
        return 1;
    }

    char pid_buf[32];
    int pid_len=snprintf(pid_buf,sizeof(pid_buf),"%d\n",(int)child_pid);
    write(fd,pid_buf,pid_len);
    close(fd);

    parent();
    
    return 0;
}



