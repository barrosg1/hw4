prog: main.c
	gcc -Wall -Werror main.c -o prog
	
clean:
	rm -f prog