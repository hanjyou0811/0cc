#include "0cc.h"

void gen_lval(Node *node) {
	if (node->kind != ND_LVAR) error("代入の左辺値が変数ではありません");
	
	println("	mov rax, rbp");
	println("	sub rax, %d", node->offset);
	println("	push rax");
}

void gen(Node *node) {
	if(!node) return ;
	if(node->kind == ND_RETURN) {
		gen(node->lhs);
		println("	pop rax");
		println("	mov rsp, rbp");
		println("	pop rbp");
		println("	ret");
		return ;
	}
	if(node->kind == ND_IF) {
		int id = lavel_id++;
		gen(node->cond);
		println("	pop rax");
		println("	cmp rax, 0");
		if(node->els) {
			println("	je .Lelse%03d", id);
		} else 
			println("	je .Lend%03d", id);
		gen(node->then);
		if(node->els)
		{
			println("	jmp .Lend%03d", id);
			println(".Lelse%03d:", id);
			gen(node->els);
		}
		println(".Lend%03d:", id);
		return ;
	}
	if(node->kind == ND_WHILE) {
		int id = lavel_id++;
		println(".Lbegin%03d:", id);
		gen(node->cond);
		println("	pop rax");
		println("	cmp rax, 0");
		println("	je .Lend%03d", id);
		gen(node->then);
		println("	jmp .Lbegin%03d", id);
		println(".Lend%03d:", id);
		return ;
	}
	if(node->kind == ND_FOR) {
		int id = lavel_id++;
		gen(node->init);
		println(".Lbegin%03d:", id);
		gen(node->cond);
		println("	pop rax");
		println("	cmp rax, 0");
		println("	je .Lend%03d", id);
		gen(node->body);
		gen(node->inc);
		println("	jmp .Lbegin%03d", id);
		println(".Lend%03d:", id);
		return ;
	}
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

        println("        pop rdi");
        println("        pop rax");

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
