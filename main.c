#include <stdbool.h>
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


// running state

bool running_state = false;

// forward declarations for structs and unions
struct zlist;
union zdata;
struct zval;
struct zenv;

typedef struct zlist zlist;
typedef union zdata zdata;
typedef struct zval zval;
typedef struct zenv zenv;

// creating enumeration of possible zval types 
enum { ZVAL_DECIMAL, ZVAL_ERROR, ZVAL_SYMBOL, ZVAL_FUNCTION, ZVAL_SYM_EXPRESSION, ZVAL_QUOTE };

typedef zval* (*zbuiltin) (zenv*, zval*);

// declaring zlist struct
struct zlist {
	
 //count to the list "zval*"
 int count;

 // pointer to the list "zval*" 
 struct zval** cell;
};

// Declaring zdata union
union zdata {
 long double decimal;

 // error and symbol types has some string data 
 char* er;
 char* sy;

 zbuiltin fu;

 // zlist struct to hold other zval cells
 zlist* list;
};

// Declaring zval struct 
struct zval {
 int type;

 // zdata union to hold only one type of data at a time
 zdata* data;
};

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

// defining ZVAL_DEC
#define ZVAL_DEC(v) ZVAL_DATA(v)->decimal

// constructing a pointer to a new decimal zval 
zval* zval_decimal(long double x) {
 zval* val = zval_create(ZVAL_DECIMAL);
 ZVAL_DEC(val) = x;
 return val;
}

//defining ZVAL_ERR
#define ZVAL_ERR(v) ZVAL_DATA(v)->er

// constructing a pointer to a new error type zval 
zval* zval_error(char* err) {
 zval* val = zval_create(ZVAL_ERROR);
 ZVAL_ERR(val) = malloc((strlen(err) + 1));
 strcpy(ZVAL_ERR(val), err);
 return val;
}

// defining ZVAL_SYM
#define ZVAL_SYM(v) ZVAL_DATA(v)->sy

// constructing a pointer to new symbol type zval 
zval* zval_symbol(char* sym){
 zval* val = zval_create(ZVAL_SYMBOL);
 ZVAL_SYM(val) = malloc(strlen(sym + 1));
 strcpy(ZVAL_SYM(val), sym);
 return val;
}

// defining ZVAL_FUN
#define ZVAL_FUN(v) ZVAL_DATA(v)->fu

// constructing a pointer to new function type zval
zval* zval_function(zbuiltin fun){
 zval* val = zval_create(ZVAL_FUNCTION);
 ZVAL_FUN(val) = fun;
 return val;
}

// defining ZVAL_LIST
#define ZVAL_LIST(v) ZVAL_DATA(v)->list

// defining ZVAL_COUNT
#define ZVAL_COUNT(v) ZVAL_LIST(v)->count

// defining ZVAL_CELL
#define ZVAL_CELL(v) ZVAL_LIST(v)->cell

// constructing a pointer to a new zval zlist
zval* zval_zlist(zval* val, int count){

 ZVAL_LIST(val) = malloc(sizeof(zlist));

 if ( 0 < count ) {
		
  ZVAL_COUNT(val) = count;
  ZVAL_CELL(val) = malloc(sizeof(zval*) * ZVAL_COUNT(val));

 } else {
		
  // if `count` is zero then we are creating an empty zlist
  ZVAL_COUNT(val) = 0;
  ZVAL_CELL(val) = NULL;
		
  /* 
    `count` could never be negative but somehow if we a get negative `count`
     due to (future) bugs, then we are creating an empty `zlist` for it. */
 }

 return val;
} // creates an empty `zlist` if `count` is 0 or else makes a `zlist` of the specified size.

// constructing a pointer to new empty symbolic expressions 
zval* zval_sym_expression(void) {

 return zval_zlist(zval_create(ZVAL_SYM_EXPRESSION), 0);
}

// constructing a pointer to new empty quote
zval* zval_quote(void) {

 return zval_zlist(zval_create(ZVAL_QUOTE), 0);
}

void zval_delete(zval* val) {
 switch(ZVAL_TYPE(val)){
  
  // declaration for zval_sym_expression for loop
  int i;
  
  // do nothing special for decimal or function type 
  case ZVAL_DECIMAL: break;
  case ZVAL_FUNCTION: break;

  // if error or symbol free the string data 
  case ZVAL_ERROR:  free(ZVAL_ERR(val)); break;
  case ZVAL_SYMBOL: free(ZVAL_SYM(val)); break;

  // if symbolic expression or quote zval then delete all the elements inside 
  case ZVAL_QUOTE:
  case ZVAL_SYM_EXPRESSION: 
   
   for(i = 0; i < ZVAL_COUNT(val); i++ ) {
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

zval* zval_copy(zval* node) {

 zval* val = zval_create(ZVAL_TYPE(node));

 switch(ZVAL_TYPE(val)) {

  // copy decimals and functions directly
  case ZVAL_DECIMAL: ZVAL_DEC(val) = ZVAL_DEC(node); break;
  case ZVAL_FUNCTION: ZVAL_FUN(val) = ZVAL_FUN(node); break;

  // copy strings using malloc and strcpy
  case ZVAL_ERROR: 
   ZVAL_ERR(val) = malloc(strlen(ZVAL_ERR(node) + 1));
   strcpy(ZVAL_ERR(val), ZVAL_ERR(node)); break;

  case ZVAL_SYMBOL:
   ZVAL_SYM(val) = malloc(strlen(ZVAL_SYM(node) + 1));
   strcpy(ZVAL_SYM(val), ZVAL_SYM(node)); break;

   // copy zlists by copying each sub-expression
  case ZVAL_QUOTE:
  case ZVAL_SYM_EXPRESSION:

   val = zval_zlist(val, ZVAL_COUNT(node));
   
   int i;
   for ( i = 0; i < ZVAL_COUNT(val); i++) {
    ZVAL_CELL(val)[i] = zval_copy(ZVAL_CELL(node)[i]);
   }
  break;
 }
	
 return val;
}

zval* zval_increase(zval* val, zval* x){
 ZVAL_COUNT(val)++;
 ZVAL_CELL(val) =  realloc(ZVAL_CELL(val), sizeof(zval*) * ZVAL_COUNT(val));
 ZVAL_CELL(val)[ZVAL_COUNT(val) - 1] = x;
 return val;
}

zval* zval_read_decimal(mpc_ast_t* node) {
 errno = 0;
 long double x = strtold(node->contents, NULL);
 return errno != ERANGE ? zval_decimal(x) : zval_error( "Invalid decimal !");
}

// defining STR_MATCH
#define STR_MATCH(str1, str2) strcmp(str1, str2) == 0

zval* zval_read(mpc_ast_t* node) {
	
 // if symbol or decimal then returning to that type 
 if (strstr(node->tag, "decimal")) { return zval_read_decimal(node); }
 if (strstr(node->tag, "symbol")) { return zval_symbol(node->contents); }

 // if root (>) or sym-expression then creating a an empty list 
 zval* x = NULL;
 if(STR_MATCH(node->tag, ">")) { x = zval_sym_expression(); }
 if(strstr(node->tag, "sym_expression")) { x = zval_sym_expression(); }
 if(strstr(node->tag, "quote")) { x = zval_quote(); }

 // Filling this list with valid expressions contained within
 int i;
 for(i = 0; i < node->children_num; i ++) {
  if ( STR_MATCH(node->children[i]->contents, "(") ) {
   continue; 
  }
  if ( STR_MATCH(node->children[i]->contents, ")") ) {
   continue; 
  }
  if ( STR_MATCH(node->children[i]->contents, "[") ) {
   continue; 
  }
  if ( STR_MATCH(node->children[i]->contents, "]") ) {
   continue; 
  }
  if ( STR_MATCH(node->children[i]->tag, "regex") ) {
   continue; 
  }
  x = zval_increase(x, zval_read(node->children[i]));
 }
 return x;
}

// forward declaration for zval_print() used in zval_expression_print()
void zval_print(zval* val);

void zval_expression_print(zval* val, char start, char end) {
 putchar(start);
 
 int i;
 for(i = 0; i < ZVAL_COUNT(val); i ++) {
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

  case ZVAL_DECIMAL: printf("%Lf", ZVAL_DEC(val)); break;
  case ZVAL_FUNCTION: printf("<function>"); break;
  case ZVAL_ERROR: printf("[error]\nError response: %s", ZVAL_ERR(val)); break;
  case ZVAL_SYMBOL: printf("%s", ZVAL_SYM(val)); break;
  case ZVAL_SYM_EXPRESSION: zval_expression_print(val, '(', ')'); break;
  case ZVAL_QUOTE: zval_expression_print(val, '[', ']'); break;
 }
}

// printing zval followed by a new line 
void zval_println(zval* val){
 zval_print(val);
 putchar('\n');
}


// zuzeelik environment

struct zenv {
 int count;
 char** sym_list;
 zval** val_list;
};

// defining ZENV_COUNT
#define ZENV_COUNT(e) e->count

// defining ZENV_SYM_LIST
#define ZENV_SYM_LIST(e) e->sym_list

// defining ZENV_SYM_LIST
#define ZENV_VAL_LIST(e) e->val_list

// constructing a pointer to new zenv
zenv* zenv_create(void) {
	
 // initialize struct
 zenv* env = malloc(sizeof(zenv));
 env->count = 0;
 env->sym_list = NULL;
 env->val_list = NULL;
 return env;
}

void zenv_delete(zenv* env){

 // Iterate over all items in environment deleting them
 
 int i;
 for (i = 0; i < env->count; i++ ) {
  free(env->sym_list[i]);
   zval_delete(env->val_list[i]);
  }


 // Free allocated memory for lists
 free(env->sym_list);
 free(env->val_list);
 free(env);
}

zval* zenv_retrieve(zenv* env, zval* val) {
	
 // Iterate over all the items in environment
 int i;
 for ( i = 0; i < env->count; i++ ) {
		
  /* Check if the stored string matches the symbol string 
     If it does, return the copy of the value */	
  if( STR_MATCH(env->sym_list[i], ZVAL_SYM(val)) ) {
   return zval_copy(env->val_list[i]);
  }
}

 // If no symbol is found return error
 return zval_error("Unbound symbol !!");
}

void zenv_store(zenv* env, zval* sym, zval* val) {
	
 /* Iterate over all items in environment
    This is to see if variable already exists */
    
 int i;
 for ( i = 0; i < env->count; i++ ) {
		
  /* If the variable is found delete item at that position
    And replace with variable supplied by user (zuzeelik programmer) */
  if( STR_MATCH(env->sym_list[i], ZVAL_SYM(sym)) ) {
   zval_delete( env->val_list[i] );
   env->val_list[i] = zval_copy(val);
  return;
  }
 }

 // If no existing entry found allocate space for new entry 
 env->count++;
 env->val_list = realloc( env->val_list, sizeof(zval*) * env->count );
 env->sym_list = realloc( env->sym_list, sizeof(char*) * env->count );

 // copy contents of zval and symbol string into new location
 env->val_list[env->count - 1] = zval_copy(val);
 env->val_list[env->count - 1] = malloc(strlen(ZVAL_SYM(sym)) + 1);
 strcpy( ZENV_SYM_LIST(env)[ env->count - 1 ], ZVAL_SYM(sym) );

}
 
zval* zval_pop (zval* val, int i) {
	
 // finding the item at i
 zval* x  = ZVAL_CELL(val)[i];

 // shifting memory after the item at "i" over the top
 memmove(ZVAL_CELL(&val)[i], ZVAL_CELL(&val)[i+1], sizeof(zval*) * (ZVAL_COUNT(val) - i - 1));

 // decreasing the count of items in the list
 ZVAL_COUNT(val)--;

 // reallocating the memory used
 ZVAL_CELL(val) = realloc(ZVAL_CELL(val), sizeof(zval*) * ZVAL_COUNT(val));

 return x;
}

zval* zval_push(zval* node, zval* val, int i) {

 // increasing the count of items in the list
 ZVAL_COUNT(node)++;

 // reallocating the memory to be used
 ZVAL_CELL(node) = realloc(ZVAL_CELL(node), sizeof(zval*) * ZVAL_COUNT(node));

 // shifting memory at "i" to "i+1"
 memmove(ZVAL_CELL(&node)[i+1], ZVAL_CELL(&node)[i], sizeof(zval*) * (ZVAL_COUNT(node) -i -1));

 // placing the item at "i"
 ZVAL_CELL(node)[i] = val;

 return node;
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
zval* zval_evaluate(zenv* env, zval* val);

// defining QFC ( quote format checker )
#define QFC(args, cond, err ) \
 if( cond ) {zval_delete(args); return zval_error(err); }

// builtin function 'list'.
zval* builtin_list(zenv* env, zval* val) {
 ZVAL_TYPE(val) = ZVAL_QUOTE;
 return val;
}

// defining QAC ( quote argument checker )
#define QAC(args, cond, fn_err) \
 QFC(args, ZVAL_COUNT(args) != cond, fn_err);

// builtin function 'eval'
zval* builtin_eval(zenv* env, zval* node){
 QAC(node, 1, "Function 'eval' received too many arguments !");
 QFC(node, ZVAL_TYPE(ZVAL_CELL(node)[0]) != ZVAL_QUOTE, "Function 'eval' received incorrect types !");

 zval* val = zval_pick(node, 0);
 ZVAL_TYPE(val) = ZVAL_SYM_EXPRESSION;
 return zval_evaluate(env, val);
}

// defining EQC (empty quote checker )
#define EQC(args, fn_err) \
 QFC(args, ZVAL_COUNT(ZVAL_CELL(args)[0]) == 0, fn_err);

// builtin function 'head' for quotes
zval* builtin_head(zenv* env, zval* node){

 // checking for error conditions
 QAC(node, 1, "Function 'head' received too many arguments !" );
 QFC(node, ZVAL_TYPE(ZVAL_CELL(node)[0]) != ZVAL_QUOTE, "Function 'head' received incorrect types !" );
 EQC(node, "Function 'head' passed [] !" );

 // otherwise taking the first argument
 zval* val = zval_pick(node, 0);

 // delete all the arguments that are not head and return
 while(ZVAL_COUNT(val) > 1) { zval_delete( zval_pop( val, 1 ) ); }
 return val;
}

// builtin function 'cons' for quotes
zval* builtin_cons(zenv* env, zval* node) {

 // checking for error conditions
 QAC(node, 2, "Function 'cons' received incorrect number of arguments !");
 QFC(node, ZVAL_TYPE(ZVAL_CELL(node)[1]) != ZVAL_QUOTE, "Function 'cons' received incorrect types in 2nd argument !");

 // otherwise taking the first argument
 zval* val = zval_pop(node, 0);

 // then pushing the 1st argument into the 2nd argument and returning it
 zval* x = zval_push(zval_pick(node, 0), val, 0);
 return x;
}

// builtin function 'init' for quotes
zval* builtin_init(zenv* env, zval* node) {

 // checking for error conditions
 QAC(node, 1, "Function 'init' received too many arguments !");
 QFC(node, ZVAL_TYPE(ZVAL_CELL(node)[0]) != ZVAL_QUOTE, "Function 'init' received incorrect types !");
 EQC(node, "Function 'init', passed [] !");

 // otherwise taking the first argument
 zval* val = zval_pick(node, 0);

 // delete the last element and return
 zval_delete(zval_pop(val, (ZVAL_COUNT(val) - 1) ));
 return val;

}

// builtin function 'tail' for quotes
zval * builtin_tail(zenv* env, zval* node){

 // checking for error conditions
 QAC(node, 1, "Function 'tail' received too many arguments ! ");
 QFC(node, ZVAL_TYPE(ZVAL_CELL(node)[0]) != ZVAL_QUOTE, "Function 'tail' received incorrect types ! " );
 EQC(node, "Function 'tail' passed [] ! ");

 // otherwise taking the first argument
 zval* val = zval_pick(node, 0);

 // delete first element and return
 zval_delete(zval_pop(val, 0));
 return val;
}

// builtin function 'join' for quotes
zval* builtin_join(zenv* env, zval* node) {
 int i;
 for(i = 0; i < ZVAL_COUNT(node); i++ ){
  QFC(node, ZVAL_TYPE(ZVAL_CELL(node)[i]) != ZVAL_QUOTE, "Function 'join' passed incorrect types !");
 }

 zval* val = zval_pop(node, 0);

 while(ZVAL_COUNT(node)){ 
  val = zval_join(val, zval_pop(node, 0)); 
 }

 zval_delete(node);
 return val;
}

// builtin function 'len' for quotes
zval* builtin_len(zenv* env, zval* node) {
 QAC(node, 1, "Function 'len' received too many arguments ! ");
 QFC(node, ZVAL_TYPE(ZVAL_CELL(node)[0]) != ZVAL_QUOTE, "Function 'len' received incorrect types ! ");

 zval* val = zval_decimal(ZVAL_COUNT(ZVAL_CELL(node)[0]));

 zval_delete(node);
	
 return val;
}

// using operator string to see which operation to perform
zval* builtin_operators(zenv* env, zval* val, char* o) {

 // first ensuring all arguments are decimals
 int i;
 for(i = 0; i < ZVAL_COUNT(val); i ++ ){
  if (ZVAL_TYPE(ZVAL_CELL(val)[i]) != ZVAL_DECIMAL ) {
   zval_delete(val);
   return zval_error("Cannot operate on a non-decimal value !!");
  }
 }

 // popping the first element
 zval* x = zval_pop(val, 0);

 // if no arguments and a "sub" or a "-" then performing a unary negation
 if(( STR_MATCH(o, "-") || STR_MATCH(o, "sub") ) && ZVAL_COUNT(val) == 0) {
  ZVAL_DEC(x) = - ZVAL_DEC(x);
 }else {
  // while there are still elements remaining
  while(ZVAL_COUNT(val) > 0) {

   // popping the next element
   zval *y = zval_pop(val, 0);

   if ( STR_MATCH(o, "+") || STR_MATCH(o, "add") ) { 
    ZVAL_DEC(x) += ZVAL_DEC(y); 
   }
   else if ( STR_MATCH(o, "-") || STR_MATCH(o, "sub") ) { 
    ZVAL_DEC(x) -= ZVAL_DEC(y); 
   }
   else if ( STR_MATCH(o, "*") || STR_MATCH(o, "mul") ) { 
     ZVAL_DEC(x) *= ZVAL_DEC(y); 
   }
   else if ( STR_MATCH(o, "/") || STR_MATCH(o, "div") ) {

    // if the second operand is zero then returning an error and breaking out
    if( ZVAL_DEC(y) == 0 ){
     zval_delete(x); zval_delete(y);
     x = zval_error("Division by zero !!??"); 
     break;
    }else {
     ZVAL_DEC(x) /= ZVAL_DEC(y);
    }
   }
   else if ( STR_MATCH(o, "%") || STR_MATCH(o, "mod") ) {

    // Again, if the second operand is zero then returning an error and breaking out
    if( ZVAL_DEC(y) == 0 ){
     zval_delete(x); zval_delete(y);
     x = zval_error("Modulo by zero !! ??"); 
     break;
    }else {
     ZVAL_DEC(x) = fmod(ZVAL_DEC(x), ZVAL_DEC(y));
    }
   }
   else if ( STR_MATCH(o, "^") || STR_MATCH(o, "pow") ) {
    ZVAL_DEC(x) = pow(ZVAL_DEC(x), ZVAL_DEC(y)); 
   }
   else if ( STR_MATCH(o, "max") ) { 
    if( ZVAL_DEC(x) < ZVAL_DEC(y) ) { 
     ZVAL_DEC(x) = ZVAL_DEC(y); 
    }
   }
   else if ( STR_MATCH(o, "min") ) {
    if ( ZVAL_DEC(y) < ZVAL_DEC(x) ) { 
     ZVAL_DEC(x) = ZVAL_DEC(y);
    }
   }
   zval_delete(y);
  }
 }
 zval_delete(val); return x;
}

zval* exit_repl(zval* transfer){
  running_state = false;
  return transfer;
}

zval* builtin_add(zenv* env, zval* val ){
  return builtin_operators(env, val, "+");
}

zval* builtin_sub(zenv* env, zval* val){
  return builtin_operators(env, val, "-");
}

zval* builtin_mul(zenv* env, zval* val){
  return builtin_operators(env, val, "*");
}

zval* builtin_div(zenv* env, zval* val){
  return builtin_operators(env, val, "/");
}

zval* builtin_mod(zenv* env, zval* val){
  return builtin_operators(env, val, "%");
}

zval* builtin_pow(zenv* env, zval* val){
  return builtin_operators(env, val, "^");
}

zval* builtin_max(zenv* env, zval* val){
  return builtin_operators(env, val, "max");
}

zval* builtin_min(zenv* env, zval* val){
  return builtin_operators(env, val, "min");
}

void zenv_include_builtin(zenv* env, char* name, zbuiltin func) {
  zval* sym_name = zval_symbol(name);
  zval* func_val = zval_function(func);
  zenv_store(env, sym_name, func_val);
  zval_delete(sym_name); 
  zval_delete(func_val);

}

void zenv_include_builtins(zenv* env){
  
  // list functions
  zenv_include_builtin(env, "list", builtin_list);
  zenv_include_builtin(env, "head", builtin_head);
  zenv_include_builtin(env, "tail", builtin_tail);
  zenv_include_builtin(env, "eval", builtin_eval);
  zenv_include_builtin(env, "join", builtin_join);
  zenv_include_builtin(env, "init", builtin_init);
  zenv_include_builtin(env, "cons", builtin_cons);
  zenv_include_builtin(env, "len", builtin_len);


  // Mathematical functions
  zenv_include_builtin(env, "+", builtin_add);
  zenv_include_builtin(env, "-", builtin_sub);
  zenv_include_builtin(env, "*", builtin_mul);
  zenv_include_builtin(env, "/", builtin_div);
  zenv_include_builtin(env, "%", builtin_mod);
  zenv_include_builtin(env, "^", builtin_pow);

  // mathematical "wordy" functions
  zenv_include_builtin(env, "add", builtin_add);
  zenv_include_builtin(env, "sub", builtin_sub);
  zenv_include_builtin(env, "mul", builtin_mul);
  zenv_include_builtin(env, "div", builtin_div);
  zenv_include_builtin(env, "mod", builtin_mod);
  zenv_include_builtin(env, "pow", builtin_pow);
  zenv_include_builtin(env, "min", builtin_min);
  zenv_include_builtin(env, "max", builtin_max);

}


zval* zval_evaluate_sym_expression (zenv* env, zval* val) {

 // counter for for loops
int i;

 //evalualtion of the children
 for ( i = 0; i < ZVAL_COUNT(val); i++ ){
  ZVAL_CELL(val)[i] = zval_evaluate(env, ZVAL_CELL(val)[i]);
 }

 // checking for errors 
 
 for ( i = 0; i < ZVAL_COUNT(val); i++ ){
  if (ZVAL_TYPE(ZVAL_CELL(val)[i]) == ZVAL_ERROR ) { 
    return zval_pick(val, i); 
  }
 }

 // if getting an empty expression
 if (ZVAL_COUNT(val) == 0) { 
   return val; 
 }

 // if getting a single expression
 if (ZVAL_COUNT(val) == 1) { 
  return zval_pick(val, 0); 
 }

 // ensuring first element is a function
 zval* first_element = zval_pop(val, 0);
 if (ZVAL_TYPE(first_element) != ZVAL_FUNCTION ) {
  zval_delete(first_element); zval_delete(val);
  return zval_error("sym-expression is not starting with a function !!");
 }

 // if so then calling function to get the result
 zval* r = ZVAL_FUN(first_element)(env, val);
 zval_delete(first_element);

 return r;
}

zval* zval_evaluate(zenv* env, zval* val) {
	
 if ( ZVAL_TYPE(val) == ZVAL_SYMBOL ) {
   zval* x = zenv_retrieve(env, val);
   zval_delete(val);
   return x;
 }

 // evaluating sym-expressions 
 if ( ZVAL_TYPE(val) == ZVAL_SYM_EXPRESSION ) {
   return zval_evaluate_sym_expression(env, val);
 }
   // all the other zval types remains the same
  return val;
}


// count total number of nodes 
int number_of_nodes(mpc_ast_t* nodes) {
 if (nodes->children_num == 0) { return 1; }
  else if (nodes->children_num >= 1) {
   int total = 1;
   int i;
   for (i = 0; i < nodes->children_num; i++) {
    total = total + number_of_nodes(nodes->children[i]);
   }
   return total;
  }else { return 0; }
  // however, number of nodes could never be negative
  // done to suppress compiler warnings. 
}

int main(int argc, char** argv) {


 // creating some parsers 
 mpc_parser_t* Decimal = mpc_new("decimal");
 mpc_parser_t* Symbol = mpc_new("symbol");
 mpc_parser_t* Sym_expression = mpc_new("sym_expression");
 mpc_parser_t* Quote = mpc_new("quote");
 mpc_parser_t* Expression = mpc_new("expression");
 mpc_parser_t* Zuzeelik = mpc_new("zuzeelik");

 // defining them with following language 
 mpca_lang(MPCA_LANG_DEFAULT,
  "                                                                       \
   decimal 	       : /-?[0-9]+(\\.[0-9]*)?/	;                       \
   symbol         : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&^%]+/ ;               \
   sym_expression : '(' <expression>* ')' ;                            \
   quote          : '[' <expression>* ']' ;                            \
   expression     : <decimal> | <symbol> | <sym_expression> | <quote> ; \
   zuzeelik       : /^/ <expression>* /$/ ;                            \
  ",
  Decimal, Symbol, Sym_expression, Quote, Expression, Zuzeelik);


 puts("zuzeelik [ version: v0.0.0-0.5.3 ] \n");
 puts("Press Ctrl+C to Exit \n");


 zenv* env = zenv_create();
 zenv_include_builtins(env);
 
 // Starting REPL

 running_state = true; 
 while(running_state == true){

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
   zval* answer = zval_evaluate(env, zval_read(result.output));
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
 mpc_cleanup(6, Decimal, Symbol, Sym_expression, Quote, Expression, Zuzeelik);
 return 0;
}
