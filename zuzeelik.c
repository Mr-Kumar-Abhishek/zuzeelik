#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

int main(int argc, char** argv) {
	puts("zuzeelik [version: v0.0.0-0.0.2]");
	puts("Press Ctrl+C to Exit \n");
	
	/* Starting REPL */
	
	while(1){
		/* output from the prompt*/
		char* input = readline("zuzeelik> ");
		
		/*Add input to history */
		add_history(input);
		
		/* Echo the input back to the user */
		printf("Got Input: %s \n", input);

		free(input);
	}

	return 0;
}
		
