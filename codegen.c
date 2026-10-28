#include "0cc.h"

void gen_lval(Node *node) {
	switch (node->kind) {
	case ND_LVAR:
		// if(node->tp->kind == ARRAY) return ;
		println("	mov rax, rbp");
		println("	sub rax, %d", node->offset);
		println("	push rax");
		return ;
	case ND_GVAR:
		println("	lea rax, [rip + %s]", node->gvar_name);
		println("	push rax");
		return ;
	case ND_DEREF:
		gen(node->lhs);
		return ;
	case ND_STR:
		gen(node);
		return ;
	}
	error("代入の左辺値が変数でもポインタでもありません。");
}

void gen(Node *node) {
	if(!node) return ;
	switch (node->kind) {
	case ND_NUM:
		println("	push %d", node->val);
		return ;
	case ND_LVAR:
		gen_lval(node);
		if(node->tp && node->tp->kind == ARRAY) return ;
		println("	pop rax");
		if(node->tp && node->tp->kind == CHAR) println("	movsx rax, BYTE PTR [rax]");
		else if(node->tp && node->tp->kind == INT) println("	movsxd rax, DWORD PTR [rax]");
		else println("	mov rax, [rax]");
		println("	push rax");
		return ;
	case ND_ARRAY_INIT:
		gen_lval(node->lhs);
		println("	pop rax");
		for (size_t i = 0; i < node->tp->array_size; i++) {
			int byte = i < node->rhs->val ? (unsigned char)node->rhs->str[i] : 0;
			println("	mov BYTE PTR [rax + %zu], %d", i, byte);
		}
		return ;
	case ND_ASSIGN:
		gen_lval(node->lhs);
		gen(node->rhs);
		println("	pop rdi");
		println("	pop rax");
		if(node->tp && node->tp->kind == CHAR) println("	mov BYTE PTR [rax], dil");
		else if(node->tp && node->tp->kind == INT) {
			println("	mov DWORD PTR [rax], edi");
			println("	movsxd rdi, edi");
		}
		else println("	mov [rax], rdi");
		println("	push rdi");
		return ;
	case ND_RETURN:
		gen(node->lhs);
		println("	pop rax");
		println("	mov rsp, rbp");
		println("	pop rbp");
		println("	ret");
		return ;
	case ND_IF:
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
	case ND_WHILE:
		id = lavel_id++;
		println(".Lbegin%03d:", id);
		gen(node->cond);
		println("	pop rax");
		println("	cmp rax, 0");
		println("	je .Lend%03d", id);
		gen(node->then);
		println("	jmp .Lbegin%03d", id);
		println(".Lend%03d:", id);
		return ;
	case ND_FOR:
		id = lavel_id++;
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
	case ND_BLOCK:
		int j = 0;
		for(int i=0;node->stmts[i];i++) {
			j++;
		}
		for(int i=0;i < j-1;i++) {
			gen(node->stmts[i]);
			if(has_value(node->stmts[i]))
				println("	pop rax");
		}
		gen(node->stmts[j-1]);
		return ;
	case ND_CALL:
		int argc = 0;
		while(node->call_args[argc]) argc++;
		if (argc > 6) error("引数が6こ以上です。");
		for(int i = argc - 1; i >= 0; i--) {
			gen(node->call_args[i]);
		}
		for(int i = 0;i < argc;i++){
			println("	pop %s", arg_addr[i]);
		}
		println("	call %s", node->func_name);
		println("	push rax");
		// println("	ret");
		return ;
	case ND_FUNC:
		println("%s:", node->func_name);
		println("	push rbp");
		println("	mov rbp, rsp");
		println("	sub rsp, %d", 208);
		for (int i = 0; i < node->argc; i++) {
			const char *int_args[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
			if (node->params[i]->tp->kind == INT)
				println("	mov DWORD PTR [rbp-%d], %s",
					node->params[i]->offset, int_args[i]);
			else
				println("	mov [rbp-%d], %s",
					node->params[i]->offset, arg_addr[i]);
		}
		gen(node->body);
		if (strcmp(node->func_name, "main") == 0) {
			println("	mov eax, 0");
		}
		println("	mov rsp, rbp");
		println("	pop rbp");
		println("	ret");
		return;
	case ND_ADDR:
		gen_lval(node->lhs);
		return ;
	case ND_DEREF:
		gen(node->lhs);
		if (node->tp && node->tp->kind == ARRAY) return ;
		println("	pop rax");
		if(node->tp && node->tp->kind == CHAR) println("	movsx rax, BYTE PTR [rax]");
		else if(node->tp && node->tp->kind == INT) println("	movsxd rax, DWORD PTR [rax]");
		else println("	mov rax, [rax]");
		println("	push rax");
		return;
	case ND_DECL:
		return ;
	case ND_SIZE:
		println("	push %d", size_of(node->lhs->tp));
		return ;
	case ND_GVAR:
		if(node->tp && node->tp->kind == ARRAY) return ;	
		gen_lval(node);
		println("	pop rax");
		if(node->tp && node->tp->kind == CHAR) println("	movsx rax, BYTE PTR [rax]");
		else if(node->tp && node->tp->kind == INT) println("	movsxd rax, DWORD PTR [rax]");
		else println("	mov rax, [rax]");
		println("	push rax");
		return ;
	case ND_STR: {
		int string_id = lavel_id++;
		println(".pushsection .rodata");
		println(".Lstr%03d:", string_id);
		for (int i = 0; i < node->val; i++)
			println("	.byte %u", (unsigned char)node->str[i]);
		println(".popsection");
		println("	lea rax, [rip + .Lstr%03d]", string_id);
		println("	push rax");
		return ;
	}
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

void gen_gval()
{
	println(".data");
	GVar *gvar = globals;
	while(gvar)
	{
		println("%s:", gvar->name);
		if(gvar->val)
			println("	.long %d", gvar->val);
		else
			println("	.zero");
		gvar = gvar->next;
	}
}
