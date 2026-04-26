#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#include "city_manager.h"

void build_path(char *out,const char *district,const char *filename){
    if(filename)
        snprintf(out,PATH_LEN,"%s/%s",district,filename);
    else 
        snprintf(out,PATH_LEN,"%s",district);
}

int check_permission(const char *path,int role,int need_write){
    struct stat st;
    if(stat(path,&st)<0){
        fprintf(stderr, "ERROR: Cannot stat '%s': %s\n", path, strerror(errno));
        return 0;
    }

    mode_t mode=st.st_mode;
    if(role==role_manager){//verif owner bits
        if(need_write){
            if ((mode & S_IWUSR) != 0) 
                return 1;  //bitul 'w' e pornit-PERMIS
            else 
                return 0;  //bitul 'w' e oprit-REFUZAT
        }else {
             return (mode & S_IRUSR) ? 1 : 0; //bit 'r' owner 
        }
    }else{//verif group bits
        if(need_write){
            return (mode & S_IWGRP) ? 1 : 0; //bit 'w' group 
        }else {
             return (mode & S_IRGRP) ? 1 : 0; //bit 'r' group 
        }
    }
}

int verif_district(const char *district,int role){
    char dir_path[PATH_LEN];
    char reports_path[PATH_LEN];
    char cfg_path[PATH_LEN];
    char log_path[PATH_LEN];
    char symlink_path[PATH_LEN];

    build_path(dir_path, district, NULL);
    build_path(reports_path, district, "reports.dat");
    build_path(cfg_path, district, "district.cfg");
    build_path(log_path, district, "logged_district");

    ///creeam directorul zonei:adica directorul locului unde avem pb
    if(mkdir(dir_path,PERM_DISTRICT_DIR)<0){
        if(errno!=EEXIST){
            fprintf(stderr,"Error:district already exists.");
            return -1;
        }
    }else {//daca nu-l aveam il creeam acum
            chmod(dir_path,PERM_DISTRICT_DIR);
            printf("NEW: Created district directory '%s'\n",dir_path);
    }

    //cream reports.dat daca nu exista
    int fr=open(reports_path,O_CREAT | O_EXCL | O_WRONLY, PERM_REPORTS_DAT);
    if(fr>=0){
        chmod(reports_path,PERM_REPORTS_DAT);
        close(fr);
    }

    ///cream district.cfg cu threshold care e setat pe 1
    int fd=open(cfg_path, O_CREAT| O_EXCL | O_WRONLY, PERM_DISTRICT_CFG);
    if(fd>=0){
        chmod(cfg_path,PERM_DISTRICT_CFG);
        const char *default_cfg="Severity Threshold=1\n";
        write(fd,default_cfg,strlen(default_cfg));
        close(fd);
    }
    ///cream logged_district
    int fl = open(log_path, O_CREAT | O_EXCL | O_WRONLY, PERM_LOGGED);
    if (fl >= 0) {
        chmod(log_path, PERM_LOGGED);
        close(fl);
    }

    ///facem symlink-ul -daca nu exista facem unul nou,altfel verificam daca nu pointeaza care un fisier care nu mai exista ca sa-l stergem
    snprintf(symlink_path, PATH_LEN, "active_reports-%s", district);
    struct stat lst;
    if (lstat(symlink_path, &lst) == 0) {
    // symlink-ul exista - verificam daca e dangling 
        struct stat st;
        if (stat(symlink_path, &st) == 0) {
        // stat() a reusit - symlink-ul pointeaza corect - il lasam 
            return 0;
        } else {
        // stat() a esuat - e dangling - il stergem si recreem 
        unlink(symlink_path);
        }
    }
    

    //Cream symlink-ul (fie nu exista, fie era dangling) 
    if (symlink(reports_path, symlink_path) < 0) {
    fprintf(stderr, "WARNING: Cannot create symlink '%s': %s\n",
            symlink_path, strerror(errno));
    }
    return 0;
}
    ///scriem actiunile in log
    void log_action(const char *district, const char *user,int role,const char *action){
        char log_path[PATH_LEN];
        build_path(log_path,district,"logged_district");
        //verficam permisiunile: doar manag poate scrie in log
        if(!check_permission(log_path,role,1)){
            return;
        }
        int fl=open(log_path,O_WRONLY|O_APPEND);
        if (fl < 0) {
        fprintf(stderr, "WARNING: Cannot open log '%s': %s\n",
                log_path, strerror(errno));
        return;
        }

        //facem timestamp
        time_t now = time(NULL);
        char timebuf[64];
        struct tm *tm_info = localtime(&now);
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

        const char *role_str = (role == role_manager) ? "manager" : "inspector";

        char line[512];
        int len = snprintf(line, sizeof(line), "[%s] role=%s user=%s action=%s\n",
                       timebuf, role_str, user, action);
        write(fl, line, len);
        close(fl);
    }

    /** S_IRUSR = bit citire owner
    S_IWUSR = bit scriere owner
    S_IXUSR = bit executie owner
    S_IRGRP, S_IWGRP, S_IXGRP = la fel pentru group
    S_IROTH, S_IWOTH, S_IXOTH = la fel pentru others */

    void print_perms_symbolic(mode_t mode) {
    char perms[11];
    perms[0] = S_ISDIR(mode)  ? 'd' :
               S_ISLNK(mode)  ? 'l' : '-';
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';
    perms[10] = '\0';
    printf("%s", perms);
}

    
