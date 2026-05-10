#ifndef city_manager_h
#define city_manager_h

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define NAME_LEN 64
#define CATEGORY_LEN 32
#define DESC_LEN 256
#define DISTRICT_LEN 64
#define PATH_LEN 512

typedef struct {
    int     id;                         /* ID-ul raportului (index 0-based) */
    char    inspector[NAME_LEN];    /* numele inspectorului             */
    double  latitude;                   /* coordonata GPS                   */
    double  longitude;                  /* coordonata GPS                   */
    char    category[CATEGORY_LEN]; /* tipul problemei                  */
    int     severity;                   /* 1=minor, 2=moderate, 3=critical  */
    time_t  timestamp;                  /* cand a fost raportat             */
    char    description[DESC_LEN];  /* descrierea problemei             */
} Report;

#define role_inspector 0
#define role_manager 1

/* 
 * PERMISIUNI (octal)
  0750 = rwxr-x--- (director: manager full, inspector read+exec)
  0664 = rw-rw-r-- (reports.dat: ambii pot citi si scrie)
  0640 = rw-r----- (district.cfg: manager scrie, inspector citeste)
  0644 = rw-r--r-- (logged_district: toti citesc, doar manager scrie)
 */
#define PERM_DISTRICT_DIR   0750
#define PERM_REPORTS_DAT    0664
#define PERM_DISTRICT_CFG   0640
#define PERM_LOGGED         0644

///operatii
int  op_add(const char *district,const char *user,int role);
int  op_list(const char *district,const char *user,int role);
int  op_view(const char *district,int report_id,const char *user,int role);
int  op_update_threshold(const char *district,int value,const char *user,int role);
int  op_filter(const char *district,int argc,char *argv[],int cond_start,
               const char *user,int role);
int op_remove_report(const char *district, int report_id,const char *user, int role);
int op_remove_district(const char *district, const char *user, int role);


///fct de utilizare
void build_path(char *out,const char *district,const char *filename);
int  verif_district(const char *district,int role);
int  check_permission(const char *path,int role,int need_write);
void log_action(const char *district,const char *user,int role,const char *action);
void print_perms_symbolic(mode_t mode);

///fct facute cu ai
int parse_condition(const char *input,char *field,char *op,char *value);
int match_condition(Report *r,const char *field,const char *op,const char *value);
 
#endif