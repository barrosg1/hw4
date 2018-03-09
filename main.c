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

// global variable
int pfds[2];

void processDirectory(char*, FILE*);

enum { FALSE, TRUE }; /* Booleans */

typedef struct FileInfo
{
	char f_name[FILE_NAME_MAX];
	int f_size;

} FileInfo;


int main(int argc, char * argv[]) {

    char* dirName;
    int pid, ret;
    FileInfo f_info;

    if(argc != 2)
    {
        puts("Invalid command line argument.");
        return -1;
    }
    else
    {

    	dirName = argv[1];

    	ret = pipe(pfds);
		if(ret == -1)
		{
			perror("Pipe Error");
			exit(1);
		}

		FILE *reader = fdopen(pfds[0], "r");
		FILE *writer = fdopen(pfds[1], "w");

    	pid = fork();

		if(pid == 0)
		{ /* CHILD Process */

			processDirectory(dirName, writer);

		}
		else
		{ /* PARENT Process */

			int size = f_info.f_size;

			close(pfds[1]);
			fscanf(reader, "%d, %s\n", &size, f_info.f_name);

			wait(NULL);
		}
    }

    return 0;

}

void processDirectory(char *dirName, FILE* writer)
{

	struct dirent dirEntry;
	struct stat st;
	char filePath[FILE_NAME_MAX];
	int fd, charsRead;
	off_t mode;

	fd=open(dirName, O_RDONLY); /* Open for reading */
	if ( fd == -1 ) {
		perror("monitor");
		exit(0);
	}

	while( TRUE ) { /* Read all directory entries */

		charsRead = syscall(SYS_getdents64, fd, &dirEntry, sizeof(struct dirent) );
		int size;

		if ( charsRead == -1 )
		{
			perror("charsRead");
			exit(0);
		}

		if ( charsRead == 0 ) break;  /* EOF */

		if ( strcmp(dirEntry.d_name, ".") != 0 &&
			strcmp( dirEntry.d_name, "..") != 0 ) {/*Skip . and  ..*/

			sprintf(filePath, "%s/%s", dirName, dirEntry.d_name);

			stat(filePath, &st);
			mode = st.st_mode;
			size = st.st_size;

			if(S_ISDIR(mode))
			{
				processDirectory(filePath, writer);
			}
			else if (S_ISREG(mode))
			{
				fprintf(writer, "%d, %s\n", size, filePath);
				close(pfds[0]);
				fflush(writer);

				//printf("FILE: %s\n", filePath);
			}

		}

		lseek( fd, dirEntry.d_off, SEEK_SET ); /*Find next entry */
	}

	close(fd);
}



























