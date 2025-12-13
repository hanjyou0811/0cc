#include "0cc.h"

char *user_input;
Token *token;
LVar *locals = NULL;
int lavel_id = 0;

int main(int argc, char **argv){
        if (argc != 2) {
                fprintf(stderr, "引数の個数が正しくありません。\n");
                return 1;
        }
	
        user_input = argv[1];
        
	tokenize(argv[1]);
	locals = NULL;
	program();

        println(".intel_syntax noprefix");
        println(".globl main");
        println("main:");
	
        println("	push rbp");
	println("	mov rbp, rsp");
	println("	sub rsp, 208");
	
	for (int i=0; code[i]; i++) {
		gen(code[i]);
		println("	pop rax");
	}

        println("	mov rsp, rbp");
	println("	pop rbp");
	println("	ret");
        return 0;
}
