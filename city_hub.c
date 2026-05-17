#define _POSIX_C_SOURCE 200809L
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
 
#define MONITOR_EXECUTABLE "./monitor_reports"
#define PID_FILE           ".monitor_pid"
#define BUF_SIZE           512
#define CMD_SIZE           256

pid_t hub_mon_pid = -1;

void cmd_start_monitor(void)
{
    if(hub_mon_pid != -1){
        if(waitpid(hub_mon_pid, NULL, WNOHANG) == 0){
            printf("[city_hub] Monitor already running (hub_mon pid=%d).\n",
                   hub_mon_pid);
            return;
        }
        hub_mon_pid = -1;
    }
 
    pid_t pid = fork();
    if(pid < 0){
        perror("city_hub: fork hub_mon");
        return;
    }
    if(pid == 0){
        hub_mon_process(); //nu se intoarce
        exit(1);
    }
 
    hub_mon_pid = pid;
    printf("[city_hub] hub_mon started (pid=%d).\n", hub_mon_pid);
}