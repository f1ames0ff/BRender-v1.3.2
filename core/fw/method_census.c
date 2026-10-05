#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BR_METHOD_CENSUS_BUCKETS 1024

typedef struct tBrMethodCensusEntry {
    const char* name;
    unsigned long long count;
} tBrMethodCensusEntry;

static tBrMethodCensusEntry entries[BR_METHOD_CENSUS_BUCKETS];
static int dump_registered;

static unsigned int census_hash(const char* name) {
    unsigned int hash = 2166136261u;
    while (*name != '\0') {
        hash ^= (unsigned char)*name++;
        hash *= 16777619u;
    }
    return hash;
}

static void census_dump(void) {
    const char* path = getenv("BR_METHOD_CENSUS_FILE");
    FILE* file;
    unsigned int i;

    if (path == NULL) {
        path = "br_method_census.txt";
    }
    file = fopen(path, "w");
    if (file == NULL) {
        return;
    }
    for (i = 0; i < BR_METHOD_CENSUS_BUCKETS; i++) {
        if (entries[i].name != NULL) {
            fprintf(file, "%llu %s\n", entries[i].count, entries[i].name);
        }
    }
    fclose(file);
}

void BrMethodCensusHit(const char* method) {
    unsigned int index = census_hash(method) & (BR_METHOD_CENSUS_BUCKETS - 1);

    if (!dump_registered) {
        dump_registered = 1;
        atexit(census_dump);
    }
    while (entries[index].name != NULL && strcmp(entries[index].name, method) != 0) {
        index = (index + 1) & (BR_METHOD_CENSUS_BUCKETS - 1);
    }
    if (entries[index].name == NULL) {
        entries[index].name = method;
    }
    entries[index].count++;
}
