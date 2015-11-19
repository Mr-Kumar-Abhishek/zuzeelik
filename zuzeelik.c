#include <stdio.h>
#include <stdlib.h>

/* if compiling in windows, compiling with this functions */

#ifdef _WIN32
#include <string.h>

	static char buffer[2048];
	/* fake readline functions */
	char* readline(char* prompt) {
		fputs(prompt, stdout);
		fputs(buffer, 2048, stdin);
		fgets(buffer, 2048, stdin);
		char* copy = malloc(strlen(buffer)+1);
		strcpy(copy, buffer);
		copy[strlen(copy) - 1] = '\0';
		return copy;
	}
	/* fake add_history function */
	void add_history(char* not_used) {}
/* or include thesse editline header */
#else 

	#include <editline/readline.h>
	#include <editline/history.h>

#endif

int main(int argc, char** argv) {
	puts("zuzeelik [version: v0.0.0-0.0.3]");
	puts("Press Ctrl+C to Exit \n");
	
	/* Starting REPL */
	
	while(1){
		/* output from the prompt*/
		char* input = readline("zuzeelik> ");
		
		/*Add input to history */
		add_history(input);
		
		/* Echo the input back to the user */
		printf("Got Input: %s \n", input);

		/*free retrieved input */
		free(input);
	}

	return 0;
}
