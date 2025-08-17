#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define print(val) _Generic((val), \
    int: print_int, \
    double: print_double, \
    char*: print_str, \
    const char*: print_str \
)(val)

void print_int(int x) { printf("%d\n", x); }
void print_double(double x) { printf("%f\n", x); }
void print_str(const char *str) { printf("%s\n", str); }

char* input() {
    static char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = '\0';
        return buffer;
    }
    return "";
}

int string_to_int(const char *str) { return atoi(str); }

char* int_to_string(int value) {
    static char buffer[50];
    snprintf(buffer, sizeof(buffer), "%d", value);
    return buffer;
}

void perr(const char *msg) {
    perror(msg);
}
