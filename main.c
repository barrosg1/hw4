#include <stdio.h>            /* For printf, fprintf */
#include <stdlib.h>
#include <string.h>        /* For strcmp */
#include <ctype.h>            /* For isdigit */
#include <fcntl.h>
#include <sys/syscall.h>
#include <dirent.h>        /* For getdents */
#include <unistd.h>
#include <sys/stat.h>        /* For IS macros */
#include <sys/types.h>        /* For modet */
#include <sys/wait.h>

#ifndef FILE_NAME_MAX
#define FILE_NAME_MAX 1024
#endif

void processDirectory(char*);

enum { FALSE, TRUE }; /* Booleans */

typedef struct FileInfo
{
	char f_name[FILE_NAME_MAX];
	int f_size;


} FileInfo;


int main(int argc, char * argv[]) {

    char* dirName;

    if(argc != 2)
    {
        puts("Invalid command line argument.");
        return -1;
    }
    else {

    	dirName = argv[1];
    	processDirectory(dirName);
    }

    return 0;

}

void processDirectory(char *dirName)
{

	struct dirent dirEntry;
	struct stat st;
	char filePath[FILE_NAME_MAX];
	int fd, charsRead;
	int pid, ret;
	int pfds[2];
	off_t mode;

	ret = pipe(pfds);

	if(ret == -1)
	{
		perror("Pipe Error");
		exit(1);
	}

	pid = fork();


	if(pid == 0)
	{

		/* CHILD Process */


		fd=open(dirName, O_RDONLY); /* Open for reading */

		if ( fd == -1 ) {

			perror("monitor:");
			exit(0);
		}

		//write(pfds[1], "Gabriel", 8);

		while( TRUE ) { /* Read all directory entries */

			charsRead = syscall(SYS_getdents64, fd, &dirEntry, sizeof(struct dirent) );

			if ( charsRead == -1 ) {perror("monitor:"); exit(0);}

			if ( charsRead == 0 ) break;  /* EOF */

			if ( strcmp(dirEntry.d_name, ".") != 0 &&

				strcmp( dirEntry.d_name, "..") != 0 ) {/*Skip . and  ..*/


				sprintf(filePath, "%s/%s", dirName, dirEntry.d_name);

				stat(filePath, &st);

				mode = st.st_mode;

				if(S_ISDIR(mode))
				{
					processDirectory(filePath);
				}
				else if (S_ISREG(mode)) {


					write(pfds[1], &filePath, strlen(filePath) + 1);

					//printf("%s\n",filePath);


				}

			}

			lseek( fd, dirEntry.d_off, SEEK_SET ); /*Find next entry */

		}

		close(fd);

		//close(pfds[0]);

	}
	else
	{

		/* PARENT Process */

		FileInfo f_info;

		close(pfds[1]);

		read(pfds[0], f_info.f_name, sizeof(f_info.f_name));
		printf("PARENT read: %s\n", f_info.f_name);

		wait(NULL);
	}


}
