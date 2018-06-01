#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

#define MAX 256

void ls(char *path);
char *fmtname(char *path);

int 
main(int argc, char *argv[])
{
  if(argc != 3)
  {
      printf(1, "wrong! please input again\n");
      exit();
  }

  int fd_source = open(argv[1], O_RDONLY);
  if(fd_source == -1)
  {
      printf(1, "open source file failed\n");
      exit();
  }

  //judge souce_file is folder
  struct stat st;
  fstat(fd_source, &st);
  if(st.type == T_DIR)
  {
      printf(1, "source_file is a directory\n");
      ls(argv[1]);
      printf(1, "the program can't open the file in the directory.\n");
      printf(1, "you have to copy them one by one.\n");
      exit();
  }

  char com[128] = {};
  strcpy(com, argv[2]);
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
      strcpy(&com[len2], &argv[1][i]);
  }

  //
  int fd_des = open(com, O_WRONLY|O_CREATE);
  if(fd_des == -1)
  {
      printf(1, "open destination file failed\n");
      exit();
  }

  //copy
  char ch[MAX] = {};
  int len = 0;
  while((len = read(fd_source, ch, MAX)) > 0)
  {
      write(fd_des, ch, len);
  }
  close(fd_source);
  close(fd_des);
  if(unlink(argv[1]) < 0)
  {
      printf(1, "sorry, you can't delete source file");
  }
  exit();
}

char* 
fmtname(char *path)
{
  static char ch[DIRSIZ+1];
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
  memmove(ch, c, strlen(c));
  memset(ch+strlen(c), ' ', DIRSIZ-strlen(c));
  return ch;
}

void 
ls(char *path)
{
  char ch[512], *c;
  int fd;
  struct dirent di;
  struct stat st;
  if((fd = open(path, 0)) < 0)
  {
    printf(2, "can't open %s.\n", path);
    return;
  }
  if(fstat(fd, &st) < 0)
  {
    printf(2, "can't stat %s. \n", path);
    close(fd);
    return;
  }
  switch(st.type)
  {
    case T_FILE:
    printf(1, "name = %s, type = file, size = %d\n", fmtname(path), st.size);
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

    while(read(fd, &di, sizeof(di)) == sizeof(di))
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
      printf(1, "name = %s, type = directory, size = %d\n", fmtname(ch), st.size);
    }
    break;
  }
  close(fd);
}