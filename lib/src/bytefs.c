#include <stdio.h>
#include <string.h>

void readf(const char *path, char *out) {
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("readf: fopen");
        return;
    }

    size_t len = fread(out, 1, 1023, f);
    out[len] = '\0';

    fclose(f);
}
