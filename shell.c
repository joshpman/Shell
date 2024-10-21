
#include "shell.h"
#define shellHeader "[My-Shell"
#define bufferSize 8192
int main(int argc, char **argv){
	printf("Hello World\n");
	int nfds = 1;
	fd_set fileSet;
	char* readHere;
	readHere = malloc(sizeof(char) * bufferSize);
	char* currentDir = "] ";
	char* userDir = getHomeDirectory();
	while(1){
		memset(readHere, 0, sizeof(*readHere));
		FD_ZERO(&fileSet);
		FD_SET(0, &fileSet);
		int selectVal = select(nfds, &fileSet, NULL, NULL, NULL);
		if(FD_ISSET(0, &fileSet)){
			read(0, readHere, bufferSize);
			char curHeader[128];
			strcpy(curHeader, shellHeader);
			strcat(curHeader, currentDir);
			write(1, curHeader, strlen(curHeader));
		};
	}
}
