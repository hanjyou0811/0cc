#include "0cc.h"

char *user_input;
Token *token;

int main(int argc, char **argv){
        if (argc != 2) {
                fprintf(stderr, "引数の個数が正しくありません。\n");
                return 1;
        }

        user_input = argv[1];
        token = tokenize(argv[1]);
        program();

        printf(".intel_syntax noprefix\n");
        printf(".globl main\n");
        printf("main:\n");

        gen(code[0]);

        printf("        pop rax\n");
        printf("        ret\n");
        return 0;
}
