#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

/* for Windows */
#ifdef _WIN32

#include <string.h>

static char buffer[2048];

/* Fake Unix readline function */
char* readline(char* prompt){
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;	
}

/* Fake add_history function */
void add_history(char* unused){}

#else

/* Otherwise include editline headers */
#include <editline/readline.h>
#include <editline/history.h>

#endif

typedef struct {
	int type;
	long num;
	int err;
}fval;

enum {FVAL_NUM, FVAL_ERR};
enum {FERR_DIV_ZERO, FERR_BAD_OP, FERR_BAD_NUM};

fval fval_num(long x){
	fval f;
	f.type = FVAL_NUM;
	f.num = x;
	return f;	
}

fval fval_err(int x){
	fval f;
	f.type = FVAL_ERR;
	f.err = x;
	return f;
}

/*print fval*/
void fval_print(fval f){
	switch(f.type){
		case FVAL_NUM: printf("%li", f.num); break;
		case FVAL_ERR:
			switch(f.err){
				case FERR_DIV_ZERO: printf("Error: Division By Zero!"); break;
				case FERR_BAD_OP: printf("Error: Invalid Operator!"); break;
				case FERR_BAD_NUM: printf("Error: Invalid Number!"); break;
			}
			break;
	}		
}

/* Print an "lval" followed by a newline */
void fval_println(fval f) { fval_print(f); putchar('\n'); }

int number_of_nodes(mpc_ast_t* t){
	if (t->children_num == 0) { return 1; }
	if (t->children_num >= 1) {
		int total = 1;
		for (int i = 0; i < t->children_num; i++) {
			total += number_of_nodes(t->children[i]);
		}
		return total;	
	}
	return 0;
}
	
int number_of_leaves(mpc_ast_t* t) { 
	if (t->children_num == 0) { 
		if(strlen(t->contents) > 0) {	
			return 1; 
		}else{
			return 0; 
		}
	}
	if (t->children_num >=1) {
		int total = 0;
		for (int i = 0; i < t->children_num; i++) {
			total += number_of_leaves(t->children[i]);
		}
		return total;
	}
	return 0;
}
	
int number_of_branches(mpc_ast_t* t) {
	if (t->children_num == 0) { 
		if(strlen(t->contents) == 0) {	
			return 1; 
		}else{
			return 0; 
		}
	}
	if (t->children_num >=1) {
		int total = 1;
		for (int i = 0; i < t->children_num; i++) {
			total += number_of_branches(t->children[i]);
		}
		return total;
	}
	return 0; 
}
			
fval eval_op(fval x, char* op, fval y){
	/* If either value is an error return it */
	if (x.type == FVAL_ERR) { return x; }
	if (y.type == FVAL_ERR) { return y; }
	
	if(strcmp(op, "+") == 0) { return fval_num(x.num + y.num); }
	if(strcmp(op, "-") == 0) { return fval_num(x.num - y.num); }
	if(strcmp(op, "*") == 0) { return fval_num(x.num * y.num); }
	if(strcmp(op, "/") == 0) { 
		return y.num == 0 ? fval_err(FERR_DIV_ZERO) : fval_num(x.num / y.num); 
	}
	if(strcmp(op, "%") == 0) { return fval_num(x.num % y.num); }
	if(strcmp(op, "^") == 0) { 
		fval res;
		res = fval_num(1);			
		for (int i = 0; i < y.num; i++){
			res = fval_num(res.num * x.num);
		}
		return res;
	}
	if(strcmp(op, "min") == 0) {
		return (x.num > y.num) ? y : x;
	}
	if(strcmp(op, "max") == 0) {
		return (x.num > y.num) ? x : y;
	}
		
	return fval_err(FERR_BAD_OP);
}
	
fval eval(mpc_ast_t* t){
	/* If tagged as number return it directly, otherwise expression. */ 
	if(strstr(t->tag, "number")) { 
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? fval_num(x) : fval_err(FERR_BAD_NUM);
	}
		
	/* The operator is always second child */
	char* op = t->children[1]->contents;
	fval x = eval(t->children[2]);
		
	/* Iterate remining children combining our operator */
	int i = 3;
	if (strstr(t->children[i]->tag, "expr")){
		while(strstr(t->children[i]->tag, "expr")){
			x = eval_op(x, op, eval(t->children[i]));
			i++;
		}
	}else{
		if(strcmp(op, "-") == 0){	x = fval_num(x.num * -1); }
	}		
	return x;
}

int main(int argc, char** argv){

	/* Create Some Parsers */
	mpc_parser_t* Number   = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr     = mpc_new("expr");
	mpc_parser_t* Fuss    = mpc_new("fuss");

	/* Define them with the following Language */
	mpca_lang(MPC_LANG_DEFAULT,
	  "                                                     \
		number   : /-?[0-9]+/ ;                             \
		operator : '+' | '-' | '*' | '/' | '%' | '^' | /min/ | /max/ ;    \
		expr     : <number> | '(' <operator> <expr>+ ')' ;  \
		fuss    : /^/ <operator> <expr>+ /$/ ;             \
	  ",
	  Number, Operator, Expr, Fuss);
	 
	/* Do some parsing here... */

	puts("Fuss Version 0.0.2");
	puts("Press Ctrl+C to Exit \n");
	
	while(1){
		char* input = readline("--fuss-->> ");
		add_history(input);
		
		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Fuss, &r)){
			/*On success, print asc */
			//mpc_ast_print(r.output);
			
			//printf("------------------------\n");
			
			/*Load AST from output */
			//mpc_ast_t* a = r.output;
			/*
			printf("Tag: %s\n", a->tag);
			printf("Contents: %s\n", a->contents);
			printf("Number of children: %i\n", a->children_num);
			printf("------------------------\n");
			*/		
			/*first children*/
			/*
			mpc_ast_t* c0 = a->children[0];
			long n_of_elements = sizeof(a->children) / sizeof(a->children[0]);
			printf("Number of elements of a child: %li\n", n_of_elements);
			printf("First Child Tag: %s\n", c0->tag);
			printf("First Child Contents: %s\n", c0->contents);
			printf("First Child Number of children: %i\n", c0->children_num);
			printf("------------------------\n");
			*/
			/*
			long result = number_of_nodes(a);
			printf("Number of nodes: %li\n", result);
			
			result = number_of_leaves(a);
			printf("Number of leaves: %li\n", result);
			
			result = number_of_branches(a);
			printf("Number of branches: %li\n", result);
			*/
			
			fval result = eval(r.output);			
			fval_println(result);			
			mpc_ast_delete(r.output);			
			
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		
		free(input);
	}
	
	mpc_cleanup(4, Number, Operator, Expr, Fuss);
	
	return 0;
}