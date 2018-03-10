#include <stdio.h>            /* For printf, fprintf */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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

struct List
{
    struct Node *head;
    struct Node *tail;
};

struct List new_list()
{
    /* construct an empty list */
    struct List list;
    list.head = NULL;
    list.tail = NULL;
    return list;
};

void processDirectory(char*, FILE*);
int list_empty(struct List *list);
void list_push(struct List *list, int, char*);
void list_append(struct List *list, int, char*);
void DisplayList(struct List * list);
void sortList(struct List * list);

int main(int argc, char * argv[])
{

    char* dirName;
    int pid, ret, pfds[2];
    struct FileInfo f_info;

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

    	if(pid < 0)
    	{
    		perror("Fork Error");
    		exit(0);
    	}
    	else if(pid == 0)
		{ /* CHILD Process */

			close(pfds[0]); /* close read pipe */
			processDirectory(dirName, writer); /* scan files and write file info to pipe */

		}
		else
		{ /* PARENT Process */

			struct List list = new_list();
			int size = f_info.f_size;
			char* fileName = f_info.f_name;

			close(pfds[1]); /* close write pipe */

			/* exit if there aren't enough data in the read file */
			if(fscanf(reader, "%d, %s\n", &size, fileName) != 2) exit(0);


			/* Reading data from the pipe */
			while(fscanf(reader, "%d, %s\n", &size, fileName) == 2)
			{

				char* fname = malloc(strlen(fileName)+1);
				strcpy(fname, fileName);

				/* adding the data read to the linked list */
				list_append(&list, size, fname);

			}

			sortList(&list);

			/* output all the data */
			DisplayList(&list);

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
				fflush(writer);
			}

		}

		lseek( fd, dirEntry.d_off, SEEK_SET ); /*Find next entry */
	}

	close(fd);
}


int list_empty(struct List *list)
{
    /* return true if the list contains no items */
    return list->head == NULL;
}

void list_push(struct List *list, int fsize, char* fname)
{
    /*  insert the item at the beginning of the list */
    struct Node *node = malloc(sizeof(struct Node));
    node->fileSize = fsize;
    node->fileName = fname;
    node->next = list->head;

    if(list_empty(list))
    {
    	list->tail = node;
    }

    list->head = node;
}

void list_append(struct List *list, int fsize, char* fname)
{
    /* append the item to the end of the list */
    if (list_empty(list))
    {
        list_push(list, fsize, fname);
    }
    else
    {
        struct Node *node = malloc(sizeof(struct Node));

        node->fileSize = fsize;
        node->fileName = fname;

        node->next = NULL;
        list->tail->next = node;
        list->tail = node;
    }
}

void DisplayList(struct List * list)
{
    struct Node * nod = list->head;
    while (nod != NULL)
    {
        printf("%d\t%s\n", nod->fileSize, nod->fileName);
        nod = nod->next;
    }
}

void sortList(struct List * list)
{
    struct Node * nod = list->head;
    if (!nod->next)
    {
    	list->head = nod;
    	list->tail = nod;
    	return;
    }
    struct Node * nod2 = list->head;
    int temp;
    char* tempChar;
    while (nod != NULL)
    {
        nod2 = list->head;
        while (nod2 != NULL)
        {
            if (nod->fileSize < nod2->fileSize)
            {
                //swap fileSize
                temp = nod->fileSize;
                nod->fileSize = nod2->fileSize;
                nod2->fileSize = temp;

                // swap fileName
                tempChar = nod->fileName;
				nod->fileName = nod2->fileName;
				nod2->fileName = tempChar;

            }
            nod2 = nod2->next;
        }
        nod = nod->next;
    }

}

