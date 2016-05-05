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

// Declaring new zdata union
typedef union zdata {
	long number;

	// error and symbol types has some string data 
	char* er;
	char* sy;

	//count to the list "zval*"
	int count;
} zdata;

// Declaring new zval struct 
typedef struct zval {
	int type;

	// zdata union to hold only one type of data at a time
	zdata data;

	// pointer to the list "zval*" 
	struct zval** cell;
} zval;

// constructing a pointer to a new number zval 
zval* zval_number(long x) {
	zval* val = malloc(sizeof(zval));
	val->type = ZVAL_NUMBER;
	val->data.number = x;
	return val;
}

// constructing a pointer to a new error type zval 
zval* zval_error(char* err) {
	zval* val = malloc(sizeof(zval));
	val->type = ZVAL_ERROR;
	val->data.er = malloc((strlen(err) + 1));
	strcpy(val->data.er, err);
	return val;
}

// constructing a pointer new symbol type zval 
zval* zval_symbol(char* sym){
	zval* val = malloc(sizeof(zval));
	val->type = ZVAL_SYMBOL;
	val->data.sy = malloc(strlen(sym + 1));
	strcpy(val->data.sy, sym);
	return val;
}

// constructing a pointer to new empty symbolic expressions 
zval* zval_sym_expression(void) {
	zval* val = malloc(sizeof(zval));
	val->type = ZVAL_SYM_EXRESSION;
	val->data.count = 0;
	val->cell = NULL;
	return val;
}

void zval_delete(zval* val) {
	switch(val->type){

		// do nothing special for number type 
		case ZVAL_NUMBER: break;

		// if error or symbol free the string data 
		case ZVAL_ERROR:  free(val->data.er); break;
		case ZVAL_SYMBOL: free(val->data.sy); break;

		// if symbolic expression then delete all the elements inside 
		case ZVAL_SYM_EXRESSION: 
			for( int i = 0; i < val->data.count; i++ ) {
				zval_delete(val->cell[i]);
			}

			// Also, free the memory contained in the pointers 
			free(val->cell);
		break;
	}
	
	// Lastly, free the memory for the zval struct itself 
	free(val);
}

zval* zval_increase(zval* val, zval* x){
	val->data.count++;
	val->cell =  realloc(val->cell, sizeof(zval*) * val->data.count);
	val->cell[val->data.count - 1] = x;
	return val;
}

zval* zval_read_number(mpc_ast_t* node) {
	errno = 0;
	long x = strtol(node->contents, NULL, 0);
	return errno != ERANGE ? zval_number(x) : zval_error( "Invalid number !");
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

// forward declaration for zval_print() used in zval_expression_print()
void zval_print(zval* val);

void zval_expression_print(zval* val, char start, char end) {
	putchar(start);
	for(int i = 0; i < val->data.count; i ++) {
		// print the value contained within 
		zval_print(val->cell[i]);

		// don't print the trailing space if it is the last element
		if(i != val->data.count -1){
			putchar(' ');
		}
	}
	putchar(end);
}

// printing a zval 
void zval_print(zval* val) {
	switch(val->type) {

		case ZVAL_NUMBER: printf("%li", val->data.number); break;
		case ZVAL_ERROR: printf("[error]\nError response: %s", val->data.er); break;
		case ZVAL_SYMBOL: printf("%s", val->data.sy); break;
		case ZVAL_SYM_EXRESSION: zval_expression_print(val, '(', ')'); break;
	}
}

// printing zval followed by a new line 
void zval_println(zval* val){
	zval_print(val);
	putchar('\n');
}

zval* zval_pop (zval* val, int i) {
	
	// finding the item at i
	zval* x  = val->cell[i];

	// shifting memory after the item at "i" over the top
	memmove(&val->cell[i], &val->cell[i+1], sizeof(zval*) * val->data.count - i - 1);

	// decreasing the count of items in the list
	val->data.count--;

	// relocating the memory used
	val->cell = realloc(val->cell, sizeof(zval*) * val->data.count);

	return x;
}

zval* zval_pick(zval* val, int i) {
	zval* x = zval_pop(val, i);
	zval_delete(val);
	return x;
}

// using operator string to see which operation to perform
zval* builtin_operators(zval* val, char* o) {

	// first ensuring all arguments are numbers
	for(int i = 0; i < val->data.count; i ++ ){
		if (val->cell[i]->type != ZVAL_NUMBER ) {
			zval_delete(val);
			return zval_error("Cannot operate on a non-number !!");
		}
	}

	// popping the first element
	zval* x = zval_pop(val, 0);

	// if no arguments and a "sub" or a "-" then performing a unary negation
	if((strcmp(o, "-") == 0 || strcmp(o, "sub") == 0 ) && val->data.count == 0) {
		x->data.number = - x->data.number;
	}

	// while there are still elements remaining
	while(val->data.count > 0) {

		// popping the next element
		zval *y = zval_pop(val, 0);

		if (strcmp(o, "+") == 0 || strcmp(o, "add") == 0 ) { x->data.number += y->data.number; }
		if (strcmp(o, "-") == 0 || strcmp(o, "sub") == 0 ) { x->data.number -= y->data.number; }
		if (strcmp(o, "*") == 0 || strcmp(o, "mul") == 0 ) { x->data.number *= y->data.number; }
		if (strcmp(o, "/") == 0 || strcmp(o, "div") == 0 ) {

			// if the second operand is zero then returning an error and breaking out
			if( y->data.number == 0 ){
				zval_delete(x); zval_delete(y);
				x = zval_error("Division by zero !!??"); break;
			}
			x->data.number /= y->data.number; 
		}
		if ( strcmp(o, "%") == 0 || strcmp(o, "mod") == 0 ) {

			// Again, if the second operand is zero then returning an error and breaking out
			if( y->data.number == 0 ){
				zval_delete(x); zval_delete(y);
				x = zval_error("Modulo by zero !! ??"); break;
			}
			x->data.number %= y->data.number;
		}
		if ( strcmp(o, "^") == 0 || strcmp(o, "pow") == 0 ) { x->data.number = pow(x->data.number, y->data.number); }
		if ( strcmp(o, "max") == 0) { 
			if( x->data.number < y->data.number ) { x->data.number = y->data.number; }
		}
		if ( strcmp(o, "min") == 0 ) {
			if ( y->data.number < y->data.number ) { x->data.number = y->data.number;}
		}
		zval_delete(y);
	}
	zval_delete(val); return x;
}

// forward declatation of zval_evaluate() used in zval_evaluate_sym_expression()
zval* zval_evaluate(zval* val);

zval* zval_evaluate_sym_expression (zval* val) {

	//evalualtion of the children
	for ( int i = 0; i < val->data.count; i++ ){
		val->cell[i] = zval_evaluate(val->cell[i]);
	}

	// checking for errors 
	for (int i = 0; i < val->data.count; i++ ){
		if (val->cell[i]->type == ZVAL_ERROR ) { return zval_pick(val, i); }
	}

	// if getting an empty expression
	if (val->data.count == 0) { return val; }

	// if getting a single expression
	if (val->data.count == 1) { return zval_pick(val, 0); }

	// ensuring first element is a symbol
	zval* first_element = zval_pop(val, 0);
	if (first_element->type != ZVAL_SYMBOL ) {
		zval_delete(first_element); zval_delete(val);
		return zval_error("sym-expression is not starting with a symbol !!");
	}

	// calling builtin operators
	zval* r = builtin_operators(val, first_element->data.sy);
	return r;
}

zval* zval_evaluate(zval* val) {
	
	// evaluating sym-expressions 
	if ( val->type == ZVAL_SYM_EXRESSION ) { return zval_evaluate_sym_expression(val);}

	// all the other zval types remains the same
	return val;
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


	puts("zuzeelik [ version: v0.0.0-0.4.0 ] \n");
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
			
			//Print the the recieved input
			printf("Received input: ");
			zval* received = zval_read(result.output);
			zval_println(received);
			zval_delete(received);

			// print evaluated answer
			printf("Evaluated output: ");
			zval* answer = zval_evaluate(zval_read(result.output));
			zval_println(answer);
			zval_delete(answer);	
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
