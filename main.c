#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifndef FILE_NAME_MAX
#define FILE_NAME_MAX 1024
#endif

void processDirectory(FILE*,char*);
void readFileInfo(FILE* reader, int, char*);
void DisplayList();
void sortList(int, char*);

enum { FALSE, TRUE }; /* Booleans */

struct FileInfo
{
	char f_name[FILE_NAME_MAX];
	int f_size;
};

/* self-referential structure */
struct Node
{
    int fileSize;
    char* fileName;
    struct Node *next;
};

struct Node *head = NULL;

int main(int argc, char * argv[])
{

    char* dirName;
    int pid, ret, pfds[2];

    if(argc != 2)
    {
        puts("Invalid command line argument.");
        return -1;
    }
    else
    {
    	dirName = argv[1];

    	ret = pipe(pfds);  // creating pipe
		if(ret == -1)
		{
			perror("Pipe Error");
			exit(1);
		}

		FILE *reader = fdopen(pfds[0], "r");
		FILE *writer = fdopen(pfds[1], "w");

    	pid = fork(); // spawning a process

    	if(pid < 0)
    	{
    		perror("Fork Error");
    		exit(0);
    	}
    	else if(pid == 0)
		{ /* CHILD Process */

			close(pfds[0]); /* close read pipe */
			processDirectory(writer, dirName); /* scan files and write file info to pipe */

		}
		else
		{ /* PARENT Process */

			struct FileInfo f_info;
			int size = f_info.f_size;
			char* fileName = f_info.f_name;

			close(pfds[1]); /* close write pipe */
			readFileInfo(reader, size, fileName); /* read file information and sort */
			DisplayList();  /* output all the data */

			wait(NULL);
		}
    }

    return 0;

}

void processDirectory(FILE* writer, char *dirName)
{

	struct dirent dirEntry;
	struct stat st;
	char filePath[FILE_NAME_MAX];
	int fd, charsRead;
	off_t mode;

	// remove the last '/' from the directory name
	// if user type it on the command line
	if(dirName[strlen(dirName)-1] == '/') dirName[strlen(dirName)-1] = '\0';

	fd=open(dirName, O_RDONLY); /* Open for reading */
	if ( fd == -1 )
	{
		perror("File Error");
		exit(0);
	}

	while( TRUE )
	{ /* Read all directory entries */

		charsRead = syscall(SYS_getdents64, fd, &dirEntry, sizeof(struct dirent));
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

			lstat(filePath, &st);  // getting info from each file
			mode = st.st_mode;
			size = st.st_size;


			if(S_ISDIR(mode))
			{
				processDirectory(writer, filePath);
			}

			else if (S_ISREG(mode))
			{

				fprintf(writer, "%d, %s\n", size, filePath); // write file info to writer
				fflush(writer);
			}

		}

		lseek( fd, dirEntry.d_off, SEEK_SET ); /*Find next entry */
	}

	close(fd);
}

void readFileInfo(FILE* reader, int fsize, char* fileName)
{
	while(fscanf(reader, "%d, %s\n", &fsize, fileName) == 2)
	{
		char* fname = malloc(strlen(fileName)+1);
		strcpy(fname, fileName);

		// adding the data read to the linked list and sorting
		sortList(fsize, fname);
	}

}

void DisplayList()
{
	struct Node *node = head;
    while (node != NULL)
    {
        printf("%d\t%s\n", node->fileSize, node->fileName);
        node = node->next;
    }
}

void sortList(int fsize, char* fname)
{ // insertion sort

	struct Node *temp=head;
	struct Node *prev=NULL;
	struct Node *node;

	node= (struct Node*) malloc(sizeof(struct Node));
	node->fileSize = fsize;
	node->fileName = fname;
	node->next = NULL;

	if(temp == NULL)
	{ // if linked list is empty
		node->next = NULL;
		head = node;
		return;
	}

	if(fsize < temp->fileSize )
	{
		node->next=head;
		head=node;
		return;
	}
	else
	{
		while(temp != NULL)
		{
			if(fsize > temp->fileSize || fsize == temp->fileSize)
			{
				// traversing to position to insert the node
				prev = temp;
				temp = temp->next;
				continue;
			}
			else
			{
				// inserting node
				prev->next = node;
				node->next = temp;
				return;
			}
		}
		prev->next=node;  //Insert node at the last position
	}

}
