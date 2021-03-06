#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/param.h>
#include <libgen.h>
#include "hash.h"

int create_tree(const char *currentPath, const char *pathToMake);

char *contains(const char *direc, const char* name, const char *currentPath);

int regFileCreator(const char * path2, const char * name, FILE *fp, FILE *fp2, char pathname2[]);

int copy_ftree(const char *src, const char *dest) {
    const char * path1;
    const char * path2;
    char *pathToMake = malloc(MAXPATHLEN + 1);
    char *currentPath = malloc(MAXPATHLEN + 1);
    char *dirName = malloc(sizeof(char*));
    if (dest[0] != '/') {
	getcwd(pathToMake, MAXPATHLEN);
        strcat(pathToMake, "/");
        strcat(pathToMake, dest);
    }
    else {
        strcat(pathToMake, dest);
    }
    struct stat *dest_info = malloc(sizeof(struct stat));
    lstat(pathToMake, dest_info);
    if (S_ISREG(dest_info->st_mode)) {
	perror("Destination is not a directory");
	return -1;
    }
    if (src[0] != '/') {
	getcwd(currentPath, MAXPATHLEN);
	strcat(currentPath, "/");
        strcat(currentPath, src);
	strcat(pathToMake, "/");
        strcat(pathToMake, src);
    }
    else {
	strcat(currentPath, src);
	dirName = basename(currentPath);
	strcat(pathToMake, "/");
        strcat(pathToMake, dirName);
    }
    path1 = pathToMake;
    path2 = currentPath;
    int numProcesses = 0;
    char *includes = contains(dest, src, currentPath);
    struct stat *src_info = malloc(sizeof(struct stat));
    lstat(path2, src_info);
    int error = 0;
    if (S_ISDIR(src_info->st_mode)) {
	if (includes[1] != 'D'){
            error = mkdir(pathToMake, ((src_info->st_mode) & (0777)));
	    if (error == -1) {
		perror("Cannot access directory");
		return -1;
	    }
        }
	numProcesses = create_tree(currentPath, pathToMake);
    }
    else {
	FILE *fp = NULL;
	FILE *fp2 = NULL;
	char nullList[1];
	regFileCreator(path1, path2, fp, fp2, nullList);
	return 0;
    }
    return (numProcesses);
}

int create_tree(const char *currentPath, const char *pathToMake) {
    char *pathToMakeCpy = malloc(MAXPATHLEN + 1);
    char *currentPathCpy = malloc(MAXPATHLEN + 1);
    DIR *dirp = opendir(currentPath);
    int error = 0;
    error = chdir(currentPath);
    if (error == -1) {
	perror("Cannot access directory");
	return -1;
    }
    int numProcesses = 1;
    struct dirent *dp;
    struct stat *content_info = malloc(sizeof(struct stat));
    dp = readdir(dirp);
    while (dp != NULL){
	int error2 = 0;
	const char * path2;
	char pathname2[MAXPATHLEN];
	lstat(dp->d_name, content_info);
	if ((dp->d_name)[0] != '.') {
 	    if (S_ISDIR(content_info->st_mode)) {
		strcat(pathToMakeCpy, pathToMake);
    		strcat(currentPathCpy, currentPath);
	        strcat(pathToMakeCpy, "/");
    		strcat(pathToMakeCpy, dp->d_name);
    		strcat(currentPathCpy, "/");
    		strcat(currentPathCpy, dp->d_name);

		//char *includes = contains(pathToMake, dp->d_name, currentPath);
		//if (includes[1] == 'R'){
		    //perror("mismatch of filetypes");
		    //if (numProcesses > 0) {
			//numProcesses = (numProcesses * -1);
		    //}
		//}
		else {
                    if (includes[1] == '0'){
		        error2 = mkdir(pathToMakeCpy, ((content_info->st_mode) & (0777)));
		    }
		    if (error2 == -1) {
			perror("Could not create directory");
			if (numProcesses > 0) {
			    numProcesses = (numProcesses * -1);
		        }
		    }
		    else {
		        int result = fork();
		        if (result == 0) {
		            int subProcesses = 0;
		            subProcesses = create_tree(currentPathCpy, pathToMakeCpy);
		            exit(subProcesses);
		        }
      	    	        int status;
		        if(wait(&status) == -1) {
                            perror("wait");
                            exit(1);
		        }
		        else {
		            if(WIFEXITED(status)) {
                                char cvalue = WEXITSTATUS(status);
				if (numProcesses > 0) {
				    if (cvalue < 0) {
					numProcesses = (numProcesses * -1);
				    }
				    numProcesses += cvalue;
				}
				else {
				    if (cvalue < 0) {
					numProcesses += cvalue;
				    }
				    else {
					numProcesses -= cvalue;
				    }
				}
                            }
		        }
		    }
		}
	    }
	    else if (S_ISREG(content_info->st_mode)) {
	        FILE *fp = NULL;
		FILE *fp2 = NULL;
		int error3 = 0;
		int error4 = 0;
		strcat(pathname2, pathToMake);
  		strcat(pathname2, "/");
		strcat(pathname2, dp->d_name);
		path2 = pathname2;
		char *includes = contains(pathToMake, dp->d_name, currentPath);
		if (includes[1] == 'R'){
		    struct stat *content_info2 = malloc(sizeof(struct stat));
		    lstat(path2, content_info2);
		    if ((content_info->st_size) == (content_info2->st_size)) {
			fp = fopen(path2, "r");
			fp2 = fopen(dp->d_name, "r");
			if ((fp != NULL) && (fp2 != NULL)) {
			    char *h1 = hash(fp);
			    char *h2 = hash(fp2);
			    if (strcmp(h1, h2) != 0) {
			        error4 = regFileCreator(path2, dp->d_name, fp, fp2, pathname2);
				if (error4 == -1){
		 		    if (numProcesses > 0) {
			    	        numProcesses = (numProcesses * -1);
		    		    }
		   		}
			        error3 = chmod(path2, ((content_info->st_mode) & (0777)));
				if (error3 == -1){
				    perror("Could not change file permissions");
				    if (numProcesses > 0) {
			                numProcesses = (numProcesses * -1);
		    		    }
				}
				
			    }
			    else {
			        error3 = chmod(path2, ((content_info->st_mode) & (0777)));
				if (error3 == -1){
				    perror("Could not change file permissions");
				    if (numProcesses > 0) {
			                numProcesses = (numProcesses * -1);
		    		    }
				}
			    } 
			}
			else {
			    perror("Could not open regular file");
			    if (numProcesses > 0) {
			        numProcesses = (numProcesses * -1);
		            }
			}
		    }
		    else {
		        error4 = regFileCreator(path2, dp->d_name, fp, fp2, pathname2);
			if (error4 == -1){
		 	    if (numProcesses > 0) {
			        numProcesses = (numProcesses * -1);
		    	    }
		        }
			error3 = chmod(path2, ((content_info->st_mode) & (0777)));
			if (error3 == -1){
			    perror("Could not change file permissions");
		   	    if (numProcesses > 0) {
			        numProcesses = (numProcesses * -1);
		    	    }
			}
		    }
		}
		else if (includes[1] == 'D') {
		    perror("mismatch of filetypes");
		    if (numProcesses > 0) {
			numProcesses = (numProcesses * -1);
		    }
		}
		else {
		    error4 = regFileCreator(path2, dp->d_name, fp, fp2, pathname2);
		    if (error4 == -1){
		 	if (numProcesses > 0) {
			    numProcesses = (numProcesses * -1);
		    	}
		    }
		}
	    }
	}
	strcpy(pathToMakeCpy, "\0");
	strcpy(currentPathCpy, "\0");
	dp = readdir(dirp);
    }
    return(numProcesses);
}

char *contains(const char *direc, const char* name, const char *currentPath) {
    char *dir = malloc(2 * sizeof(char));
    struct dirent *dp;
    DIR *dirp = opendir(direc);
    struct stat *content_info = malloc(sizeof(struct stat));
    int error = chdir(direc);
    if (error == -1) {
	dir[0] = 'F';
        dir[1] = 'F';
	perror("Cannot access directory");
	return (dir);
    }
    dp = readdir(dirp);
    while (dp != NULL) {
	lstat(dp->d_name, content_info);
        if (strcmp(dp->d_name, name) == 0) {
	    if (S_ISDIR(content_info->st_mode)) {
		dir[0] = 'Y';
		dir[1] = 'D';
	    }
	    else {
		dir[0] = 'Y';
		dir[1] = 'R';
	    }
	    chdir(currentPath);
	    return (dir);
	}
        dp = readdir(dirp);
    }
    chdir(currentPath);
    dir[0] = '0';
    dir[1] = '0';
    return (dir);
}

int regFileCreator(const char * path2, const char * name, FILE *fp, FILE *fp2, char pathname2[]) {
    fp2 = fopen(name, "r");
    if (fp2 == NULL) {
	perror("Could not open regular file");
	strcpy(pathname2, "\0");
	return -1;
    }
    fp = fopen(path2, "w");
    if (fp == NULL) {
	perror("Could not write to regular file");
	strcpy(pathname2, "\0");
	return -1;
    }
    char byte = '\0';
    while (fread(&byte, 1, 1, fp2) == 1) {
        fwrite(&byte, 1, 1, fp);	
    }
    strcpy(pathname2, "\0");
    return 1;
}