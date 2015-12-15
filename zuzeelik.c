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

/*using operator string to see which operator to perform */
long int evaluate_o(long int x, char* o, long int y){
	if(strcmp(o, "+") == 0 || strcmp(o, "add") == 0 ) { return x + y; }
	if(strcmp(o, "-") == 0 || strcmp(o, "sub") == 0 ) { return x - y; }
	if(strcmp(o, "/") == 0 || strcmp(o, "div") == 0 ) { return x / y; }
	if(strcmp(o, "*") == 0 || strcmp(o, "mul") == 0 ) { return x * y; }
	if(strcmp(o, "%") == 0) {return x % y; }
	return 0;
}

long int evaluate(mpc_ast_t* node){
	
	/*If tagged as number returning it directly */
	if(strstr(node->tag, "number")){
		return atoi(node->contents);
	}

	/*The operator is always second child */
	char* o = node->children[1]->contents;

	/* storing the third child in x */
	long int x = evaluate(node->children[2]);

	/* Iterating the remaining children and combining (from fourth child) */
	int i = 3;
	while(strstr(node->children[i]->tag, "expr")){
		x = evaluate_o(x, o, evaluate(node->children[i]));
		i++;
	}
	return x;
}

int main(int argc, char** argv) {

	/* creating some parsers */
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expression = mpc_new("expression");
	mpc_parser_t* Zuzeelik = mpc_new("zuzeelik");

	/* defining them with following language */
	mpca_lang(MPCA_LANG_DEFAULT,
		" 															   							\
			number 	   : /-?[0-9]+(\\.[0-9]*)?/	; 		  						   				\
			operator   : '+' | '-' | '*' | '/' | '%' | \"add\" | \"sub\" | \"mul\" | \"div\" ;	\
			expression : <number> | '(' <operator> <expression>+ ')' ; 							\
			zuzeelik   : /^/ <operator> <expression>+ /$/ ; 		   							\
		",
	Number, Operator, Expression, Zuzeelik);


	puts("zuzeelik [ version: v0.0.0-0.2.0 ] \n");
	puts("Press Ctrl+C to Exit \n");
	
	/* Starting REPL */
	while(1){
		/* output from the prompt*/
		char* input = readline("zuzeelik> ");
		
		/*Add input to history */
		add_history(input);
		
		/* An attempt to parse the input */
		mpc_result_t result;
		if(mpc_parse("<stdin>", input, Zuzeelik, &result)) {

			/* On success print the Abstract Syntax Tree */
			printf("\nAbstract Syntax Tree:\n\n");
			mpc_ast_print(result.output);
			printf("\n\n");
			
			/*After this print the evaluated answer */
			printf("Evaluated output: ");
			long int answer = evaluate(result.output);
			printf("%li\n", answer);
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
