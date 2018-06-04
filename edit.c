#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "x86.h"

#define MAX_LINE_NUM 24
#define KEY_UP 0xE2
#define KEY_DOWN 0xE3
#define KEY_LEFT 0xE4
#define KEY_RIGHT 0xE5
#define KEY_ESC 27
#define CRTPORT 0x3d4
#define PAR_SIZE 2048
#define MAX_OUTPUT 80*(MAX_LINE_NUM-1) // last line to print status

#define VIS_MODE 0
#define EDT_MODE 1
#define MV_U 17
#define MV_D 18
#define MV_L 19
#define MV_R 20
char move_u = 17;
char move_d = 18;
char move_l = 19;
char move_r = 20;

int new_file;

struct par
{
  struct par* before;
  struct par* next;
  int size;
  char content[PAR_SIZE];
};

struct par*
add_back(struct par* front)
{
  struct par* new_par;
  new_par = (struct par*)sbrk(sizeof(struct par));
  memset(new_par->content, 0, PAR_SIZE);

  new_par->size = 0;
  new_par->before = front;
  new_par->next = front->next;
  front->next = new_par;

  if (new_par -> next != 0) {
    new_par->next->before = new_par;
  }
  return new_par;
}

struct par first_par;
struct par* head_par;
struct par* current_par;
struct par* screen_par;
int edit_index;

int mode;
int buf_cnt;
char buf[196];
char trans[PAR_SIZE];

char* file_name;

int LR_cnt; // +: left, -: right
int UD_cnt; // +: up, -: down

char str[PAR_SIZE];//record string
int strLen;

int delnum; //delete num 

int
lines(struct par* para)
{
  return (para->size / 80) + 1;
}

int
open_file()
{
  int fd;
  fd = open(file_name, O_RDWR);
  if(fd < 0){
    new_file = 1;
    printf(1, "vim: created new file %s.\n", file_name);
    fd = open(file_name, O_CREATE|O_RDWR);
  }
  return fd;
}

void
putc(char c)
{
  write(1, &c, 1);
}

void
save()
{
  int fd;
  char is_first;
  struct par* save_par;
  is_first = 1;
  fd = open_file(file_name);
  save_par = head_par;
  while (1) {
    if (save_par == 0) {
      break;
    }
    if (!is_first) {
      write(fd, "\n", 1);
    }
    if (is_first) {
      is_first = 0;
    }
    write(fd, save_par->content, save_par->size);
    save_par = save_par->next;
  }
  close(fd);
}

void
wash_buf()
{
  strLen = 0;
  delnum = 0;
  int i;
  int ch;
  LR_cnt = 0;
  UD_cnt = 0;
  for (i = 0; i < buf_cnt; ++i) 
  {
    ch = buf[i] & 0xff;
    switch (ch) 
    {
      case KEY_RIGHT:
       --LR_cnt;
       break;
      case KEY_LEFT:
        ++LR_cnt;
        break;
      case KEY_DOWN:
        --UD_cnt;
        break;
      case KEY_UP:
        ++UD_cnt;
        break;
      case '@':
      	delnum++;
      	break;
      case 'e':
      case 's':
      case 'q':
        if (mode == VIS_MODE)
        {
          goto rewrite;
        }
        else
        {
          str[strLen] = ch;
          strLen ++;
        }
        break;
      default:
          str[strLen] = ch;
          strLen ++;
        break;
      }
    }
  ch = buf[i-1] & 0xff;

rewrite:
  buf_cnt = 0;
  if (LR_cnt > 0) 
  {
    for (i = 0; i < LR_cnt; ++i, ++buf_cnt) 
    {
      buf[buf_cnt] = move_l;
    }
  }
  else 
  {
    for (i = 0; i < -LR_cnt; ++i, ++buf_cnt) 
    {
      buf[buf_cnt] = move_r;
    }
  }
  if (UD_cnt > 0) 
  {
    for (i = 0; i < UD_cnt; ++i, ++buf_cnt) 
    {
      buf[buf_cnt] = move_u;
    }
  }
  else 
  {
    for (i = 0; i < -UD_cnt; ++i, ++buf_cnt) 
    {
      buf[buf_cnt] = move_d;
    }
  }
  switch (ch) 
  {
    case 'e':
      buf[buf_cnt++] = 'e';
      break;
    case 's':
      buf[buf_cnt++] = 's';
      break;
    case 'q':
      buf[buf_cnt++] = 'q';
    default:
      break;
  }
}

void
load_file(int fd)
{
  int n;
  int i;
  char buf[128];
  if (new_file) {
    edit_index = 0;
    return;
  }
  while(1) {
    n = read(fd, buf, 128);
    if (n <= 0) {
      break;
    }

    for (i = 0; i < n; ++i) {
      if (buf[i] == '\n') {
        current_par = add_back(current_par);
        edit_index = 0;
      }
      else {
        current_par->content[edit_index++] = buf[i];
        ++(current_par->size);
      }
    }
  }
  --edit_index;
}

void
output(char* str, int length)
{
  write(1, str, length);
  edit_index += length;
  int i;
  for (i = 0; i < length; ++i) {
    putc(move_r);
  }
}

void
m_clear()
{
  int i;  
  for (i = 0; i < MAX_LINE_NUM + 2; ++i) {
    putc(move_d);
  }
  for (i = 0; i < MAX_LINE_NUM + 2; ++i) {
    putc('\n');
  }
}

void
update_output()
{
  // clear up the screen
  int i;
  int j;
  m_clear();

  // print mode info
  switch(mode) {
    case VIS_MODE:
      printf(1, "VISUAL MODE");
      for (j = 0; j < 11; ++j) {
        putc(move_l);
      }
      putc('\n');
      break;
    case EDT_MODE:
      printf(1, "EDIT MODE");
      for (j = 0; j < 9; ++j) {
        putc(move_l);
      }
      putc('\n');
      break;
    default:
      break;
  }

  // move to the top
  for (i = 0; i < MAX_LINE_NUM + 2; ++i) {
    putc(move_u);
  }

  // print out the content
  struct par* print_par;
  struct par* print_tmppar;
  print_par = screen_par;
  print_tmppar = screen_par;
  int cnt;
  cnt = 0;
  while(1) {
    if (print_par == 0) {
      break;
    }
    if (cnt != 0) {
      putc('\n');
    }
    for (i = 0; i < print_par->size; ++i) {
      putc((print_par->content)[i]);
    }
    ++cnt;
    print_par = print_par->next;
  }

  // move the cursor from anywhere to left top
  for (i = 0; i < MAX_LINE_NUM + 1; ++i) {
    putc(move_u);
  }
  for (i = 0; i < 81; ++i) {
    putc(move_l);
  }

  // move the cursor to the current par
  print_par = screen_par;
  while(print_par != current_par) {
    for (i = 0; i < lines(print_par); ++i) {
      putc(move_d);
    }
    print_par = print_par->next;
  }

  // move the cursor the current

  cnt = 0;
  while(1) {
    if (print_tmppar == 0) {
      break;
    }
    if (cnt != 0) {
      putc('\n');
    }
    for (i = 0; i < print_tmppar->size; ++i) {
      putc((print_tmppar->content)[i]);
    }
    ++cnt;
    print_tmppar = print_tmppar->next;
  }
}

void
visual_mode()
{
  memset(buf, 0, 128);
  int i;
  while(1) {
    buf_cnt = read(0, buf, 128);
    if (buf_cnt < 0) {
      break;
    }
    wash_buf();
    for (i = 0; i < buf_cnt; ++i) {
      switch (buf[i]) {
        case MV_R:
          if (edit_index + 1 <= current_par->size) {
            ++edit_index;
          }
          else if (current_par -> next != 0) {
            current_par = current_par->next;
            edit_index = 0;
          }
          else {
            putc(move_l); // ensure the cursor won't move beyond the content
          }
          break;
        case MV_L:
          if (edit_index - 1 >= 0) {
            --edit_index;
          }
          else if (current_par -> before != 0) {
            current_par = current_par->before;
            edit_index = current_par->size - 1;
          }
          // console ensures the cursor won't move beyond
          break;
        case MV_D:
          if (edit_index >= current_par->size - 1) {
            if (current_par->next != 0) {
              current_par = current_par->next;
              edit_index = 0;
            }
          }
          else if (edit_index + 80 < current_par->size) {
            edit_index += 80;
          }
          else {
            edit_index = current_par->size - 1;
          }
          break;
        case MV_U:
          if (edit_index - 80 >= 0) {
            edit_index -= 80;
          }
          else if (current_par-> before != 0) {
            current_par = current_par->before;
            edit_index = current_par->size - 1;
          }
          break;
        case 'e':
          mode = EDT_MODE;
          update_output();
          return;
        case 'q':
          m_clear();
          exit();
        case 's':
          save();
        default:
          break;
      }
    }
    // putc(move_u); // conpensate \n
    update_output();
  }
}

void
edit_mode()
{
  memset(buf, 0, 128);
  struct par* new_par;
  while (1) {
    buf_cnt = read(0, buf, 128);
    if (buf_cnt < 0) {
      break;
    }
    --buf_cnt; // remove ending enter
    switch (buf[0] & 0xff) {
      case KEY_LEFT:
      case KEY_DOWN:
      case '@':
        wash_buf();
        if (LR_cnt > 0) {
          if (LR_cnt > edit_index) {
            LR_cnt = edit_index;
          }
          memmove(trans, current_par->content+edit_index-LR_cnt, current_par->size - edit_index + LR_cnt);
          memmove(current_par->content+edit_index-LR_cnt, str, strLen);
          memmove(current_par->content+edit_index-LR_cnt+strLen, trans, current_par->size - edit_index + LR_cnt);
          edit_index += strLen;
          current_par->size += strLen;
        }
        if (UD_cnt < 0) {
          new_par = add_back(current_par);
          memmove(new_par->content, current_par->content+edit_index, current_par->size-edit_index);
          new_par->size = current_par->size - edit_index;
          current_par->size = edit_index;
          current_par = new_par;
          edit_index = 0;
        }
        if(delnum > 0)
        {
          memmove(trans, current_par->content+edit_index-LR_cnt, current_par->size - edit_index+LR_cnt);
          memmove(current_par->content+edit_index-LR_cnt-delnum, trans, current_par->size-edit_index+LR_cnt);
		  edit_index -= delnum;
		  current_par->size -= delnum;	
        }
        break;
      case KEY_ESC:
        mode = VIS_MODE;
        update_output();
        return;
      default:
        //insert data putc
        memmove(trans, current_par->content+edit_index, current_par->size-edit_index);
        printf(1,current_par->content);
        memmove(current_par->content+edit_index+buf_cnt, trans, current_par->size-edit_index);
        // memmove(current_par->content+edit_index+buf_cnt, current_par->content+edit_index, current_par->size-edit_index);
        memmove(current_par->content+edit_index, buf, buf_cnt);
        edit_index += buf_cnt;
        current_par->size += buf_cnt;
        break;
    }

    update_output();
  }
  return;
}

int
main(int argc, char *argv[])
{

  new_file = 0;
  first_par.size = 0;
  first_par.before = 0;
  first_par.next = 0;
  memset(first_par.content, 0, PAR_SIZE);
  screen_par = &first_par;
  current_par = &first_par;
  head_par = &first_par;
  edit_index = 0;
  mode = VIS_MODE;

  if(argc != 2){
    printf(1, "Argument error.\n");
    exit();
  }

  int fd;

  // while((fd = open("console", O_RDWR)) >= 0){
  //   if(fd >= 3){
  //     close(fd);
  //     break;
  //   }
  // }

  file_name = argv[1];
  fd = open_file(file_name);
  load_file(fd);
  close(fd);
  update_output();
  while (1) {
    switch(mode) {
      case VIS_MODE:
        visual_mode();
        break;
      case EDT_MODE:
        edit_mode();
        break;
      default:
        break;
    }
  }

  exit();
}


