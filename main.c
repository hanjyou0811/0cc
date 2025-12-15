#include "0cc.h"

char *user_input;
Token *token;
LVar *locals = NULL;
int lavel_id = 0;
const char *arg_addr[] = {
        "rdi", "rsi", "rdx", "rcx", "r8", "r9"
};

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
        for (int i=0;code[i];i++){
		if(code[i]->func_name) {
			if(strncmp(code[i]->func_name, "main", 4))
				println(".extern %s", code[i]->func_name);
		}
	}
	for (int i=0; code[i]; i++) {
		gen(code[i]);
	}

    //     println("	mov rsp, rbp");
	// println("	pop rbp");
	// println("	ret");
        return 0;
}
