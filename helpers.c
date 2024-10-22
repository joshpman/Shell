#include "shell.h"
#define shellHeader "[My-Shell] "
char *getHomeDirectory() {
  uid_t callingUserID = getuid();
  struct passwd *userPasswdFile = getpwuid(callingUserID);
  return userPasswdFile->pw_dir;
}
void writeHeader(){
    write(1, shellHeader, strlen(shellHeader));
}
void processCommand(char* buffer, int bytesRead){

}