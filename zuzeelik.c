#include <stdio.h>

static char input[2048]; //declaring buffer for user input, size 2048

int main(int argc, char** argv) {
	puts("zuzeelik Version 0.0.0-0.0.1");
	puts("Press Ctrl+c to Exit \n");
	
	/* Starting REPL */
	
	while(1){
		/* output from the prompt*/
		fputs("zuzeelik> ", stdout);
		
		/* read a line of the user of max size 2048 */
		fgets(input, 2048, stdin);
		
		/* Echo the input back to the user */
		printf("Got Input: %s", input);
	}

	return 0;
}
		
