#include "auth.h"
#include <stdio.h>
#include <string.h>

bool login_user(const char *username, const char *password) {
    FILE *fp = fopen(USER_FILE, "r");
    if (!fp) return false;

    char line[100];
    char file_user[MAX_USERNAME], file_pass[MAX_PASSWORD];

    while (fgets(line, sizeof(line), fp)) {
        sscanf(line, "%31[^:]:%31s", file_user, file_pass);
        if (strcmp(username, file_user) == 0 && strcmp(password, file_pass) == 0) {
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}

bool register_user(const char *username, const char *password) {
    // Check if username exists
    if (login_user(username, password)) return false; // already exists

    FILE *fp = fopen(USER_FILE, "a");
    if (!fp) return false;

    fprintf(fp, "%s:%s\n", username, password);
    fclose(fp);
    return true;
}

