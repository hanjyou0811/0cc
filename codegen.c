#include "0cc.h"

void gen_lval(Node *node) {
	if (node->kind != ND_LVAR) error("代入の左辺値が変数ではありません");
	
	println("	mov rax, rbp");
	println("	sub rax, %d", node->offset);
	println("	push rax");
}

void gen(Node *node) {
	switch (node->kind) {
	case ND_NUM:
		println("	push %d", node->val);
		return ;
	case ND_LVAR:
		gen_lval(node);
		println("	pop rax");
		println("	mov rax, [rax]");
		println("	push rax");
		return ;
	case ND_ASSIGN:
		gen_lval(node->lhs);
		gen(node->rhs);
		
		println("	pop rdi");
		println("	pop rax");
		println("	mov [rax], rdi");
		println("	push rdi");
		return ;
	}
	gen(node->lhs);
	gen(node->rhs);

        println("        pop rdi\n");
        println("        pop rax\n");

        switch (node->kind) {
        case ND_ADD:
                println("        add rax, rdi");
                break;
        case ND_SUB:
                println("        sub rax, rdi");
                break;
        case ND_MUL:
                println("        imul rax, rdi");
                break;
        case ND_DIV:
                println("        cqo");
                println("        idiv rdi");
                break;
        case ND_EQL:
                println("        cmp rax, rdi");
		println("        sete al");
                println("        movzb rax, al");
                break;
        case ND_NEQL:
                println("        cmp rax, rdi");
                println("        setne al");
                println("        movzb rax, al");
                break;
        case ND_LST:
                println("        cmp rax, rdi");
                println("        setl al");
                println("        movzb rax, al");
                break;
        case ND_LSE:
                println("        cmp rax, rdi");
                println("        setle al");
                println("        movzb rax, al");
        }
        println("        push rax");
}
