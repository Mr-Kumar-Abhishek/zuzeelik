#include "mpc/mpc.h"

// if compiling in windows, compiling with these functions 
#ifdef _WIN32
#include <string.h>

	static char buffer[2048];

	// fake readline functions 
	char* readline(char* prompt) {
		fputs(prompt, stdout);
		fgets(buffer, 2048, stdin);
		char* copy = malloc(strlen(buffer)+1);
		strcpy(copy, buffer);
		copy[strlen(copy) - 1] = '\0';
		return copy;
	}

	// fake add_history function 
	void add_history(char* not_used) {}

// or include these editline header 
#else 

	#include <editline/readline.h>

	// if not not OS X then include header file below
	#ifndef __APPLE__
		#include <editline/history.h>
	#endif

#endif

// creating enumeration of possible zval types 
enum { ZVAL_NUMBER, ZVAL_ERROR, ZVAL_SYMBOL, ZVAL_SYM_EXRESSION };

// Declaring new zval struct 
typedef struct zval {
	int type;
	long number;

	// error and symbol types has some string data 
	char* er;
	char* sy;

	// count and pointer to the list "zval*" 
	int count;
	struct zval** cell;
} zval;

// constructing a pointer to a new number zval 
zval* zval_number(long x) {
	zval* val = malloc(sizeof(zval));
	val->type = ZVAL_NUMBER;
	val->number = x;
	return val;
}

// constructing a pointer to a new error zval 
zval* zval_error(char* err) {
	zval* val = malloc(sizeof(zval));
	val->type = ZVAL_ERROR;
	val->er = malloc((strlen(err) + 1));
	strcpy(val->er, err);
	return val;
}

// constructing a pointer new symbol type zval 
zval* zval_symbol(char* sym){
	zval* val = malloc(sizeof(zval));
	val->type = ZVAL_SYMBOL;
	val->sy = malloc(strlen(sym + 1));
	strcpy(val->sy, sym);
	return val;
}

// constructing a pointer to new empty symbolic expressions 
zval* zval_sym_expression(void) {
	zval* val = malloc(sizeof(zval));
	val->type = ZVAL_SYM_EXRESSION;
	val->count = 0;
	val->cell = NULL;
	return val;
}

void zval_delete(zval* val) {
	switch(val->type){

		// do nothing special for number type 
		case ZVAL_NUMBER: break;

		// if error or symbol free the string data 
		case ZVAL_ERROR:  free(val->er); break;
		case ZVAL_SYMBOL: free(val->sy); break;

		// if symbolic expression then delete all the elements inside 
		case ZVAL_SYM_EXRESSION: 
			for( int i = 0; i < val->count; i++ ) {
				zval_delete(val->cell[i]);
			}

			// Also, free the memory contained in the pointers 
			free(val->cell);
		break;
	}
	
	// Lastly, free the memory for the zval struct itself 
	free(val);
}

zval* zval_read_number(mpc_ast_t* node) {
	errno = 0;
	long x = strtol(node->contents, NULL, 0);
	return errno != ERANGE ? zval_number(x) : zval_error( "Invalid number !");
}

zval* zval_increase(zval* val, zval* x){
	val->count++;
	val->cell =  realloc(val->cell, sizeof(zval*) * val->count);
	val->cell[val->count - 1] = x;
	return val;
}

zval* zval_read(mpc_ast_t* node) {
	
	// if symbol or number then returning to that type 
	if (strstr(node->tag, "number")) { return zval_read_number(node); }
	if (strstr(node->tag, "symbol")) { return zval_symbol(node->contents); }

	// if root (>) or sym-expression then creating a an empty list 
	zval* x = NULL;
	if(strcmp(node->tag, ">") == 0 ) { x = zval_sym_expression(); }
	if(strstr(node->tag, "sym_expression")) { x = zval_sym_expression(); }

	// Filling this list with valid expressions contained within
	for(int i = 0; i < node->children_num; i ++) {
		if ( strcmp(node->children[i]->contents, "(") == 0 ) { continue; }
		if ( strcmp(node->children[i]->contents, ")") == 0 ) { continue; }
		if ( strcmp(node->children[i]->tag, "regex") == 0 ) { continue; }
		x = zval_increase(x, zval_read(node->children[i]));
	}
	return x;
}

void zval_print(zval* );

void zval_expression_print(zval* val, char start, char end) {
	putchar(start);
	for(int i = 0; i < val->count; i ++) {
		// print the value contained within 
		zval_print(val->cell[i]);

		// don't print the trailing space if it is the last element
		if(i != val->count -1){
			putchar(' ');
		}
	}
	putchar(end);
}

// printing a zval 
void zval_print(zval* val) {
	switch(val->type) {

		case ZVAL_NUMBER: printf("%li", val->number); break;
		case ZVAL_ERROR: printf("Error: %s", val->er); break;
		case ZVAL_SYMBOL: printf("%s", val->sy); break;
		case ZVAL_SYM_EXRESSION: zval_expression_print(val, '(', ')'); break;
	}
}

// printing zval followed by a new line 
void zval_println(zval* val){
	zval_print(val);
	putchar('\n');
}

// count total number of nodes 
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

// using operator string to see which operation to perform 
/*
zval evaluate_o(zval x, char* o, zval y){

	// If any value is an error then return it 
	if ( x.type == ZVAL_ERROR ) { return x; }
	if ( y.type == ZVAL_ERROR ) { return y; }

	// otherwise doing calculations on number values 
	if(strcmp(o, "+") == 0 || strcmp(o, "add") == 0 ) { return zval_number( x.number + y.number ); }
	if(strcmp(o, "-") == 0 || strcmp(o, "sub") == 0 ) { return zval_number( x.number - y.number ); }
	if(strcmp(o, "/") == 0 || strcmp(o, "div") == 0 ) {
	 	
	 	//if the second operand is zero then returning an error
	 	return y.number == 0 ? zval_error( ZERROR_DIV_ZERO ): zval_number( x.number / y.number ); 
	}
	if(strcmp(o, "*") == 0 || strcmp(o, "mul") == 0 ) { return zval_number( x.number * y.number ); }
	if(strcmp(o, "%") == 0 || strcmp(o, "mod") == 0 ) {
		
		// Agian, if the second operand is zero then returning an error 	
		 return y.number == 0 ? zval_error( ZERROR_MOD_ZERO ): zval_number( x.number % y.number ); 
	}
	if(strcmp(o, "^") == 0 || strcmp(o, "pow") == 0 ) { return zval_number((pow(x.number,y.number))); }
	if(strcmp(o, "max") == 0 ){
		if (x.number<=y.number ){ return y; } else if(x.number>y.number){ return x; } 
	 }
	if(strcmp(o, "min") == 0 ){
		if (y.number<=x.number) { return y; } else if(y.number>x.number){ return x; }
	}
	return zval_error( ZERROR_BAD_OP );
}
*/
/*
zval evaluate(mpc_ast_t* node){

	// If tagged as number ... 
	if(strstr(node->tag, "number")){
		
		// checking if there is any error in conversion 
		errno = 0;
		long x = strtol(node->contents, NULL, 10);
		return errno != ERANGE ? zval_number(x) : zval_error( ZERROR_BAD_NUMBER );
	}

	// The operator is always second child 
	char* o = node->children[1]->contents;

	// initalizing for x to store third child, later on
	// by default a zval error of being a bad number 
	zval x = zval_error( ZERROR_BAD_NUMBER ); 

	// special case of getting only one negative number or expression 
	if ( node->children_num == 4 && strcmp(o, "-") == 0) {
		x = evaluate_o(zval_number(0), o, evaluate(node->children[2]));
	}else {
		// storing the third child in x 
	 	x = evaluate(node->children[2]);	
	}
	
	// Iterating the remaining children and combining (from fourth child) 
	int i = 3;
	while(strstr(node->children[i]->tag, "expr")){
		x = evaluate_o(x, o, evaluate(node->children[i]));
		i++;
	}
	return x;
}
*/
int main(int argc, char** argv) {

	// creating some parsers 
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sym_expression = mpc_new("sym_expression");
	mpc_parser_t* Expression = mpc_new("expression");
	mpc_parser_t* Zuzeelik = mpc_new("zuzeelik");

	// defining them with following language 
	mpca_lang(MPCA_LANG_DEFAULT,
		" 										                                                              \
			number 	     : /-?[0-9]+(\\.[0-9]*)?/	;                                                         \
			symbol       : '+' | '-' | '*' | '/' | '%' | '^' |                                                \
			              \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \"max\" | \"min\"  | \"pow\"  ;	  \
			sym_expression : '(' <expression>* ')' ;                                                          \
			expression   : <number> | <symbol> | <sym_expression> ;                                           \
			zuzeelik     : /^/ <expression>* /$/ ; 		                                                      \
		",
	Number, Symbol, Sym_expression, Expression, Zuzeelik);


	puts("zuzeelik [ version: v0.0.0-0.3.0 ] \n");
	puts("Press Ctrl+C to Exit \n");
	
	// Starting REPL 
	while(1){

		// output from the prompt 
		char* input = readline("zuzeelik> ");
		
		// Add input to history 
		add_history(input);
		
		// An attempt to parse the input 
		mpc_result_t result;
		if(mpc_parse("<stdin>", input, Zuzeelik, &result)) {

			// On success print the Abstract Syntax Tree 
			printf("\nAbstract Syntax Tree:\n\n");
			mpc_ast_print(result.output);
			printf("\n\nTotal number of nodes: %i\n\n", number_of_nodes(result.output)); 
			
		/*	// Print the evaluated answer 
			printf("Evaluated output: ");
			zval answer = evaluate(result.output);
			zval_println(answer); */

			//Print the the recieved input
			printf("Recieved input: ");
			zval* x = zval_read(result.output);
			zval_println(x);
			zval_delete(x);	
			mpc_ast_delete(result.output); 
		}else {

			// Or else print the error 
			mpc_err_print(result.error);
			mpc_err_delete(result.error);
		}

		// free retrieved input 
		free(input);
	}

	// undefining and deleting parsers 
	mpc_cleanup(5, Number, Symbol, Sym_expression, Expression, Zuzeelik);
	return 0;
}
