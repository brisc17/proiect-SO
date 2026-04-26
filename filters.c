#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "city_manager.h"

/* ============================================================
 * parse_condition()
 *
 * Parseaza un string de forma "field:operator:value" in
 * cele trei componente separate.
 *
 * Exemple:
 *   "severity:>=:2"   => field="severity", op=">=", value="2"
 *   "category:==:road" => field="category", op="==", value="road"
 *
 * Aceasta functie a fost generata cu asistenta AI (Claude),
 * descriind structura Record si cerintele de parsing.
 * Am verificat fiecare linie si am inteles logica inainte
 * de a o include in proiect.
 *
 * Returneza 1 daca parsing-ul a reusit, 0 altfel.
 * ============================================================ */
int parse_condition(const char *input, char *field, char *op, char *value) {
    if (!input || !field || !op || !value) return 0;

    /* Facem o copie pentru a nu modifica stringul original */
    char buf[256];
    strncpy(buf, input, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* Cautam primul ':' pentru a separa field */
    char *first_colon = strchr(buf, ':');
    if (!first_colon) return 0; /* nu e format valid */

    *first_colon = '\0'; /* terminam field-ul */
    strncpy(field, buf,NAME_LEN - 1);
    field[NAME_LEN - 1] = '\0';

    /* Restul e "operator:value" */
    char *rest = first_colon + 1;

    /* Operatorii pot fi: ==, !=, <=, >=, <, >
     * Cautam al doilea ':' pentru a separa operatorul de valoare
     * Dar operatorul poate fi 1 sau 2 caractere, asa ca cautam ':' */
    char *second_colon = strchr(rest, ':');
    if (!second_colon) return 0;

    *second_colon = '\0';
    strncpy(op, rest, 3); /* max 2 chars operator + null */
    op[3] = '\0';

    strncpy(value, second_colon + 1, DESC_LEN - 1);
    value[DESC_LEN - 1] = '\0';

    /* Validam ca field, op si value nu sunt goale */
    if (strlen(field) == 0 || strlen(op) == 0 || strlen(value) == 0) {
        return 0;
    }

    return 1;
}

/* ============================================================
 * match_condition()
 *
 * Verifica daca un Record satisface o conditie data.
 *
 * Campuri suportate: severity, category, inspector, id
 *
 * Operatori suportati:
 *   == (egal)
 *   != (diferit)
 *   <, >, <=, >= (comparatii numerice pentru severity si id)
 *
 * Aceasta functie a fost generata cu asistenta AI, descriind
 * structura Report si tipul fiecarui camp.
 * Am adaugat manual validarea operatorilor invalizi.
 *
 * Returneaza 1 daca record-ul satisface conditia, 0 altfel.
 * ============================================================ */
int match_condition(Report *r, const char *field, const char *op,
                    const char *value) {
    if (!r || !field || !op || !value) return 0;

    /* ---- Campuri de tip STRING: category, inspector ---- */
    if (strcmp(field, "category") == 0) {
        int cmp = strcmp(r->category, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
        fprintf(stderr, "WARNING: Operator '%s' not valid for string field\n", op);
        return 0;
    }

    if (strcmp(field, "inspector") == 0) {
        int cmp = strcmp(r->inspector, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
        fprintf(stderr, "WARNING: Operator '%s' not valid for string field\n", op);
        return 0;
    }

    /* ---- Campuri de tip INTEGER: severity, id ---- */
    if (strcmp(field, "severity") == 0 || strcmp(field, "id") == 0) {
        int record_val = (strcmp(field, "severity") == 0) ? r->severity : r->id;
        int cond_val   = atoi(value);

        if (strcmp(op, "==") == 0) return record_val == cond_val;
        if (strcmp(op, "!=") == 0) return record_val != cond_val;
        if (strcmp(op, "<")  == 0) return record_val <  cond_val;
        if (strcmp(op, ">")  == 0) return record_val >  cond_val;
        if (strcmp(op, "<=") == 0) return record_val <= cond_val;
        if (strcmp(op, ">=") == 0) return record_val >= cond_val;

        fprintf(stderr, "WARNING: Unknown operator '%s'\n", op);
        return 0;
    }

    fprintf(stderr, "WARNING: Unknown field '%s'\n", field);
    return 0;
}

/* ============================================================
 * op_filter()
 *
 * Filtreaza si afiseaza rapoartele care satisfac TOATE
 * conditiile date (AND logic intre conditii).
 *
 * Aceasta functie e scrisa manual (nu AI-assisted).
 *
 * Fluxul:
 * 1. Parsam fiecare conditie din argv[]
 * 2. Citim reports.dat record cu record
 * 3. Pentru fiecare record, testam toate conditiile
 * 4. Afisam daca TOATE conditiile sunt satisfacute (AND)
 * ============================================================ */
int op_filter(const char *district, int argc, char *argv[],
              int cond_start, const char *user, int role) {
    char reports_path[PATH_LEN];
    build_path(reports_path, district, "reports.dat");

    if (!check_permission(reports_path, role, 0)) {
        fprintf(stderr, "ERROR: No read permission on reports.dat\n");
        return -1;
    }

    /* Parsam toate conditiile */
    int num_conds = argc - cond_start;
    if (num_conds <= 0) {
        fprintf(stderr, "ERROR: No filter conditions provided\n");
        return -1;
    }

    /* Alocam arrays pentru field/op/value */
    char fields[16][NAME_LEN];
    char ops[16][4];
    char values[16][DESC_LEN];

    if (num_conds > 16) num_conds = 16; /* limita maxima */

    int valid_conds = 0;
    for (int i = 0; i < num_conds; i++) {
        if (parse_condition(argv[cond_start + i],
                            fields[valid_conds],
                            ops[valid_conds],
                            values[valid_conds])) {
            valid_conds++;
        } else {
            fprintf(stderr, "WARNING: Invalid condition '%s' - skipping\n",
                    argv[cond_start + i]);
        }
    }

    if (valid_conds == 0) {
        fprintf(stderr, "ERROR: No valid conditions\n");
        return -1;
    }

    /* Deschidem fisierul si citim records */
    int fd = open(reports_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot open reports.dat: %s\n", strerror(errno));
        return -1;
    }

    printf("=== Filter results for district '%s' ===\n", district);

    int match_count = 0;
    Report r;
    ssize_t bytes_read;

    while ((bytes_read = read(fd, &r, sizeof(Report))) == sizeof(Report)) {
        /* Testam TOATE conditiile (AND logic) */
        int all_match = 1;
        for (int i = 0; i < valid_conds; i++) {
            if (!match_condition(&r, fields[i], ops[i], values[i])) {
                all_match = 0;
                break;
            }
        }

        if (all_match) {
            char ts[32];
            struct tm *t = localtime(&r.timestamp);
            strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M", t);

            printf("ID:%-4d | %-20s | %-12s | sev:%d | %s | %s\n",
                   r.id, r.inspector, r.category,
                   r.severity, ts, r.description);
            match_count++;
        }
    }

    close(fd);

    if (match_count == 0) {
        printf("(no reports match the given conditions)\n");
    } else {
        printf("\nMatched: %d report(s)\n", match_count);
    }

    log_action(district, user, role, "filter");
    return 0;
}
