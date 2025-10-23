#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>

#define MAX_USERNAME 32
#define MAX_PASSWORD 32
#define USER_FILE "data/users.txt"

// Function declarations
bool login_user(const char *username, const char *password);
bool register_user(const char *username, const char *password);

#endif
