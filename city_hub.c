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
#define SCORER_EXECUTABLE  "./scorer"
#define CMD_SIZE           256
#define BUF_SIZE           512
#define MAX_DISTRICTS      32
 
pid_t hub_mon_pid = -1;


void hub_mon_process() {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("[hub_mon] pipe");
        exit(1);
    }
 
    //convertim write-end in string ca sa-l trimitem ca argv[1] 
    char fd_str[16];
    snprintf(fd_str, sizeof(fd_str), "%d", pipefd[1]);
 
    pid_t mon_pid = fork();
    if (mon_pid < 0) {
        perror("[hub_mon] fork monitor");
        exit(1);
    }
 
    if (mon_pid == 0) {
        //child process va fi monitor_reports
        //copilul nu citeste din pipe,inchidem read-end
        close(pipefd[0]);
        //inlocuim imaginea procesului cu monitor_reports.
        //fd_str ii spune monitorului pe ce fd sa scrie
        execl(MONITOR_EXECUTABLE, "monitor_reports", fd_str, (char *)NULL);
        perror("[hub_mon] execl monitor_reports");
        exit(1);
    }
 
    //parent process hub_mon
    //nu scriem in pipe,inchidem write-end
    close(pipefd[1]);
 
    FILE *f = fdopen(pipefd[0], "r");
    if (!f) {
        perror("[hub_mon] fdopen");
        exit(1);
    }
 
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        //format asteptat: MSG:<type>:<mesaj>\n
        if (strncmp(line, "MSG:", 4) != 0) continue;
 
        char *type_start = line + 4;
        char *colon = strchr(type_start, ':');
        if (!colon) continue;
 
        //terminam type_start la ':' ca sa fie string separat 
        *colon = '\0';
        char *type = type_start;
        char *msg  = colon + 1;
 
        msg[strcspn(msg, "\n")] = '\0';
 
        if (strcmp(type, "error") == 0) {
            printf("[city_hub] Monitor error: %s\n", msg);
            fflush(stdout);
            break; //monitorul a scris eroare si s-a terminat 
        } else if (strcmp(type, "exit") == 0) {
            printf("[city_hub] Monitor has ended: %s\n", msg);
            fflush(stdout);
            break; //monitorul se inchide 
        } else {
            //mesaj normal de la monitor 
            printf("[city_hub/monitor] %s\n", msg);
            fflush(stdout);
        }
    }
 
   
    printf("[city_hub] hub_mon: monitor pipe closed. Exiting.\n");
    fflush(stdout);
    fclose(f);
    waitpid(mon_pid, NULL, 0);
    exit(0);
}

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

void cmd_calculate_scores(char *districts[],int nr_dstr){
    if(nr_dstr<=0){
        printf("[city_hub] calculate_scores: no districts specified\n");
        return;
    }

    //array de read-end-uri, cate unul per district 
    int read_ends[MAX_DISTRICTS];
    pid_t scorer_pids[MAX_DISTRICTS];
    int actual = 0; //cati scoreri am pornit cu succes 
 
    //pornim cate un scorer per district
    for (int i = 0; i<nr_dstr && i<MAX_DISTRICTS; i++) {
        int pipefd[2];
        if (pipe(pipefd) < 0) {
            perror("[city_hub] pipe for scorer");
            continue;
        }
 
        pid_t pid = fork();
        if (pid < 0) {
            perror("[city_hub] fork scorer");
            close(pipefd[0]);
            close(pipefd[1]);
            continue;
        }
 
        if (pid == 0) {
            //copil=scorer
            //redirectam stdout al scorerului catre write-end-ul pipe-ului
            //dupa dup2(), orice printf() din scorer merge in pipe
            if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
                perror("[scorer] dup2");
                exit(1);
            }
            close(pipefd[0]); //copilul nu citeste 
            close(pipefd[1]); //inchidem originalul,dup2 a facut copia 
 
            execl(SCORER_EXECUTABLE, "scorer", districts[i], (char *)NULL);
            perror("[city_hub] execl scorer");
            exit(1);
        }
 
        //parent
        close(pipefd[1]);
        read_ends[actual]   = pipefd[0];
        scorer_pids[actual] = pid;
        actual++;
    }
 
    //citim output-ul din fiecare pipe si afisam raportul
    printf("\n=== Workload Report ===\n");
    printf("%-20s %-25s %-15s %-10s\n",
           "District", "Inspector", "Total Severity", "Nr Reports");
    printf("%-20s %-25s %-15s %-10s\n",
           "--------", "---------", "--------------", "----------");
 
    for (int i = 0; i < actual; i++) {
        FILE *f = fdopen(read_ends[i], "r");
        if (!f) {
            perror("[city_hub] fdopen scorer pipe");
            close(read_ends[i]);
            waitpid(scorer_pids[i], NULL, 0);
            continue;
        }
 
        char line[BUF_SIZE];
        while (fgets(line, sizeof(line), f)) {
            //format: SCORE:<district>:<inspector>:<total_sev>:<nr_rapoarte> 
            if (strncmp(line, "SCORE:", 6) != 0) continue;
 
            
            char *p = line + 6;
 
            //district 
            char *colon1 = strchr(p, ':');
            if (!colon1) continue;
            *colon1 = '\0';
            char *dist = p;
 
            //inspector 
            char *colon2 = strchr(colon1 + 1, ':');
            if (!colon2) continue;
            *colon2 = '\0';
            char *insp = colon1 + 1;
 
            //total_severity 
            char *colon3 = strchr(colon2 + 1, ':');
            if (!colon3) continue;
            *colon3 = '\0';
            char *sev_str = colon2 + 1;
 
            //nr_rapoarte 
            char *nr_str = colon3 + 1;
            nr_str[strcspn(nr_str, "\n")] = '\0';
 
            printf("%-20s %-25s %-15s %-10s\n",
                   dist, insp, sev_str, nr_str);
        }
 
        fclose(f); //inchidem si read-end-ul 
        waitpid(scorer_pids[i], NULL, 0);
    }
 
    printf("======================\n\n");
}

int main(){
    char line[256*4];
    printf("[city_hub] Started. Available commands:\n");
    printf("  start_monitor\n");
    printf("  quit\n");
    printf("> ");
    fflush(stdout);

    while(fgets(line,sizeof(line),stdin)){
        line[strcspn(line,"\n")]='\0';

        if(strlen(line)==0){
            printf(">");
            fflush(stdout);
            continue;
        }


        char *tokens[MAX_DISTRICTS + 2]; //cmd + districte 
        int num_tokens=0;
        char *tok=strtok(line, " ");
        while (tok && num_tokens < MAX_DISTRICTS + 1) {
            tokens[num_tokens++]=tok;
            tok=strtok(NULL, " ");
        }
 
        if (num_tokens==0) {
            printf("> ");
            fflush(stdout);
            continue;
        }
 
        char *cmd=tokens[0];
 
        if (strcmp(cmd, "start_monitor")==0) {
            cmd_start_monitor();
 
        } else if (strcmp(cmd, "calculate_scores")==0) {
            /* Districtele sunt tokens[1], tokens[2], ... */
            cmd_calculate_scores(tokens+1,num_tokens-1);
 
        } else if (strcmp(cmd, "quit")==0) {
            printf("[city_hub] Shutting down.\n");
            if (hub_mon_pid != -1) {
                kill(hub_mon_pid, SIGTERM);
                waitpid(hub_mon_pid, NULL, 0);
            }
            break;
 
        } else {
            printf("[city_hub] Unknown command: '%s'\n", cmd);
            printf("  Commands: start_monitor | calculate_scores <d1> [d2...] | quit\n");
        }
 
        printf("> ");
        fflush(stdout);
    }
 
    return 0;
}