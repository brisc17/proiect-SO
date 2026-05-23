#define _POSIX_C_SOURCE 200809L
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
 
#include "city_manager.h"

typedef struct{
    char name[NAME_LEN];
    int total_sev;
    int num_reports;
}InspectorScore;

#define MAX_INSPECTORS 64

int main(int argc,char *argv[]){
    if(argc<2){
        fprintf(stderr, "scorer: usage: scorer <district>\n");
        return 1;
    }

    const char *district=argv[1];
    char reports_path[PATH_LEN];

    snprintf(reports_path,PATH_LEN,"%s/reports.dat",district);

    int fd=open(reports_path,O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "scorer: cannot open '%s': %s\n",
                reports_path, strerror(errno));
        return 1;
    }

    InspectorScore scores[MAX_INSPECTORS];
    int num_insp=0;

    Report r;
    ssize_t bytes_read;

    while((bytes_read=read(fd,&r,sizeof(Report)))==sizeof(Report)){
        int found=-1;
        for(int i=0;i<num_insp;i++){
            if(strcmp(scores[i].name,r.inspector)==0){
                found=i;
                break;
            }
        }

        if(found == -1){
            //inspector nou,il adaugam
            if(num_insp>=MAX_INSPECTORS){
                fprintf(stderr, "scorer: too many inspectors (max %d)\n",
                        MAX_INSPECTORS);
                break;
            }
            strncpy(scores[num_insp].name, r.inspector, NAME_LEN - 1);
            scores[num_insp].name[NAME_LEN - 1]='\0';
            scores[num_insp].total_sev=r.severity;
            scores[num_insp].num_reports=1;
            num_insp++;
        }else{
            //stim inspectorul
            scores[found].total_sev += r.severity;
            scores[found].num_reports++;   
        }
    }
    close(fd);

    for (int i=0;i<num_insp;i++) {
        printf("SCORE:%s:%s:%d:%d\n",
               district,
               scores[i].name,
               scores[i].total_sev,
               scores[i].num_reports);
    }
 
    /* Daca nu exista niciun raport, semnalam asta */
    if (num_insp==0) {
        printf("SCORE:%s:(no reports):-:0\n", district);
    }
 
    fflush(stdout);
    return 0;

}