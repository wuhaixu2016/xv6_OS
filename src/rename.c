#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int 
main(int argc, char * argv[])
{
  if (argc != 3)
  {
    printf(2, "usage: 'rename oldpath newpath'\n" );
    exit();
  }
  
  char * oldpath = argv[1];
  char * newpath = argv[2];
  int ret = rename(oldpath, newpath);
  if(ret < 0)
  {
    if(ret == -1) {
      printf(2, "error: '%s' does not exist.. \n", oldpath);
    }
    else if(ret == -2) {
      printf(2, "error: cannot rename '%s' to '%s'.. \n", oldpath, newpath);
    }
  }
  else {
    printf(1, "rename '%s' to '%s' successfully!\n", oldpath, newpath);
  }
  exit();
}