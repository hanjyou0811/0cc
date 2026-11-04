#include "0cc.h"

static int loop_id = -1;

static void gen_stmt(Node *node) {
	if (!node) return;
	gen(node);
	if (has_value(node)) println("\tpop rax");
}

void gen_lval(Node *node) {
	switch (node->kind) {
	case ND_LVAR:
		println("	mov rax, rbp");
		println("	sub rax, %d", node->offset);
		println("	push rax");
		return ;
	case ND_GVAR:
		println("	lea rax, [rip + %s]", node->gvar_name);
		println("	push rax");
		return ;
	case ND_MEMBER:
		gen(node->lhs);
		println("\tpop rax");
		println("\tadd rax, %d", node->offset);
		println("\tpush rax");
		return;
	case ND_DEREF:
		gen(node->lhs);
		return ;
	case ND_STR:
		gen(node);
		return ;
	}
	error("代入の左辺値が変数でもポインタでもありません。");
}

static void gen_update(Node *node) {
	Type *tp = node->lhs->tp;
	bool postfix = node->kind == ND_POST_INC || node->kind == ND_POST_DEC;
	gen_lval(node->lhs);
	// Keep the destination and its old value across evaluation of the RHS.
	println("\tmov rax, [rsp]");
	if (tp->kind == CHAR) println("\tmovsx rax, BYTE PTR [rax]");
	else if (tp->kind == INT) println("\tmovsxd rax, DWORD PTR [rax]");
	else println("\tmov rax, [rax]");
	println("\tpush rax");
	if (node->rhs) gen(node->rhs);
	else println("\tpush 1");
	println("\tpop rdi");
	println("\tpop rax");
	println("\tpop rcx");
	if (postfix) println("\tmov r8, rax");
	if (tp->kind == PTR && size_of(tp->ptr_to) != 1)
		println("\timul rdi, %d", size_of(tp->ptr_to));
	switch (node->kind) {
	case ND_ADD_ASSIGN: case ND_PRE_INC: case ND_POST_INC:
		println("\tadd rax, rdi");
		break;
	case ND_SUB_ASSIGN: case ND_PRE_DEC: case ND_POST_DEC:
		println("\tsub rax, rdi");
		break;
	case ND_MUL_ASSIGN:
		println("\timul rax, rdi");
		break;
	case ND_DIV_ASSIGN:
		println("\tcqo");
		println("\tidiv rdi");
		break;
	default:
		error("不明な更新演算子です。");
	}
	if (tp->kind == CHAR) {
		println("\tmov BYTE PTR [rcx], al");
		println("\tmovsx rax, al");
	} else if (tp->kind == INT) {
		println("\tmov DWORD PTR [rcx], eax");
		println("\tmovsxd rax, eax");
	} else println("\tmov [rcx], rax");
	println("\tpush %s", postfix ? "r8" : "rax");
}

void gen(Node *node) {
	if(!node) return ;
	switch (node->kind) {
	case ND_NOP:
		return;
	case ND_NOT:
		gen(node->lhs);
		println("\tpop rax");
		println("\tcmp rax, 0");
		println("\tsete al");
		println("\tmovzx rax, al");
		println("\tpush rax");
		return;
	case ND_LOGAND:
	case ND_LOGOR: {
		int logical_id = lavel_id++;
		gen(node->lhs);
		println("\tpop rax");
		println("\tcmp rax, 0");
		println("\t%s .Llogical_short%03d",
			node->kind == ND_LOGAND ? "je" : "jne", logical_id);
		gen(node->rhs);
		println("\tpop rax");
		println("\tcmp rax, 0");
		println("\tsetne al");
		println("\tmovzx rax, al");
		println("\tjmp .Llogical_end%03d", logical_id);
		println(".Llogical_short%03d:", logical_id);
		println("\tmov rax, %d", node->kind == ND_LOGOR);
		println(".Llogical_end%03d:", logical_id);
		println("\tpush rax");
		return;
	}
	case ND_NUM:
		println("	push %d", node->val);
		return ;
	case ND_COMMA:
		gen(node->lhs);
		println("\tpop rax");
		gen(node->rhs);
		return ;
	case ND_MEMBER:
	case ND_LVAR:
		gen_lval(node);
		if(node->tp && (node->tp->kind == ARRAY || node->tp->kind == STRUCT)) return ;
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
		if (node->tp && node->tp->kind == STRUCT) {
			for (int i = 0; i < size_of(node->tp); i++) {
				println("\tmov cl, BYTE PTR [rdi + %d]", i);
				println("\tmov BYTE PTR [rax + %d], cl", i);
			}
			println("\tpush rax");
			return;
		}
		if(node->tp && node->tp->kind == CHAR) println("	mov BYTE PTR [rax], dil");
		else if(node->tp && node->tp->kind == INT) {
			println("	mov DWORD PTR [rax], edi");
			println("	movsxd rdi, edi");
		}
		else println("	mov [rax], rdi");
		println("	push rdi");
		return ;
	case ND_ADD_ASSIGN:
	case ND_SUB_ASSIGN:
	case ND_MUL_ASSIGN:
	case ND_DIV_ASSIGN:
	case ND_PRE_INC:
	case ND_PRE_DEC:
	case ND_POST_INC:
	case ND_POST_DEC:
		gen_update(node);
		return ;
	case ND_BREAK:
		println("\tjmp .Lend%03d", loop_id);
		return;
	case ND_CONTINUE:
		println("\tjmp .Lcontinue%03d", loop_id);
		return;
	case ND_DEF:
		gen_stmt(node->lhs);
		gen_stmt(node->rhs);
		return;
	case ND_RETURN:
		if (node->lhs) {
			gen(node->lhs);
			println("\tpop rax");
		}
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
		gen_stmt(node->then);
		if(node->els)
		{
			println("	jmp .Lend%03d", id);
			println(".Lelse%03d:", id);
			gen_stmt(node->els);
		}
		println(".Lend%03d:", id);
		return ;
	case ND_WHILE: {
		int id = lavel_id++;
		int outer_loop = loop_id;
		loop_id = id;
		println(".Lbegin%03d:", id);
		println(".Lcontinue%03d:", id);
		gen(node->cond);
		println("	pop rax");
		println("	cmp rax, 0");
		println("	je .Lend%03d", id);
		gen_stmt(node->then);
		println("	jmp .Lbegin%03d", id);
		println(".Lend%03d:", id);
		loop_id = outer_loop;
		return ;
	}
	case ND_FOR: {
		int id = lavel_id++;
		int outer_loop = loop_id;
		loop_id = id;
		gen_stmt(node->init);
		println(".Lbegin%03d:", id);
		if (node->cond) {
			gen(node->cond);
			println("	pop rax");
			println("	cmp rax, 0");
			println("	je .Lend%03d", id);
		}
		gen_stmt(node->body);
		println(".Lcontinue%03d:", id);
		gen_stmt(node->inc);
		println("	jmp .Lbegin%03d", id);
		println(".Lend%03d:", id);
		loop_id = outer_loop;
		return ;
	}
	case ND_BLOCK:
		for (int i = 0; node->stmts[i]; i++)
			gen_stmt(node->stmts[i]);
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
		if (node->tp && node->tp->kind == CHAR) println("\tmovsx rax, al");
		else if (node->tp && node->tp->kind == INT) println("\tmovsxd rax, eax");
		println("	push rax");
		// println("	ret");
		return ;
	case ND_FUNC:
		println("%s:", node->func_name);
		println("	push rbp");
		println("	mov rbp, rsp");
		if (node->stack_size)
			println("\tsub rsp, %d", node->stack_size);
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
		if (node->tp && (node->tp->kind == ARRAY || node->tp->kind == STRUCT)) return ;
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
		gen_lval(node);
		if(node->tp && (node->tp->kind == ARRAY || node->tp->kind == STRUCT)) return;
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
	for (GVar *gvar = globals; gvar; gvar = gvar->next)
		gen_global_variable(gvar);
	gen_initializer_strings();
}
