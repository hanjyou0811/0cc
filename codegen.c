#include "0cc.h"

void gen(Node *node) {
        if (node->kind == ND_NUM) {
                printf("        push %d\n", node->val);
                return ;
        }
	if (node->kind == ND_LVAR) {
		fprintf(stderr, "kokomade\n");
		return; 
	}

        gen(node->lhs);
        gen(node->rhs);

        printf("        pop rdi\n");
        printf("        pop rax\n");

        switch (node->kind) {
        case ND_ADD:
                printf("        add rax, rdi\n");
                break;
        case ND_SUB:
                printf("        sub rax, rdi\n");
                break;
        case ND_MUL:
                printf("        imul rax, rdi\n");
                break;
        case ND_DIV:
                printf("        cqo\n");
                printf("        idiv rdi\n");
                break;
        case ND_EQL:
                printf("        cmp rax, rdi\n");
                printf("        sete al\n");
                printf("        movzb rax, al\n");
                break;
        case ND_NEQL:
                printf("        cmp rax, rdi\n");
                printf("        setne al\n");
                printf("        movzb rax, al\n");
                break;
        case ND_LST:
                printf("        cmp rax, rdi\n");
                printf("        setl al\n");
                printf("        movzb rax, al\n");
                break;
        case ND_LSE:
                printf("        cmp rax, rdi\n");
                printf("        setle al\n");
                printf("        movzb rax, al\n");
                break;
	default:
		fprintf(stderr, "kokomade\n");
		exit(1);
        }
        printf("        push rax\n");
}
