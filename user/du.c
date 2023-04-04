#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"

int k = 0;
int t = 0;
int r = 0;
int threshold = 0;
int totalsize = 0; 
char *d = ".";

char *fmtname (char *path){
    static char buf[DIRSIZ + 1];
    char *p;
    
    for (p = path + strlen(path); p >= path && *p != '/'; p--);
    p++;
    
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p , strlen(p));
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}


int ceil(int a, int b){
    if (a % b != 0){
        return (a / b) + 1;
    }else {
        return (a / b);
    }
}

void du(char *path) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;   

  if ((fd = open(path, 0)) < 0) {
    printf(1, "check usage.\n");
    return;
  }

  if (fstat(fd, &st) < 0) {
    printf(2, "du: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type) {
  case T_FILE:
    totalsize += st.size;

    printf(1, "%d %s\n", st.size, fmtname(path));
    printf(1, "%d %s\n", totalsize, fmtname(path));
    
    break;

  case T_DIR:
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
      printf(1, "du: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if (stat(buf, &st) < 0) {
        printf(1, "du: cannot stat %s\n", buf);
        continue;
      }

      if (k == 1) { // -k is active, print blocks
        if (st.type == 2) { // current is file
            if(t == 1){ // threshold
                if (st.size > threshold) {
                    totalsize += ceil(st.size, BSIZE);
                    printf(1, "%d %s/%s\n", ceil(st.size, BSIZE), path, fmtname(buf));
                
                }
            } else if (t == 0) { // only print size
                totalsize += ceil(st.size, BSIZE);
                printf(1, "%d %s/%s\n", ceil(st.size, BSIZE), path, fmtname(buf));
            }
        } else if (st.type == 1 && r == 1) { // r == 1
            if(t == 1){ // threshold
                if (st.size > threshold) {
                    totalsize += ceil(st.size, BSIZE);
                    printf(1, "%d %s/%s\n", ceil(st.size, BSIZE), path, fmtname(buf));
                }
            } else if (t == 0) { // only print size
                totalsize += ceil(st.size, BSIZE);
                printf(1, "%d %s/%s\n", ceil(st.size, BSIZE), path, fmtname(buf));
            }
        }
      } else { // no k, no blocks
        if (st.type == 2) {
            if(t == 1){ // threshold
                if (st.size > threshold) {
                    totalsize += st.size;
                    printf(1, "%d %s/%s\n", st.size, path, fmtname(buf));
                    
                }
            } else if (t == 0) { // no threshold
                totalsize += st.size;
                printf(1, "%d %s/%s\n", st.size, path, fmtname(buf));
                
            }
        } else if (st.type == 1 && r == 1) { // r == 1
            if(t == 1){ // print blocks
                if (st.size > threshold) {
                    totalsize += st.size;
                    printf(1, "%d %s/%s\n", st.size, path, fmtname(buf));
                }
            } else if (t == 0) { // only print size
                totalsize += st.size;
                printf(1, "%d %s/%s\n", st.size, path, fmtname(buf));
            }
        }

      }
    }
    // if (k == 0) {
    //     printf(1, "%d %s\n", totalsize, path);
    // }else if (k == 1) {
    //     printf(1, "%d %s\n", totalsize, path);
    // } 
    printf(1, "%d %s\n", totalsize, path);
    break;
  }
  close(fd);
}

char isNumber(char *string){
    int i;
    i = strlen(string);
    while(i--){
        if(string[i] >= '0' && string[i] <= '9')
            continue;

        return 0;
    }
    return 1; // if is integer
}

int main(int argc, char *argv[]){
    int i;
    if (argc < 2){ // du
        du(".");
        exit();
    }

    for (i = 1; i < argc; i++){
        if (strcmp(argv[i], "-k") == 0) { // if -k
            if (k == 0) {
                k = 1;
            } else {
                printf(1, "check usage.\n");
                exit();
            }
            
        } else if (strcmp(argv[i], "-t") == 0) { // if -t. check if threshold is valid
            if (t == 0) { // if touch -t
                t = 1;
            } else {
                printf(1, "check usage.\n");
                exit();
            }
            if (argv[i+1] == 0){ // -t empty
                printf(1, "check usage.\n");
                exit();
            }
            if (isNumber(argv[i+1]) == 0) { 
                printf(1, "check usage.\n");
                exit();
            }

        } else if (t == 1 && isNumber(argv[i]) == 1) {
            threshold = atoi(argv[i]);
        } else if (strcmp(argv[i], "-r") == 0) { // if -r r == 1

            if (r == 0) {
                r = 1;
            } else {
                printf(1, "check usage.\n");
                exit();
            }

            if (argv[i+1] != 0) {
                char sl [strlen(argv[i+1])];
                strcpy(sl, argv[i+1]);
                if (strcmp(&sl[strlen(sl)-1], "/") == 0) {
                    sl[strlen(sl)-1] = '\0';
                    du(sl);
                    exit();
                }

                du(argv[i+1]);
                exit();
            }

        } else {
            char slash[strlen(argv[i])];
            strcpy(slash, argv[i]);
            if (strcmp(argv[i], "/") == 0) {
                du(".");
                exit();
            }            
            if (strcmp(&slash[strlen(slash)-1], "/") == 0){ // the last character is /
                slash[strlen(slash)-1] = '\0'; // remove the last /
                du(slash);
                exit(); 
            }
            du(argv[i]);
            exit();
        }
    }
    du(d);
    exit();
}
