#define HISTORY_COMMAND_MAX_LENGTH 100

#define CORRECTED_COMMAND_MAX_LENGTH 100

struct record {
    int size;
    int capacity;
    char ** data;
};