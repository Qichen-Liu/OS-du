/* 
INSERT THIS FILE INTO ./user/casegen.c
ADD _casegen to UPROGS in Makefile, 
     just as you did for the rest of your HW1 programs
*/

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"

#define SETUP_COMMAND "setup"
#define TEARDOWN_COMMAND "teardown"

#define MAX_FILE_COUNT 32
#define MAX_FILE_SIZE BSIZE * 3

#define DEBUG 0

#define debug(_var) if(DEBUG==1) { _var };

static char* PLACED_DIRS[] = {
  "/",
  "/a",
  "/a/b",
  "/a/b/c"
};

uint hex_to_uint(char*);
uint lfsr(uint);
void make_dirs();
void make_files(uint);
void rm_files(uint);
void rm_dirs();

int main(int argc, char* argv[]) {
  if (argc < 3 || argc > 4) {
    printf(1, "check usage.  casegen [setup | teardown] [hex-code]\n");
    exit();
  }
  uint key = hex_to_uint(argv[2]);

  debug(printf(1, "Key: %x\n\n", key);)

  if (strcmp(argv[1], SETUP_COMMAND) == 0) {
    make_dirs();
    make_files(key);
  } else if (strcmp(argv[1], TEARDOWN_COMMAND) == 0) {
    rm_files(key);
    rm_dirs();
  }
  printf(1, "OK.\n");
  exit();
}

void make_dirs() {
  int result = 0;
  result += mkdir(PLACED_DIRS[1]);
  result += mkdir(PLACED_DIRS[2]);
  result += mkdir(PLACED_DIRS[3]);
  if (result < 0) {
    printf(1, "could not make dirs\n");
    exit();
  }
}

void rm_dirs() {
  int result = 0;
  result += unlink(PLACED_DIRS[3]);
  result += unlink(PLACED_DIRS[2]);
  result += unlink(PLACED_DIRS[1]);
  if (result < 0) {
    debug(printf(1, "could not remove dirs\n");)
    exit();
  }
}

typedef struct file_s {
    char path[15];
    int size;
} file_s;


void filelist_from_key(uint, file_s[MAX_FILE_COUNT]);
void fill_file(char*, int);

void make_files(uint key) {
  file_s filelist[MAX_FILE_COUNT];
  filelist_from_key(key, filelist);

  for (int i = 0; i < MAX_FILE_COUNT; i++) {
    debug(printf(1, "file: %d %s\n", filelist[i].size, filelist[i].path);)
    fill_file(filelist[i].path, filelist[i].size);
  }
}

void rm_files(uint key) {
  file_s filelist[MAX_FILE_COUNT];
  filelist_from_key(key, filelist);

  for (int i = 0; i < MAX_FILE_COUNT; i++) {
    int status = unlink(filelist[i].path);
    if (status < 0) {
      debug(printf(1, "could not delete %s\n", filelist[i].path);)
      exit();
    }
  }
}

uint hex_to_uint(char* arg) {
  uint value = 0;
  for (char* ptr = arg; *ptr != '\0'; ptr++) {
    if (*ptr >= '0' && *ptr <= '9') {
      value *= 16;
      value += *ptr - '0';
      continue;
    } else if (*ptr >= 'a' && *ptr <= 'f') {
      value *= 16;
      value += *ptr - 'a' + 10;
      continue;
    } else {
      printf(1, "bad hex: %s\n", arg);
      exit();
    }
  }
  return value;
}

uint lfsr(uint first) {
  uint result = first;
  if (result == 0) {
    result = 0xFFFFFFFF - 5;
  }
  while (1) {
    result ^= result >> 7;
    result ^= result << 9;
    result ^= result >> 13;
    if (result != first) {
      // debug(printf(1, "lfsr next: %x\n", result););
      return result;
    }
  }
}


void filelist_from_key(uint key, file_s filelist[MAX_FILE_COUNT]) {
  for (int i = 0; i < MAX_FILE_COUNT; i++) {
    char filename[7];
    uint fp = lfsr(key);
    float prob = (float)fp / 0xFFFFFFFF;
    int dir_id = (int)(prob * 4);
    int dir_len = 0;
    for (int j = 0; j < 8; j++) {
      filelist[i].path[j] = PLACED_DIRS[dir_id][j];
      if (PLACED_DIRS[dir_id][j] == '\0') {
        break;
      }
      dir_len++;
    }

    char* fptr = 0;
    if (strcmp(PLACED_DIRS[dir_id], "/") == 0) {
      strcpy(filename, "file");
      fptr = filename + 4;
    } else {
      strcpy(filename, "/file");
      fptr = filename + 5;
    }

    if (i < 10) {
      *fptr++ = '0';
      *fptr++ = '0' + i;
      *fptr = '\0';
    } else {
      *fptr++ = (i / 10) + '0';
      *fptr++ = (i - (i/10) * 10) + '0';
      *fptr = '\0';
    }

    for (int j = dir_len; j < 7 + 8; j++) {
      filelist[i].path[j] = filename[j - dir_len];
      if (filename[j - dir_len] == '\0') {
        break;
      }
    }
    // debug(printf(1, "filename: %s\n", filelist[i].path););

    fp = lfsr(fp);
    filelist[i].size = (int)(((float)fp / 0xFFFFFFFF)*MAX_FILE_SIZE + 30);
    // debug(printf(1, "    size: %d\n", filelist[i].size););
    key = fp;
  }
}

char blank[] = "0000000000000000000000000000000";
void fill_file(char* path, int bytes) {
  int rem = bytes;
  int fd = open(path, O_CREATE | O_WRONLY);
  if (fd < 0) {
    printf(1, "could not create file: %s\n", path);
    exit();
  }
  int status = 0;
  while(rem > 32) {
    status = write(fd, blank, 32);
    if (status <= 0) {
      printf(1, "error writing to %s\n", path);
      close(fd);
      exit();
    }
    rem -= 32;
  }
  if (rem != 0) {
    status = write(fd, blank, rem);
    if (status <= 0) {
      printf(1, "error writing to %s\n", path);
      close(fd);
      exit();
    }
  }
  
  close(fd);
}