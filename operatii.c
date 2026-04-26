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


int op_add(const char *district, const char *user, int role){
    if(verif_district(district,role)<0)
        return -1;
    
    char reports_path[PATH_LEN];
    build_path(reports_path,district,"reports.dat");

    //verificam daca avem permisiune de scriere
    if(!check_permission(reports_path,role,1)){
         fprintf(stderr, "ERROR: Role '%s' does not have write permission on reports.dat\n",
                (role == role_manager) ? "manager" : "inspector");
        return -1;
    }

    Report r;
    memset(&r,0,sizeof(Report));//init cu 0

    //citim datele de la tastatura
    printf("=== Add New Report for district '%s' ===\n", district);

    printf("GPS Latitude: ");
    if (scanf("%lf", &r.latitude)!=1) {
        fprintf(stderr, "ERROR: Invalid latitude\n");
        return -1;
    }

    printf("GPS Longitude: ");
    if (scanf("%lf", &r.longitude)!=1) {
        fprintf(stderr, "ERROR: Invalid longitude\n");
        return -1;
    }

    printf("Category (road/lighting/flooding/other): ");
    if (scanf("%31s", r.category)!=1) {
        fprintf(stderr, "ERROR: Invalid category\n");
        return -1;
    }

    printf("Severity (1=minor, 2=moderate, 3=critical): ");
    if (scanf("%d", &r.severity)!= 1 || r.severity<1 || r.severity>3) {
        fprintf(stderr, "ERROR: Severity must be 1, 2, or 3\n");
        return -1;
    }

    int ch;
    while ((ch = getchar())!='\n' && ch!=EOF);

    printf("Description: ");
    if (fgets(r.description, DESC_LEN, stdin)==NULL) {
        fprintf(stderr, "ERROR: Cannot read description\n");
        return -1;
    }

     /* Scoatem newline-ul de la fgets */
    r.description[strcspn(r.description, "\n")]='\0';

    /* Setam campurile automate */
    strncpy(r.inspector,user,NAME_LEN - 1);
    r.timestamp=time(NULL);

     /* Calculam ID-ul: numarul de records existente */
    struct stat st;
    if (stat(reports_path, &st)==0) {
        r.id=(int)(st.st_size / sizeof(Report));
    } else {
        r.id=0;
    }

    int fr=open(reports_path,O_WRONLY|O_APPEND|O_CREAT,PERM_REPORTS_DAT);
    if(fr<0){
        fprintf(stderr,"ERROR: Cannot open reports.dat: %s\n", strerror(errno));
        return -1;
    }

    //setam permisiunile corecte
    chmod(reports_path,PERM_REPORTS_DAT);
    ///scriem record-ul
    ssize_t written_rep=write(fr,&r,sizeof(Report)); 

    if(written_rep != sizeof(Report)){
        fprintf(stderr, "ERROR: Failed to write report\n");
        return -1;
    }

    printf("SUCCESS: Report #%d added to district '%s'\n", r.id, district);

    char cfg_path[PATH_LEN];
    build_path(cfg_path, district, "district.cfg");

    if (check_permission(cfg_path, role, 0)) {
        int cfg_fd=open(cfg_path, O_RDONLY);
        if (cfg_fd>=0) {
            char cfg_buf[64];
            memset(cfg_buf,0,sizeof(cfg_buf));
            read(cfg_fd, cfg_buf, sizeof(cfg_buf)-1);
            close(cfg_fd);

            int threshold=1;
            char *eq=strchr(cfg_buf, '=');
            if (eq) threshold=atoi(eq + 1);

            if (r.severity>=threshold) {
                printf("*** ESCALATION ALERT *** Report #%d in '%s': "
                    "severity %d >= threshold %d (category: %s)\n",
                    r.id, district, r.severity, threshold, r.category);
            }
        }
    }

    //logam actiunea
    char action_desc[256];
     snprintf(action_desc, sizeof(action_desc),
            "add report #%d category=%s severity=%d", r.id, r.category, r.severity);
    log_action(district,user,role,action_desc);

    return 0;

}

int op_list(const char *district, const char *user, int role) {
    char reports_path[PATH_LEN];
    build_path(reports_path, district, "reports.dat");

    /* Verificam permisiunea de citire */
    if (!check_permission(reports_path, role, 0)) {
        fprintf(stderr, "ERROR: No read permission on reports.dat\n");
        return -1;
    }

    int fd = open(reports_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot open reports.dat: %s\n", strerror(errno));
        return -1;
    }

    /* Obtinem informatiile despre fisier cu stat() */
    struct stat st;
    fstat(fd, &st); /* fstat() e ca stat() dar lucreaza cu fd in loc de path */

    /* Afisam informatiile fisierului */
    printf("=== District: %s ===\n", district);
    printf("File: reports.dat | Size: %ld bytes | Permissions: ",
           (long)st.st_size);
    print_perms_symbolic(st.st_mode);

    /* Afisam si last modification time */
    char timebuf[64];
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);
    printf(" | Last modified: %s\n", timebuf);

    int count = 0;
    Report r;
    ssize_t bytes_read;

    printf("\n%-5s %-20s %-12s %-10s %-8s %s\n",
           "ID", "Inspector", "Category", "Severity", "Timestamp", "Description");
    printf("%-5s %-20s %-12s %-10s %-8s %s\n",
           "---", "---------", "--------", "--------", "---------", "-----------");

    /* Citim records unul cate unul */
    while ((bytes_read = read(fd, &r, sizeof(Report))) == sizeof(Report)) {
        /* Formateaza timestamp-ul */
        char ts[32];
        struct tm *t = localtime(&r.timestamp);
        strftime(ts, sizeof(ts), "%m/%d %H:%M", t);

        const char *sev_str = (r.severity == 1) ? "minor" :
                              (r.severity == 2) ? "moderate" : "critical";

        printf("%-5d %-20s %-12s %-10s %-8s %.40s\n",
               r.id, r.inspector, r.category, sev_str, ts, r.description);
        count++;
    }

    close(fd);

    if (count == 0) {
        printf("(no reports in this district)\n");
    } else {
        printf("\nTotal: %d report(s)\n", count);
    }

    log_action(district, user, role, "list");
    return 0;
}

int op_view(const char *district, int report_id, const char *user, int role) {
    char reports_path[PATH_LEN];
    build_path(reports_path,district,"reports.dat");

    if (!check_permission(reports_path,role,0)) {
        fprintf(stderr, "ERROR: No read permission on reports.dat\n");
        return -1;
    }

    int fd = open(reports_path, O_RDONLY);
    if (fd<0) {
        fprintf(stderr, "ERROR: Cannot open reports.dat: %s\n", strerror(errno));
        return -1;
    }

    /* Verificam ca report_id e valid */
    struct stat st;
    fstat(fd,&st);
    int total = (int)(st.st_size / sizeof(Report));

    if (report_id<0 || report_id>=total) {
        fprintf(stderr, "ERROR: Report ID %d not found (total: %d)\n",
                report_id,total);
        close(fd);
        return -1;
    }

    /* Sarim direct la pozitia record-ului */
    off_t offset = (off_t)report_id * sizeof(Report);
    if (lseek(fd, offset,SEEK_SET)<0) {
        fprintf(stderr,"ERROR: lseek failed: %s\n",strerror(errno));
        close(fd);
        return -1;
    }

    Report r;
    if (read(fd,&r,sizeof(Report)) != sizeof(Report)) {
        fprintf(stderr, "ERROR: Failed to read report\n");
        close(fd);
        return -1;
    }
    close(fd);

    /* Afisam toate detaliile */
    char timebuf[64];
    struct tm *t = localtime(&r.timestamp);
    strftime(timebuf, sizeof(timebuf),"%Y-%m-%d %H:%M:%S",t);

    printf("=== Report #%d - District: %s ===\n",r.id,district);
    printf("Inspector  : %s\n",r.inspector);
    printf("Category   : %s\n",r.category);
    printf("Severity   : %d (%s)\n",r.severity,
           r.severity == 1 ? "minor" : r.severity == 2 ? "moderate" : "critical");
    printf("GPS        : %.6f, %.6f\n",r.latitude,r.longitude);
    printf("Timestamp  : %s\n",timebuf);
    printf("Description: %s\n",r.description);

    log_action(district,user,role,"view");
    return 0;
}

int op_update_threshold(const char *district, int value,
                        const char *user, int role) {
    if (role!=role_manager) {
        fprintf(stderr, "ERROR: Only managers can update threshold\n");
        return -1;
    }

    char cfg_path[PATH_LEN];
    build_path(cfg_path, district,"district.cfg");

    //verificam ca permisiunile sunt exact 640 
    struct stat st;
    if (stat(cfg_path,&st)<0) {
        fprintf(stderr,"ERROR: Cannot stat district.cfg: %s\n",strerror(errno));
        return -1;
    }

    //Extragem doar permission bits (ignoram type bits) 
    mode_t perms = st.st_mode & 0777;
    if (perms != PERM_DISTRICT_CFG) {
        fprintf(stderr,
                "ERROR: district.cfg permissions tampered! "
                "Expected 0%o, found 0%o. Refusing operation.\n",
                PERM_DISTRICT_CFG, perms);
        return -1;
    }

    //Acum scriem noul threshold 
    int fd=open(cfg_path,O_WRONLY | O_TRUNC);
    if (fd<0) {
        fprintf(stderr,"ERROR: Cannot open district.cfg for writing: %s\n",
                strerror(errno));
        return -1;
    }

    char buf[64];
    int len = snprintf(buf,sizeof(buf),"severity_threshold=%d\n",value);
    write(fd,buf,len);
    close(fd);

    printf("SUCCESS: Severity threshold updated to %d in district '%s'\n",
           value,district);

    char action_desc[64];
    snprintf(action_desc, sizeof(action_desc),
             "update_threshold value=%d",value);
    log_action(district,user,role,action_desc);

    return 0;
}
