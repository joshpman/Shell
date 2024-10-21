#include "shell.h"

char* getHomeDirectory(){
    uid_t callingUserID = getuid();
    struct passwd *userPasswdFile = getpwuid(callingUserID);
    return userPasswdFile->pw_dir;
}