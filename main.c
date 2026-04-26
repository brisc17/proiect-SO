#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "city_manager.h"

//print_usage()=afiseaza instructiunile de utilizare
static void print_usage(const char *prog) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s --role <inspector|manager> --user <name> --add <district>\n", prog);
    fprintf(stderr, "  %s --role <inspector|manager> --user <name> --list <district>\n", prog);
    fprintf(stderr, "  %s --role <inspector|manager> --user <name> --view <district> <report_id>\n", prog);
    fprintf(stderr, "  %s --role manager            --user <name> --remove_report <district> <report_id>\n", prog);
    fprintf(stderr, "  %s --role manager            --user <name> --update_threshold <district> <value>\n", prog);
    fprintf(stderr, "  %s --role <inspector|manager> --user <name> --filter <district> <cond1> [cond2...]\n", prog);
    fprintf(stderr, "\nCondition format: field:operator:value\n");
    fprintf(stderr, "  Fields: severity, category, inspector, id\n");
    fprintf(stderr, "  Operators: ==, !=, <, >, <=, >=\n");
    fprintf(stderr, "  Example: severity:>=:2 category:==:road\n");
}


int main(int argc, char *argv[]){
    const char *role_extras=NULL;
    const char *user_extras=NULL;
    const char *operation=NULL;
    int op_arg_poz=-1;

    for(int i=1;i<argc;i++){
        if (strcmp(argv[i], "--role") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: --role requires an argument\n");
                return 1;
            }
            role_extras = argv[++i];
        }
        else if (strcmp(argv[i], "--user") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: --user requires an argument\n");
                return 1;
            }
            user_extras = argv[++i];
        }
        else if (strcmp(argv[i], "--add") == 0 ||
                 strcmp(argv[i], "--list") == 0 ||
                 strcmp(argv[i], "--view") == 0 ||
                 strcmp(argv[i], "--remove_report") == 0 ||
                 strcmp(argv[i], "--update_threshold") == 0 ||
                 strcmp(argv[i], "--filter") == 0) {
            //salvam operatia si indexul primului argument al ei 
            operation  = argv[i] + 2; //skip "--" 
            op_arg_poz = i + 1;
            break; //restul argumentelor sunt pentru operatie 
        }
        else {
            fprintf(stderr,"ERROR: Unknown argument '%s'\n",argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    //validam argumentele obligatorii
    if (!role_extras) {
        fprintf(stderr,"ERROR: --role is required\n");
        print_usage(argv[0]);
        return 1;
    }
    if (!user_extras) {
        fprintf(stderr,"ERROR: --user is required\n");
        print_usage(argv[0]);
        return 1;
    }
    if (!operation) {
        fprintf(stderr,"ERROR: No operation specified\n");
        print_usage(argv[0]);
        return 1;
    }

    int role;
    if (strcmp(role_extras,"manager") == 0) {
        role = role_manager;
    } else if (strcmp(role_extras,"inspector") == 0) {
        role = role_inspector;
    } else {
        fprintf(stderr,"ERROR: Invalid role '%s' (use 'manager' or 'inspector')\n",
            role_extras);
        return 1;
    }
   

    //dispatch: apelam operatia corecta

    //--add <district> 
    if (strcmp(operation, "add") == 0) {
        if (op_arg_poz >= argc) {
            fprintf(stderr, "ERROR: --add requires <district>\n");
            return 1;
        }
        return op_add(argv[op_arg_poz], user_extras, role);
    }

    //--list <district> 
    if (strcmp(operation, "list") == 0) {
        if (op_arg_poz >= argc) {
            fprintf(stderr, "ERROR: --list requires <district>\n");
            return 1;
        }
        return op_list(argv[op_arg_poz], user_extras, role);
    }

    //--view <district> <report_id>
    if (strcmp(operation, "view") == 0) {
        if (op_arg_poz + 1 >= argc) {
            fprintf(stderr, "ERROR: --view requires <district> <report_id>\n");
            return 1;
        }
        int report_id = atoi(argv[op_arg_poz + 1]);
        return op_view(argv[op_arg_poz], report_id, user_extras, role);
    }

    //--update_threshold <district> <value> 
    if (strcmp(operation, "update_threshold") == 0) {
        if (op_arg_poz + 1 >= argc) {
            fprintf(stderr, "ERROR: --update_threshold requires <district> <value>\n");
            return 1;
        }
        int value = atoi(argv[op_arg_poz + 1]);
        return op_update_threshold(argv[op_arg_poz], value, user_extras, role);
    }

    //--filter <district> <cond1> [cond2...] 
    if (strcmp(operation, "filter") == 0) {
        if (op_arg_poz>=argc) {
            fprintf(stderr, "ERROR: --filter requires <district> <condition(s)>\n");
            return 1;
        }
        //districtul e primul argument dupa --filter
        //conditiile incep de la op_arg_idx + 1 
        if (op_arg_poz+1>=argc) {
            fprintf(stderr, "ERROR: --filter requires at least one condition\n");
            return 1;
        }
        return op_filter(argv[op_arg_poz], argc, argv,
                         op_arg_poz + 1, user_extras, role);
    }

    fprintf(stderr,"ERROR: Unknown operation '%s'\n",operation);
    print_usage(argv[0]);
    return 1;

}