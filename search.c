#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

void SearchFile(char *filePath, char *fileName);
#define MAX 1000

int
main(int argc, char *argv[])
{
  if(argc < 2)
  {
    printf(1, "wrong! please input again\n");
    exit();
  }
  SearchFile(" ",argv[1]);
  exit();
}

int compare(char *ch, char *ch2)
{
  if(strlen(ch) != strlen(ch2))
    return -1;
  for(int i = 0; i < strlen(ch); i++)
  {
    if(ch[i] != ch2[i])
     return -1;
  }
  return 0;
}

char* get(char *filePath, char *fileName)
{
  int len = strlen(filePath);
  int j, i = 0;
  for(i = 0; i < len; i++)
  {
    for(j = i; j < len; j++)
    {
      if((filePath[j]=='.') || (filePath[j]=='/'))
        break;
    }
    if(j == len)
      break;
  }
  if(i == len)
    return ".";
  for(j = 0; i < len-i; j++)
    fileName[j] = filePath[j+i];
  fileName[len-i]=0;
  return fileName;
}
	
void SearchFile(char *filePath, char *fileName)
{
  int file;
  if((file=open(filePath,0)) < 0)
  {
    printf(2, "sorry, can't open %s\n", filePath);
    return;
  }
  char ch[512], *c;
  struct dirent di;
  struct stat st;
  if(fstat(file,&st) < 0)
  {
    printf(2, "sorry, can't open %s\n", filePath);
    close(file);
    return;
  }
  switch(st.type)
  {
    case T_FILE:
    {
      char name[MAX];
      if(compare(get(filePath,name),fileName)==0)
      {
        printf(1,"path:%s size:%d\n", fileName, filePath);

      }
      if((st.type == 1) && (compare(get(ch, name),".")!=0)&&(compare(get(ch, name),"..")!=0))
        SearchFile(get(filePath,name),fileName);
      break;
    }

    case T_DIR:
    if(strlen(filePath)+1+DIRSIZ+1 > sizeof(ch))
    {
      printf(1, "path too long\n");
      break;
    }
    strcpy(ch, filePath);
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
      char name[MAX];
      if(compare(get(ch,name),fileName)==0)
        printf(1,"path:%s size:%d\n",fileName,ch);
      if((st.type == 1) && (compare(get(ch, name),".")!=0)&&(compare(get(ch, name),"..")!=0))
      {
        SearchFile(fileName,get(filePath,name));
      }
    }
    break;
  }
  close(file);
}
