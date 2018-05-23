#include "types.h"
#include "user.h"
#include "fcntl.h"

#define IMPROVE_SH

#ifdef IMPROVE_SH
#include "record.h"
#endif //x IMPROVE_SH

// Parsed command representation
#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5

#define MAXARGS 10

struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

int fork1(void);  // Fork but panics on failure.
void panic(char*);
struct cmd *parsecmd(char*);

#ifdef IMPROVE_SH

#define COMMAND_NUMBER 22
#define COMMAND_MAX_LENGTH 32

char commands[COMMAND_NUMBER][COMMAND_MAX_LENGTH] = {
  "cat",  "echo",     "forktest",   "grep",   "init",
	"kill", "ln",       "ls",         "mkdir",  "rm",
	"sh",   "stressfs", "usertests",  "wc",     "zombie",
  "history",  "shell"
};

#define LEVENSHTEIN_BUFFER_SIZE COMMAND_MAX_LENGTH

float 
levenshtein(char *a, char *b) 
{
  uint a_length = strlen(a);
  uint b_length = strlen(b);
  uint cache[LEVENSHTEIN_BUFFER_SIZE] = { 0, };
  uint a_index = 0;
  uint b_index = 0;
  uint a_distance;
  uint b_distance;
  uint result;
  char code;

  if (a == b) {
    return 0;
  }
  if (a_length == 0) {
    return 1;
  }
  if (b_length == 0) {
    return 1;
  }
  while (a_index < a_length) {
    cache[a_index] = a_index + 1;
    a_index++;
  }
  while (b_index < b_length) {
    code = b[b_index];
    result = a_distance = b_index++;
    a_index = -1;

    while (++a_index < a_length) {
      b_distance = code == a[a_index] ? a_distance : a_distance + 1;
      a_distance = cache[a_index];

      cache[a_index] = result = a_distance > result
        ? b_distance > result
          ? result + 1
          : b_distance
        : b_distance > a_distance
          ? a_distance + 1
          : b_distance;
    }
  }
  return (float)result / (a_length < b_length ? b_length : a_length);
}

// Return COMMAND_NUMBER when there are no matching commands.
// Rerurn the order of command in commands[] when there is a best matching command.
uint 
correct_command(char * wrong_command) 
{
  uint i;
  uint order = 0;
  float current_levenshtein;
  float min_levenshtein = 1;

  for (i = 0; i < COMMAND_NUMBER; ++i) {
    current_levenshtein = levenshtein(wrong_command, commands[i]);
    if (current_levenshtein < min_levenshtein) {
      order = i;
      min_levenshtein = current_levenshtein;
    }
  }

  return min_levenshtein <= 0.5 ? order : COMMAND_NUMBER;
}

char corrected_command[CORRECTED_COMMAND_MAX_LENGTH] = "";

// Determine if ecmd->argv[0] is an abbreviation. 
// For example, his* is an abbreviation of history.
// Return COMMAND_NUMBER when there are no matching commands.
// Rerurn the order of command in commands[] when there is a best matching command.
uint 
check_command(char * raw_command) 
{
  uint raw_command_length = strlen(raw_command);
  if (raw_command_length < 2 || COMMAND_MAX_LENGTH < raw_command_length) {
    return COMMAND_NUMBER;
  }
  // abbreviation sign '*'
  if (raw_command[raw_command_length - 1] != '*') {
    return COMMAND_NUMBER;
  }
  for (uint i = 0; i < COMMAND_NUMBER; ++i) {
    uint current_command_length = strlen(commands[i]);
    int found = 1;
    for (uint j = 0; j < current_command_length && j < raw_command_length - 1; j++) {
      if (commands[i][j] != raw_command[j]) {
        found = 0;
        break;
      }
    }
    if (found) {
      return i;
    }
  }
  return COMMAND_NUMBER;
}

void init_record(struct record*);
void save_record(struct record*);
void read_record(struct record*);
void push_record(struct record*, char*);
void free_record(struct record*);
void print_record(struct record*, int);

#endif //x IMPROVE_SH

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit();
  
  switch(cmd->type){
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit();

#ifdef IMPROVE_SH

    // Determine if ecmd->argv[0] is an abbreviation. 
    // For example, his* is an abbreviation of history.
    uint order = check_command(ecmd->argv[0]);
    if (order != COMMAND_NUMBER) {
      int offset = 0;
      strcpy(corrected_command + offset, commands[order]);
      offset += strlen(commands[order]);
      corrected_command[offset++] = ' ';
      for (int i = 1; i < MAXARGS; ++i) {
        if (ecmd->argv[i] != 0) {
          strcpy(corrected_command + offset, ecmd->argv[i]);
          offset += strlen(ecmd->argv[i]);
          corrected_command[offset++] = ' ';
        }
      }
      corrected_command[offset - 1] = 0;
      printf(1, "%s\n", corrected_command);

      struct record sheet;
      read_record(&sheet);
      push_record(&sheet, corrected_command);
      save_record(&sheet);

      exec(commands[order], ecmd->argv);
    }
    else {
      exec(ecmd->argv[0], ecmd->argv);
    }

    // Try to correct command.
    order = correct_command(ecmd->argv[0]);
    if (order == COMMAND_NUMBER) { // failed.
      printf(2, "exec %s failed\n", ecmd->argv[0]);
      break;      
    }

    printf(2, "exec %s failed.\nif you want to exec %s, type \"yep\".\n", ecmd->argv[0], commands[order]);
    // Record corrected command to file.
    int offset = 0;
    strcpy(corrected_command + offset, commands[order]);
    offset += strlen(commands[order]);
    corrected_command[offset++] = ' ';
    for (int i = 1; i < MAXARGS; ++i) {
      if (ecmd->argv[i] != 0) {
        strcpy(corrected_command + offset, ecmd->argv[i]);
        offset += strlen(ecmd->argv[i]);
        corrected_command[offset++] = ' ';
      }
    }
    corrected_command[offset - 1] = '\n';
    // printf(1, "%s", corrected_command); // Check
    wrecord("record/command", corrected_command, CORRECTED_COMMAND_MAX_LENGTH);

#else // IMPROVE_SH
    
    exec(ecmd->argv[0], ecmd->argv);
    printf(2, "exec %s failed\n", ecmd->argv[0]);
    
#endif //x IMPROVE_SH

    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    close(rcmd->fd);
    if(open(rcmd->file, rcmd->mode) < 0){
      printf(2, "open %s failed\n", rcmd->file);
      exit();
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    if(fork1() == 0)
      runcmd(lcmd->left);
    wait();
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p) < 0)
      panic("pipe");
    if(fork1() == 0){
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if(fork1() == 0){
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait();
    wait();
    break;
    
  case BACK:
    bcmd = (struct backcmd*)cmd;
    if(fork1() == 0)
      runcmd(bcmd->cmd);
    break;
  }
  exit();
}

int
getcmd(char *buf, int nbuf)
{
  printf(2, "$ ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

void
panic(char *s)
{
  printf(2, "%s\n", s);
  exit();
}

int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

//PAGEBREAK!
// Constructors

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd*)cmd;
}
//PAGEBREAK!
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    printf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parsepipe(ps, es);
  while(peek(ps, es, "&")){
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }
  if(peek(ps, es, ";")){
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch(tok){
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    case '+':  // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    }
  }
  return cmd;
}

struct cmd*
parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if(!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if(!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  if(peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|)&;")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a')
      panic("syntax");
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if(argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// NUL-terminate all the counted strings.
struct cmd*
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    return 0;
  
  switch(cmd->type){
  case EXEC:
    ecmd = (struct execcmd*)cmd;
    for(i=0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;
    
  case LIST:
    lcmd = (struct listcmd*)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    nulterminate(bcmd->cmd);
    break;
  }
  return cmd;
}

#ifdef IMPROVE_SH

#define MAX_HISTORY 1000

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

void 
expand_sheet(struct record * sheet) {
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
print_record(struct record * sheet, int number_limit)
{
  int min = number_limit <= sheet->size ? number_limit : sheet->size;  
  for (int i = sheet->size - min; i < sheet->size; ++i) {
    printf(1, "  %d  %s\n", i + 1, sheet->data[i]);
  }
}

#endif //x IMPROVE_SH

int 
main(void) 
{
  struct record sheet;
  read_record(&sheet);

  rrecord("record/command", corrected_command, CORRECTED_COMMAND_MAX_LENGTH);
  printf(1, "%s", corrected_command); // Check
  for (int i = 0; i < CORRECTED_COMMAND_MAX_LENGTH; i++) {
    if (corrected_command[i] == '\n') {
      corrected_command[i] = 0;
      push_record(&sheet, corrected_command);
      save_record(&sheet);
      corrected_command[i] = '\n';
      break;
    }
  }
  // Leak.

  runcmd(parsecmd(corrected_command));
  exit();
}