#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

#define MAX 1000

void cp(char *path);

int 
main(int argc, char *argv[])
{
  if(argc != 3)
  {
    printf(1, "wrong! please input again\n");
    exit();
  }

  int source_file = open(argv[1], O_RDONLY);
  if(source_file == -1)
  {
      printf(1, "open source file failed\n");
      exit();
  }

  //judge souce_file is folder
  struct stat st;
  fstat(source_file, &st);
  if(st.type == T_DIR)
  {
      printf(1, "source_file is a directory\n");
      cp(argv[1]);
      printf(1, "the program can't open the file in the directory.\n");
      printf(1, "you have to copy them one by one.\n");
      exit();
  }

  char c[MAX] = {};
  strcpy(c, argv[2]);
  int len1 = strlen(argv[1]);
  int len2 = strlen(argv[2]);
  if(argv[2][len2-1] == '/')
  {
      int i ;
      for(i = len1 - 1; i >= 0; i--)
      {
          if(argv[1][i] == '/')
          {
              break;
          }
      }
      i++;
      strcpy(&c[len2], &argv[1][i]);
  }

  //
  int destination_file = open(c, O_WRONLY|O_CREATE);
  if(destination_file == -1)
  {
      printf(1, "open destination file failed\n");
      exit();
  }

  //copy
  char ch[MAX] = {};
  int len = 0;
  while((len = read(source_file, ch, MAX)) > 0)
  {
      write(destination_file, ch, len);
  }
  close(source_file);
  close(destination_file);
  exit();
}

char* 
p(char *path)
{
  char *c;
  for(c = path+strlen(path); c >= path && *c != '/'; c--)
  {
    ;
  }
  c++;

  if(strlen(c) >= DIRSIZ)
  {
      return c;
  }
  static char ch[DIRSIZ+1];
  memmove(ch, c, strlen(c));
  memset(ch+strlen(c), ' ', DIRSIZ-strlen(c));
  return ch;
}

void 
cp(char *path)
{
  char ch[MAX], *c;
  int file;
  struct dirent di;
  struct stat st;
  if((file = open(path, 0)) < 0)
  {
    printf(2, "can't open %s.\n", path);
    return;
  }
  if(fstat(file, &st) < 0)
  {
    printf(2, "can't stat %s. \n", path);
    close(file);
    return;
  }
  switch(st.type)
  {
    case T_FILE:
    printf(1, "name = %s, type = file\n", p(path));
    break;

    case T_DIR:
    if(strlen(path)+1+DIRSIZ+1 > sizeof(ch))
    {
      printf(1, "path too long\n");
      break;
    }
    strcpy(ch, path);
    c = ch + strlen(ch);
    *c++ = '/';

    while(read(file, &di, sizeof(di)) == sizeof(di))
    {
      if(di.inum == 0)
        continue;
      memmove(c, di.name, DIRSIZ);
      c[DIRSIZ] = 0;
      if(stat(ch, &st) < 0)
      {
        printf(1, "can't stat &s\n", ch);
        continue;
      }
      printf(1, "name = %s, type = directory\n", p(ch));
    }
    break;
  }
  close(file);
}