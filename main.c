#include "0cc.h"

char *user_input;
char *filename;
Token *token;
LVar *locals = NULL;
GVar *globals = NULL;
int lavel_id = 0;
const char *arg_addr[] = {
        "rdi", "rsi", "rdx", "rcx", "r8", "r9"
};
extern_funcs *funcs = NULL;

int main(int argc, char **argv){
	if (argc != 2) {
		fprintf(stderr, "引数の個数が正しくありません。\n");
		return 1;
	}

	filename = argv[1];
	user_input = read_file(filename);
        
	tokenize(user_input);
	locals = NULL;
	program();
	
	println(".intel_syntax noprefix");
	println(".globl main");
	while(funcs){
		println(".extern %s", funcs->func_name);
		funcs = funcs->nex;
	}
	gen_gval();
	println(".text");
	for (int i=0; code[i]; i++) {
		gen(code[i]);
	}
     return 0;
}
