#include "mpc/mpc.h"

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
/* or include these editline header */
#else 

	#include <editline/readline.h>
	/*if not not OS X then include header file below*/
	#ifndef __APPLE__
		#include <editline/history.h>
	#endif

#endif

int main(int argc, char** argv) {

	/* creating some parsers */
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expression = mpc_new("expression");
	mpc_parser_t* Zuzeelik = mpc_new("zuzeelik");

	/* defining them with following language */
	mpca_lang(MPCA_LANG_DEFAULT,
		" 															   \
			number 	   : /-?[0-9]+/ ; 		  						   \
			operator   : '+' | '-' | '*' | '/' ;					   \
			expression : <number> | '(' <operator> <expression>+ ')' ; \
			zuzeelik   : /^/ <operator> <expression>+ /$/ ; 		   \
		",
	Number, Operator, Expression, Zuzeelik);


	puts("zuzeelik [ version: v0.0.0-0.1.0 ] \n");
	puts("Press Ctrl+C to Exit \n");
	
	/* Starting REPL */
	while(1){
		/* output from the prompt*/
		char* input = readline("zuzeelik> ");
		
		/*Add input to history */
		add_history(input);
		
		/* An attepmt to parse the input */
		mpc_result_t result;
		if(mpc_parse("<stdin>", input, Zuzeelik, &result)) {

			/* On success print the AST */
			mpc_ast_print(result.output);
			mpc_ast_delete(result.output);
		}else {

			/*Or else print the error */
			mpc_err_print(result.error);
			mpc_err_delete(result.error);
		}

		/*free retrieved input */
		free(input);
	}

	/* undefining and deleting parsers */
	mpc_cleanup(4, Number, Operator, Expression, Zuzeelik);
	return 0;
}
