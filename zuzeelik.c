#include "mpc/mpc.h"

/* if compiling in windows, compiling with this functions */

#ifdef _WIN32
#include <string.h>

	static char buffer[2048];

	/* fake readline functions */
	char* readline(char* prompt) {
		fputs(prompt, stdout);
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

/* creating enumeration of possible error types */
enum { ZERROR_DIV_ZERO, ZERROR_MOD_ZERO, ZERROR_BAD_OP, ZERROR_BAD_NUMBER };

/* creating enumeration of possible zval types */
enum { ZVAL_NUMBER, ZVAL_ERROR };

/* Declaring new zdata union */
typedef union zdata {
	int er;
	long number;
} zdata;

/* Declaring new zval struct */
typedef struct {
	int type;
	zdata data;
} zval;

/* creating new number type zval */
zval zval_number(long x) {
	zval val;
	val.type = ZVAL_NUMBER;
	val.data.number = x;
	return val;
}

/* creating new error type zval */
zval zval_error(int x) {
	zval val;
	val.type = ZVAL_ERROR;
	val.data.er = x;
	return val;
} 

/* printing a zval */
void zval_print(zval val) {
	switch(val.type) {

		/* in case the type is a number print it */
		/* Also, then break out of the switch */
		case ZVAL_NUMBER:
			printf("%li", val.data.number);
		break;

		case ZVAL_ERROR:
			printf("[error] \n\n");

			/* checking which type of error it is */
			if ( val.data.er == ZERROR_DIV_ZERO ) {
				printf("Error Code: %i \n\nError message: huh ?! Division by zero ?!! \n", val.data.er);
			}
			if ( val.data.er == ZERROR_MOD_ZERO ) {
				printf("Error Code: %i \n\nError message: huh ?! Modulo by zero ?!! \n", val.data.er);
			}
			if ( val.data.er == ZERROR_BAD_OP ){
				printf("Error Code: %i \n\nError message: I don't know this operator, rest is up to you.. \n", val.data.er);
			}
			if( val.data.er == ZERROR_BAD_NUMBER ) {
				printf("Error Code: %i \n\nError message: Bad Number ! Just BAD ! >:( \n ", val.data.er);
			}
		break;
	}
}

/* printing zval followed by a new line */
void zval_println(zval val){
	zval_print(val);
	putchar('\n');
}

/* count total number of nodes */
int number_of_nodes(mpc_ast_t* nodes) {
  if (nodes->children_num == 0) { return 1; }
  if (nodes->children_num >= 1) {
    int total = 1;
    for (int i = 0; i < nodes->children_num; i++) {
      total = total + number_of_nodes(nodes->children[i]);
    }
    return total;
  }
  return 0;
}

/* using operator string to see which operation to perform */
zval evaluate_o(zval x, char* o, zval y){

	/* If any value is an error then return it */
	if ( x.type == ZVAL_ERROR ) { return x; }
	if ( y.type == ZVAL_ERROR ) { return y; }

	/* otherwise doing calculations on number values */
	if(strcmp(o, "+") == 0 || strcmp(o, "add") == 0 ) { return zval_number( x.data.number + y.data.number ); }
	if(strcmp(o, "-") == 0 || strcmp(o, "sub") == 0 ) { return zval_number( x.data.number - y.data.number ); }
	if(strcmp(o, "/") == 0 || strcmp(o, "div") == 0 ) {
	 	
	 	/*if the second operand is zero then returning an error */
	 	return y.data.number == 0 ? zval_error( ZERROR_DIV_ZERO ): zval_number( x.data.number / y.data.number ); 
	}
	if(strcmp(o, "*") == 0 || strcmp(o, "mul") == 0 ) { return zval_number( x.data.number * y.data.number ); }
	if(strcmp(o, "%") == 0 || strcmp(o, "mod") == 0 ) {
		
		/* Agian, if the second operand is zero then returning an error */	
		 return y.data.number == 0 ? zval_error( ZERROR_MOD_ZERO ): zval_number( x.data.number % y.data.number ); 
	}
	if(strcmp(o, "^") == 0 || strcmp(o, "pow") == 0 ) { return zval_number((pow(x.data.number,y.data.number))); }
	if(strcmp(o, "max") == 0 ){
		if (x.data.number<=y.data.number ){ return y; } else if(x.data.number>y.data.number){ return x; } 
	 }
	if(strcmp(o, "min") == 0 ){
		if (y.data.number<=x.data.number) { return y; } else if(y.data.number>x.data.number){ return x; }
	}
	return zval_error( ZERROR_BAD_OP );
}

zval evaluate(mpc_ast_t* node){

	/*If tagged as number ... */
	if(strstr(node->tag, "number")){
		
		/* checking if there is any error in conversion */
		errno = 0;
		long x = strtol(node->contents, NULL, 10);
		return errno != ERANGE ? zval_number(x) : zval_error( ZERROR_BAD_NUMBER );
	}

	/*The operator is always second child */
	char* o = node->children[1]->contents;

	/* initalizing for x to store third child, later on*/
	/* by default a zval error of being a bad number */
	zval x = zval_error( ZERROR_BAD_NUMBER ); 

	/* special case of getting only one negative number or expression */
	if ( node->children_num == 4 && strcmp(o, "-") == 0) {
		x = evaluate_o(zval_number(0), o, evaluate(node->children[2]));
	}else {
		/* storing the third child in x */
	 	x = evaluate(node->children[2]);	
	}
	
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
		" 										                                                                \
			number 	   : /-?[0-9]+(\\.[0-9]*)?/	;                                                               \
			operator   : '+' | '-' | '*' | '/' | '%' | '^' |                                                    \
			             \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \"max\" | \"min\"  | \"pow\"  ;	    \
			expression : <number> | '(' <operator> <expression>+ ')' ;                                          \
			zuzeelik   : /^/ <operator> <expression>+ /$/ ; 		                                            \
		",
	Number, Operator, Expression, Zuzeelik);


	puts("zuzeelik [ version: v0.0.0-0.3.0 ] \n");
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
			printf("\n\nTotal number of nodes: %i\n\n", number_of_nodes(result.output));
			
			/*Print the evaluated answer */
			printf("Evaluated output: ");
			zval answer = evaluate(result.output);
			zval_println(answer);
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
