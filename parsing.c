#include "mpc.h"

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
		operator : '+' | '-' | '*' | '/' ;                  \
		expr     : <number> | '(' <operator> <expr>+ ')' ;  \
		fuss    : /^/ <operator> <expr>+ /$/ ;             \
	  ",
	  Number, Operator, Expr, Fuss);


	/* Do some parsing here... */

	mpc_cleanup(4, Number, Operator, Expr, Fuss);
}