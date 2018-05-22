#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "record.h"

void init_record(struct record*);
void save_record(struct record*);
void read_record(struct record*);
void push_record(struct record*, char*);
void free_record(struct record*);
void print_record(struct record*);

// TODO: Limit the number of history records.
int main(int argc, char* argv[]) { 
  struct record sheet;
  read_record(&sheet);
  print_record(&sheet);
  free_record(&sheet);
  exit();
}

char * buffer = 0;

void 
init_record(struct record * sheet)
{
  sheet->size = 0;
  sheet->capacity = 4;
  sheet->data = malloc(sheet->capacity * sizeof(char *));
  for (int i = 0; i < sheet->capacity; ++i) {
    sheet->data[i] = malloc(HISTORY_COMMAND_MAX_LENGTH * sizeof(char));
  }
}

void 
save_record(struct record * sheet) 
{
  wrecord("record/history_size", &(sheet->size), sizeof(int));
  wrecord("record/history_capacity", &(sheet->capacity), sizeof(int));  
  // printf(1, "  size:%d  capacity:%d\n", sheet->size, sheet->capacity);
  
  // Write records into buffer.
  const uint write_size = sheet->capacity * HISTORY_COMMAND_MAX_LENGTH * sizeof(char);
  buffer = malloc(write_size);
  int offset = 0;
  for (int i = 0; i < sheet->capacity; ++i) {
    strcpy(buffer + offset, sheet->data[i]);
    offset += HISTORY_COMMAND_MAX_LENGTH;
  }
  // Write records into file.
  wrecord("record/history_data", buffer, write_size);
}

void 
read_record(struct record * sheet)
{
  rrecord("record/history_size", &(sheet->size), sizeof(int));
  rrecord("record/history_capacity", &(sheet->capacity), sizeof(int));
  // printf(1, "  size:%d  capacity:%d\n", sheet->size, sheet->capacity);

  // Read records from file.
  if (buffer) {
    free(buffer);
  }
  const uint read_size = sheet->capacity * HISTORY_COMMAND_MAX_LENGTH * sizeof(char);
  buffer = malloc(read_size);
  rrecord("record/history_data", buffer, read_size);

  // Write records into sheet.
  int offset = 0;
  sheet->data = malloc(sheet->capacity * sizeof(char *));
  for (int i = 0; i < sheet->capacity; ++i) {
    sheet->data[i] = malloc(HISTORY_COMMAND_MAX_LENGTH * sizeof(char)); 
    char t = buffer[offset + HISTORY_COMMAND_MAX_LENGTH - 1];
    buffer[offset + HISTORY_COMMAND_MAX_LENGTH - 1] = 0;
    strcpy(sheet->data[i], buffer + offset);
    buffer[offset + HISTORY_COMMAND_MAX_LENGTH - 1] = t;
    offset += HISTORY_COMMAND_MAX_LENGTH;
  }

  free(buffer);
}

void expand_sheet(struct record * sheet) {
  if (sheet->size < sheet->capacity) {
    return;
  }
  char ** old = sheet->data;
  sheet->capacity <<= 1;
  sheet->data = malloc(sheet->capacity * sizeof(char *));
  for (int i = 0; i < sheet->size; ++i) {
    sheet->data[i] = old[i];
  }
  for (int i = sheet->size; i < sheet->capacity; ++i) {
    sheet->data[i] = malloc(HISTORY_COMMAND_MAX_LENGTH * sizeof(char));
  }
}

void 
push_record(struct record * sheet, char * line)
{
  expand_sheet(sheet);
  strcpy(sheet->data[sheet->size++], line);
  // printf(1, "  push_history: %s\n", sheet->data[sheet->size - 1]);
}

void 
free_record(struct record * sheet)
{
  for (int i = 0; i < sheet->capacity; ++i) {
    free(sheet->data[i]);
  }
  free(sheet->data);

  if (buffer) {
    free(buffer);
  }
}

void 
print_record(struct record * sheet)
{
  for (int i = 0; i < sheet->size; ++i) {
    printf(1, "  %d  %s\n", i + 1, sheet->data[i]);
  }
}