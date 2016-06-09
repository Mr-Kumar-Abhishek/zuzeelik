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

	// if OS X then include header file below
	#ifdef __APPLE__
		#include <AvailabilityMacros.h>

		// if MAC OS X version doesn't falls into this then include <editline/history.h>
		#if !(MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8)
			#include <editline/history.h>
		#endif
	#endif

#endif

// creating enumeration of possible zval types 
enum { ZVAL_NUMBER, ZVAL_ERROR, ZVAL_SYMBOL, ZVAL_SYM_EXRESSION, ZVAL_QUOTE };


// declaring new zlist struct
typedef struct zlist {
	
	//count to the list "zval*"
	int count;

	// pointer to the list "zval*" 
	struct zval** cell;
} zlist;

// Declaring new zdata union
typedef union zdata {
	long double number;

	// error and symbol types has some string data 
	char* er;
	char* sy;

	// zlist struct to hold other zval cells
	zlist* list;
} zdata;

// Declaring new zval struct 
typedef struct zval {
	int type;

	// zdata union to hold only one type of data at a time
	zdata* data;
} zval;

// defining ZVAL_TYPE
#define ZVAL_TYPE(v) v->type

// defining ZVAL_DATA
#define ZVAL_DATA(v) v->data

// constructing a pointer to new zval
zval* zval_create(int zval_type) {
	zval* val = malloc(sizeof(zval));
	ZVAL_TYPE(val) = zval_type;
	ZVAL_DATA(val) = malloc(sizeof(zdata));
	return val;
}

// defining ZVAL_NUM
#define ZVAL_NUM(v) v->data->number

// constructing a pointer to a new number zval 
zval* zval_number(long double x) {
	zval* val = zval_create(ZVAL_NUMBER);
	ZVAL_NUM(val) = x;
	return val;
}

//defining ZVAL_ERR
#define ZVAL_ERR(v) v->data->er

// constructing a pointer to a new error type zval 
zval* zval_error(char* err) {
	zval* val = zval_create(ZVAL_ERROR);
	ZVAL_ERR(val) = malloc((strlen(err) + 1));
	strcpy(ZVAL_ERR(val), err);
	return val;
}

// defining ZVAL_SYM
#define ZVAL_SYM(v) v->data->sy

// constructing a pointer new symbol type zval 
zval* zval_symbol(char* sym){
	zval* val = zval_create(ZVAL_SYMBOL);
	ZVAL_SYM(val) = malloc(strlen(sym + 1));
	strcpy(ZVAL_SYM(val), sym);
	return val;
}

// defining ZVAL_LIST
#define ZVAL_LIST(v) v->data->list

// defining ZVAL_COUNT
#define ZVAL_COUNT(v) v->data->list->count

// defining ZVAL_CELL
#define ZVAL_CELL(v) v->data->list->cell

// constructing a pointer to new empty symbolic expressions 
zval* zval_sym_expression(void) {
	zval* val = zval_create(ZVAL_SYM_EXRESSION);
	ZVAL_LIST(val) = malloc(sizeof(zlist));
	ZVAL_COUNT(val) = 0;
	ZVAL_CELL(val) = NULL;
	return val;
}

// constructing a pointer to new empty quote
zval* zval_quote(void) {
	zval* val = zval_create(ZVAL_QUOTE);
	ZVAL_LIST(val) = malloc(sizeof(zlist));
	ZVAL_COUNT(val) = 0;
	ZVAL_CELL(val) = NULL;
	return val;
}

void zval_delete(zval* val) {
	switch(ZVAL_TYPE(val)){

		// do nothing special for number type 
		case ZVAL_NUMBER: break;

		// if error or symbol free the string data 
		case ZVAL_ERROR:  free(ZVAL_ERR(val)); break;
		case ZVAL_SYMBOL: free(ZVAL_SYM(val)); break;

		// if symbolic expression or quote zval then delete all the elements inside 
		case ZVAL_QUOTE:
		case ZVAL_SYM_EXRESSION: 
			for( int i = 0; i < ZVAL_COUNT(val); i++ ) {
				zval_delete(ZVAL_CELL(val)[i]);
			}

			// free the memory contained in the pointers 
			free(ZVAL_CELL(val));

			// Also, free the memory contained in zlist.
			free(ZVAL_LIST(val));
		break;
	}
	
	//free zdata union
	free(val->data);

	// Lastly, free the memory for the zval struct itself 
	free(val);
}

zval* zval_increase(zval* val, zval* x){
	ZVAL_COUNT(val)++;
	ZVAL_CELL(val) =  realloc(ZVAL_CELL(val), sizeof(zval*) * ZVAL_COUNT(val));
	ZVAL_CELL(val)[ZVAL_COUNT(val) - 1] = x;
	return val;
}

zval* zval_read_number(mpc_ast_t* node) {
	errno = 0;
	long double x = strtold(node->contents, NULL);
	return errno != ERANGE ? zval_number(x) : zval_error( "Invalid number !");
}

// defining STR_MATCH
#define STR_MATCH(str1, str2) strcmp(str1, str2) == 0

zval* zval_read(mpc_ast_t* node) {
	
	// if symbol or number then returning to that type 
	if (strstr(node->tag, "number")) { return zval_read_number(node); }
	if (strstr(node->tag, "symbol")) { return zval_symbol(node->contents); }

	// if root (>) or sym-expression then creating a an empty list 
	zval* x = NULL;
	if(STR_MATCH(node->tag, ">")) { x = zval_sym_expression(); }
	if(strstr(node->tag, "sym_expression")) { x = zval_sym_expression(); }
	if(strstr(node->tag, "quote")) { x = zval_quote(); }

	// Filling this list with valid expressions contained within
	for(int i = 0; i < node->children_num; i ++) {
		if ( STR_MATCH(node->children[i]->contents, "(") ) { continue; }
		if ( STR_MATCH(node->children[i]->contents, ")") ) { continue; }
		if ( STR_MATCH(node->children[i]->contents, "[") ) { continue; }
		if ( STR_MATCH(node->children[i]->contents, "]") ) { continue; }
		if ( STR_MATCH(node->children[i]->tag, "regex") ) { continue; }
		x = zval_increase(x, zval_read(node->children[i]));
	}
	return x;
}

// forward declaration for zval_print() used in zval_expression_print()
void zval_print(zval* val);

void zval_expression_print(zval* val, char start, char end) {
	putchar(start);
	for(int i = 0; i < ZVAL_COUNT(val); i ++) {
		// print the value contained within 
		zval_print(ZVAL_CELL(val)[i]);

		// don't print the trailing space if it is the last element
		if(i != ZVAL_COUNT(val) -1){
			putchar(' ');
		}
	}
	putchar(end);
}

// printing a zval 
void zval_print(zval* val) {
	switch(ZVAL_TYPE(val)) {

		case ZVAL_NUMBER: printf("%Lf", ZVAL_NUM(val)); break;
		case ZVAL_ERROR: printf("[error]\nError response: %s", ZVAL_ERR(val)); break;
		case ZVAL_SYMBOL: printf("%s", ZVAL_SYM(val)); break;
		case ZVAL_SYM_EXRESSION: zval_expression_print(val, '(', ')'); break;
		case ZVAL_QUOTE: zval_expression_print(val, '[', ']'); break;
	}
}

// printing zval followed by a new line 
void zval_println(zval* val){
	zval_print(val);
	putchar('\n');
}

zval* zval_pop (zval* val, int i) {
	
	// finding the item at i
	zval* x  = ZVAL_CELL(val)[i];

	// shifting memory after the item at "i" over the top
	memmove(ZVAL_CELL(&val)[i], ZVAL_CELL(&val)[i+1], sizeof(zval*) * (ZVAL_COUNT(val) - i - 1));

	// decreasing the count of items in the list
	ZVAL_COUNT(val)--;

	// relocating the memory used
	ZVAL_CELL(val) = realloc(ZVAL_CELL(val), sizeof(zval*) * ZVAL_COUNT(val));

	return x;
}

zval* zval_pick(zval* val, int i) {
	zval* x = zval_pop(val, i);
	zval_delete(val);
	return x;
}

zval* zval_join(zval* x, zval* y){
	
	// for each cell in 'y' add it to 'x'
	while( ZVAL_COUNT(y) ) {
		x = zval_increase(x, zval_pop(y, 0));
	}

	// delete the empty 'y' and return 'x'
	zval_delete(y);
	return x;
}

// forward declatation of zval_evaluate() used in zval_evaluate_sym_expression() and builtin_eval()
zval* zval_evaluate(zval* val);

// defining QFC ( quote format checker )
#define QFC(args, cond, err ) \
	if( cond ) {zval_delete(args); return zval_error(err); }

// builtin function 'list'.
zval* builtin_list(zval* val) {
	ZVAL_TYPE(val) = ZVAL_QUOTE;
	return val;
}

// builtin function 'eval'
zval* builtin_eval(zval* node){
	QFC(node, ZVAL_COUNT(node) != 1, "Function 'eval' received too many arguments !");
	QFC(node, ZVAL_TYPE(ZVAL_CELL(node)[0]) != ZVAL_QUOTE, "Function 'eval' received incorrect types !");

	zval* val = zval_pick(node, 0);
	ZVAL_TYPE(val) = ZVAL_SYM_EXRESSION;
	return zval_evaluate(val);
}

// builtin function 'head' for quotes
zval* builtin_head(zval* node){

	// checking for error conditions
	QFC(node, ZVAL_COUNT(node) != 1, "Function 'head' received too many arguments !" );
	QFC(node, ZVAL_TYPE(ZVAL_CELL(node)[0]) != ZVAL_QUOTE, "Function 'head' received incorrect types !" );
	QFC(node, ZVAL_COUNT(ZVAL_CELL(node)[0]) == 0, "Function 'head' passed [] !" );

	// otherwise taking the first argument
	zval* val = zval_pick(node, 0);

	// delete all the arguments that are not head and return
	while(ZVAL_COUNT(val) > 1) { zval_delete( zval_pop( val, 1 ) ); }
	return val;

}

// builtin function 'tail' for quotes
zval * builtin_tail(zval* node){

	// checking for error conditions
	QFC(node, ZVAL_COUNT(node) != 1, "Function 'tail' received too many arguments ! ");
	QFC(node, ZVAL_TYPE(ZVAL_CELL(node)[0]) != ZVAL_QUOTE, "Function 'tail' received incorrect types ! " );
	QFC(node, ZVAL_COUNT(ZVAL_CELL(node)[0]) == 0, "Function 'tail' passed [] ! ");

	// otherwise taking the first argument
	zval* val = zval_pick(node, 0);

	// delete first element and return
	zval_delete(zval_pop(val, 0));
	return val;

}

// builtin function 'join' for quotes
zval* builtin_join(zval* node) {
	for(int i = 0; i < ZVAL_COUNT(node); i++ ){
		QFC(node, ZVAL_TYPE(ZVAL_CELL(node)[i]) != ZVAL_QUOTE, "Function 'join' passed incorrect types !");
	}

	zval*val = zval_pop(node, 0);

	while(ZVAL_COUNT(node)){ val = zval_join(val, zval_pop(node, 0)); }

	zval_delete(node);
	return val;
}

// using operator string to see which operation to perform
zval* builtin_operators(zval* val, char* o) {

	// first ensuring all arguments are numbers
	for(int i = 0; i < ZVAL_COUNT(val); i ++ ){
		if (ZVAL_TYPE(ZVAL_CELL(val)[i]) != ZVAL_NUMBER ) {
			zval_delete(val);
			return zval_error("Cannot operate on a non-number !!");
		}
	}

	// popping the first element
	zval* x = zval_pop(val, 0);

	// if no arguments and a "sub" or a "-" then performing a unary negation
	if(( STR_MATCH(o, "-") || STR_MATCH(o, "sub") ) && ZVAL_COUNT(val) == 0) {
		ZVAL_NUM(x) = - ZVAL_NUM(x);
	}

	// while there are still elements remaining
	while(ZVAL_COUNT(val) > 0) {

		// popping the next element
		zval *y = zval_pop(val, 0);

		if ( STR_MATCH(o, "+") || STR_MATCH(o, "add") ) { ZVAL_NUM(x) += ZVAL_NUM(y); }
		if ( STR_MATCH(o, "-") || STR_MATCH(o, "sub") ) { ZVAL_NUM(x) -= ZVAL_NUM(y); }
		if ( STR_MATCH(o, "*") || STR_MATCH(o, "mul") ) { ZVAL_NUM(x) *= ZVAL_NUM(y); }
		if ( STR_MATCH(o, "/") || STR_MATCH(o, "div") ) {

			// if the second operand is zero then returning an error and breaking out
			if( ZVAL_NUM(y) == 0 ){
				zval_delete(x); zval_delete(y);
				x = zval_error("Division by zero !!??"); break;
			}
			ZVAL_NUM(x) /= ZVAL_NUM(y); 
		}
		if ( STR_MATCH(o, "%") || STR_MATCH(o, "mod") ) {

			// Again, if the second operand is zero then returning an error and breaking out
			if( ZVAL_NUM(y) == 0 ){
				zval_delete(x); zval_delete(y);
				x = zval_error("Modulo by zero !! ??"); break;
			}
			ZVAL_NUM(x) = fmod(ZVAL_NUM(x), ZVAL_NUM(y));
		}
		if ( STR_MATCH(o, "^") || STR_MATCH(o, "pow") ) {
			ZVAL_NUM(x) = pow(ZVAL_NUM(x), ZVAL_NUM(y)); 
		}
		if ( STR_MATCH(o, "max") ) { 
			if( ZVAL_NUM(x) < ZVAL_NUM(y) ) { ZVAL_NUM(x) = ZVAL_NUM(y); }
		}
		if ( STR_MATCH(o, "min") ) {
			if ( ZVAL_NUM(y) < ZVAL_NUM(x) ) { ZVAL_NUM(x) = ZVAL_NUM(y);}
		}
		zval_delete(y);
	}
	zval_delete(val); return x;
}

// builtin lookup for functions 
zval* builtin_lookup (zval* node, char* fn){
	if( STR_MATCH("head", fn) ) { return builtin_head(node); }
	if( STR_MATCH("tail", fn) ) { return builtin_tail(node); }
	if( STR_MATCH("list", fn) ) { return builtin_list(node); }
	if( STR_MATCH("eval", fn) ) { return builtin_eval(node); }
	if( STR_MATCH("join", fn) ) { return builtin_join(node); }
	if( strstr("+-/*%^", fn) ||
		STR_MATCH("add", fn) || STR_MATCH("sub", fn ) || 
		STR_MATCH("mul", fn) || STR_MATCH("div", fn ) || 
		STR_MATCH("mod", fn) || STR_MATCH("pow", fn ) || 
		STR_MATCH("min", fn) || STR_MATCH("max", fn ) ){
			return builtin_operators(node, fn);
		}
	zval_delete(node);
	return zval_error("Unknown function !!");
}

zval* zval_evaluate_sym_expression (zval* val) {

	//evalualtion of the children
	for ( int i = 0; i < ZVAL_COUNT(val); i++ ){
		ZVAL_CELL(val)[i] = zval_evaluate(ZVAL_CELL(val)[i]);
	}

	// checking for errors 
	for (int i = 0; i < ZVAL_COUNT(val); i++ ){
		if (ZVAL_TYPE(ZVAL_CELL(val)[i]) == ZVAL_ERROR ) { return zval_pick(val, i); }
	}

	// if getting an empty expression
	if (ZVAL_COUNT(val) == 0) { return val; }

	// if getting a single expression
	if (ZVAL_COUNT(val) == 1) { return zval_pick(val, 0); }

	// ensuring first element is a symbol
	zval* first_element = zval_pop(val, 0);
	if (ZVAL_TYPE(first_element) != ZVAL_SYMBOL ) {
		zval_delete(first_element); zval_delete(val);
		return zval_error("sym-expression is not starting with a symbol !!");
	}

	// calling builtin lookups
	zval* r = builtin_lookup(val, ZVAL_SYM(first_element));
	return r;
}

zval* zval_evaluate(zval* val) {
	
	// evaluating sym-expressions 
	if ( ZVAL_TYPE(val) == ZVAL_SYM_EXRESSION ) { return zval_evaluate_sym_expression(val);}

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
	mpc_parser_t* Quote = mpc_new("quote");
	mpc_parser_t* Expression = mpc_new("expression");
	mpc_parser_t* Zuzeelik = mpc_new("zuzeelik");

	// defining them with following language 
	mpca_lang(MPCA_LANG_DEFAULT,
		"                                                                                                          \
			number 	       : /-?[0-9]+(\\.[0-9]*)?/	;                                                          \
			symbol         : '+' | '-' | '*' | '/' | '%' | '^' |                                               \
			                \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \"max\" | \"min\"  | \"pow\"  |  \
			                \"head\" | \"tail\" | \"list\" | \"eval\" | \"join\"                            ;  \
			sym_expression : '(' <expression>* ')' ;                                                           \
			quote          : '[' <expression>* ']' ;                                                           \
			expression     : <number> | <symbol> | <sym_expression> | <quote> ;                                \
			zuzeelik       : /^/ <expression>* /$/ ;                                                           \
		",
	Number, Symbol, Sym_expression, Quote, Expression, Zuzeelik);


	puts("zuzeelik [ version: v0.0.0-0.5.1 ] \n");
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
			
			//Print the the received input
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
	mpc_cleanup(6, Number, Symbol, Sym_expression, Quote, Expression, Zuzeelik);
	return 0;
}
