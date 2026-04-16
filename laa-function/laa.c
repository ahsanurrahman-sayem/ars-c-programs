#include<stdio.h>
#include<dirent.h>

int main(int argc,char * argv[]){
	struct dirent *dirobj;
	DIR *dir = opendir(".");
	if(dir != NULL){
		while((dirobj = readdir(dir))!= NULL){
			if(dirobj->d_name[0]=='.'){printf("\e[1;34m%s\e[0m\n",dirobj->d_name);
			}
		}
		closedir(dir);
	}else{
		perror("opendir");
		return 1;
	}
}
