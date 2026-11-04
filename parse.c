#include "0cc.h"
#include <limits.h>

Node *code[100];
static int loop_depth;
static Type *return_type;
static Node *current_function;

Scope *scope;
static bool function_body;
void enter_scope(void) { Scope *s = calloc(1, sizeof(Scope)); s->next = scope; scope = s; }
void leave_scope(void) { scope = scope->next; }
static bool same_name(Token *a, Token *b) { return a->len == b->len && !memcmp(a->str, b->str, a->len); }
VarScope *find_name(Token *tok) {
    for (Scope *s = scope; s; s = s->next)
        for (VarScope *v = s->vars; v; v = v->next)
            if (same_name(v->name, tok)) return v;
    return NULL;
}
VarScope *declare_name(Token *tok) {
    for (VarScope *v = scope->vars; v; v = v->next)
        if (same_name(v->name, tok)) error_at(tok->str, "duplicate declaration");
    VarScope *v = calloc(1, sizeof(VarScope)); v->name = tok; v->next = scope->vars; scope->vars = v; return v;
}
bool is_typename(Token *tok) {
    if (tok->kind == TK_TYPE) return true;
    VarScope *v = tok->kind == TK_IDENT ? find_name(tok) : NULL;
    return v && v->typedef_type;
}
static Node *typedef_decl(void) {
    Type *base = declspec();
    do {
        Token *name = NULL; Type *tp = declarator(base, &name);
        VarScope *existing = NULL;
        for (VarScope *v = scope->vars; v; v = v->next)
            if (same_name(v->name, name)) { existing = v; break; }
        if (existing) {
            if (!existing->typedef_type || !compatible_type(existing->typedef_type, tp))
                error_at(name->str, "incompatible typedef redeclaration");
        } else declare_name(name)->typedef_type = tp;
    } while (consume(","));
    expect(';'); return new_node(ND_NOP, NULL, NULL);
}

static int local_offset(Type *tp, int previous, Token *tok) {
	if (tp->kind == ARRAY && tp->array_size && tp->array_size > INT_MAX / size_of(tp->ptr_to))
		error_at(tok->str, "ローカル変数のサイズが大きすぎます。");
	int size = tp->kind == ARRAY && !tp->array_size ? 8 : MAX(size_of(tp), 8);
	if (size > INT_MAX - 15 - previous)
		error_at(tok->str, "関数のスタックサイズが大きすぎます。");
	int offset = previous + size;
	current_function->stack_size = MAX(current_function->stack_size, (offset + 15) / 16 * 16);
	return offset;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs){
	Node *node      = calloc(1, sizeof(Node));
	node->kind      = kind;
	node->lhs       = lhs;
	node->rhs       = rhs;
	if (kind == ND_MUL || kind == ND_DIV || kind == ND_EQL ||
	    kind == ND_NEQL || kind == ND_LST || kind == ND_LSE)
		node->tp = new_type(INT, NULL);
	return node;
}

Node *new_node_num(int val) {
	Node *node      = calloc(1, sizeof(Node));
	node->kind      = ND_NUM;
	node->val       = val;
	node->tp 		= new_type(INT, NULL);
	return node;
}

Node *new_node_if(Node* cond, Node *then, Node *els) 
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_IF;
	node->cond = cond;
	node->then = then;
	node->els = els;
	return node;
}

Node *new_node_while(Node* cond, Node *body) 
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_WHILE;
	node->cond = cond;
	node->then = body;
	return node;
}

Type *new_type(Typename typename, Type *next)
{
	Type *tp = calloc(1, sizeof(Type));
	tp->kind = typename;
	if(next) tp->ptr_to = next;
	return tp;
}

int eval_const_expr(Node *node) {
    switch (node->kind) {
    case ND_NUM:
        return node->val;
    case ND_ADD:
        return eval_const_expr(node->lhs)
             + eval_const_expr(node->rhs);
    case ND_SUB:
        return eval_const_expr(node->lhs)
             - eval_const_expr(node->rhs);
    case ND_MUL:
        return eval_const_expr(node->lhs)
             * eval_const_expr(node->rhs);
    case ND_DIV:
        return eval_const_expr(node->lhs)
             / eval_const_expr(node->rhs);
    default:
        error_at(token->str, "配列サイズは定数の式である必要があります");
    }
}

void program() {
	int i = 0;
    enter_scope();
	while(!at_eof()) {
		if (consume("typedef")) { typedef_decl(); continue; }
		if(is_function()) {
			code[i++] = func();
		}else{
			global_decl();
		}
	}
	code[i] = NULL;
}

Node *func() {
	locals = NULL;
	Node *node = calloc(1, sizeof(Node));
	current_function = node;
    Type *ty = declspec();
    Token *tok;
	tok = NULL;
	ty = declarator(ty, &tok);
	if(!tok || tok->kind != TK_IDENT) error_at(token->str, "識別子が来るべきです。");
	declare_name(tok);
    enter_scope();
	node->kind = ND_FUNC;
	node->func_name = strndup(tok->str, tok->len);
	node->tp = ty;
	if(!consume("(")) 
		error_at(token->str, "'('が来るべきです。");
	if(!consume(")")) 
		node->argc = consume_paramList(node, ty);
	return_type = ty;
	loop_depth = 0;
	function_body = true;
	node->body = block();
    leave_scope();
	return_type = NULL;
	return node;
}

Node *block(){
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;
	LVar *saved = locals;
    bool own_scope = !function_body; function_body = false;
    if (own_scope) enter_scope();
	if(!consume("{")) error_at(token->str, "'{'が必要です。");
    int i = 0;
    while (!consume("}")) {
        node->stmts[i++] = stmt();
    }
    node->stmts[i] = NULL;
	locals = saved;
    if (own_scope) leave_scope();
	return node;
}

Node *stmt() {
	Node *node;
	if (consume("typedef")) return typedef_decl();
	if (consume(";")) return new_node(ND_NOP, NULL, NULL);
	Token *start = token;
	if (consume("break")) {
		if (!loop_depth) error_at(start->str, "breakはループ内でのみ使えます。");
		expect(';');
		return new_node(ND_BREAK, NULL, NULL);
	}
	if (consume("continue")) {
		if (!loop_depth) error_at(start->str, "continueはループ内でのみ使えます。");
		expect(';');
		return new_node(ND_CONTINUE, NULL, NULL);
	}
	if (consume("return")) {
		Node *value = NULL;
		if (consume(";")) {
			if (return_type->kind != VOID)
				error_at(start->str, "戻り値が必要です。");
		} else {
			value = expr();
			expect(';');
			if (return_type->kind == VOID)
				error_at(start->str, "void関数では値を返せません。");
		}
		return new_node(ND_RETURN, value, NULL);
	}
	
	if (consume("if")) {
		if (!consume("(")) error_at(token->str, "'('ではじめてください");
		Node *cond = expr();
		if (!consume(")")) error_at(token->str, "')'でとじてください");
		Node *then = stmt();
		Node *els = NULL;
		if (consume("else")) {
			els = stmt();
		}
		node = new_node_if(cond, then, els);
		return node;
	}
	if (consume("while")) {
		if (!consume("(")) error_at(token->str, "'('ではじめてください");
		Node *cond = expr();
		if (!consume(")")) error_at(token->str, "')'でとじてください");
		loop_depth++;
		Node *then = stmt();
		loop_depth--;
		node = new_node_while(cond, then);
		return node;
	}
	if (consume("for")) {
        LVar *saved_locals = locals; enter_scope();
		node = new_node(ND_FOR, NULL, NULL);
		if (!consume("(")) error_at(token->str, "'('ではじめてください");
		// init
		if (!consume(";")) {
			if (!is_typename(token)) node->init = expr();
			else {
				Type *ty = declspec();
				node->init = def_stmt(ty);
			}
			if(!consume(";")) error_at(token->str, "初期化式を書いてください");
		}
		// cond
		if (!consume(";")) {
			node->cond = expr();
			if(!consume(";")) error_at(token->str, "条件式を書いてください");
		}
		// inc
		if (!consume(")")) {
			node->inc = expr();
			if(!consume(")")) error_at(token->str, "')'でとじてください");
		}
		loop_depth++;
		node->body = stmt();
		loop_depth--;
        locals = saved_locals; leave_scope();
		return node;
	}
	if (token->str[0] == '{') {
		return block();
	}
	if (is_typename(token)) {
		Type *ty = declspec();
		node = def_stmt(ty);
	}
	else node = expr();
	if (!consume(";")) error_at(token->str, "';'で終わっていないです");
	return node;
}

Node *def_stmt(Type *tp){
	Node *node = decl(tp);
	while(consume(",")){
		node = new_node(ND_DEF, node, decl(tp));
	}
	return node;
}

void global_decl()
{
    Type *ty = declspec();
	decl_global(ty);
	expect(';');
}

void decl_global(Type *ty)
{
	Token *tok = NULL;
	ty = declarator(ty, &tok);
    if (ty->kind == VOID) error_at(tok->str, "void object");
	GVar *gvar = calloc(1, sizeof(GVar));
	declare_name(tok);
	gvar->name = strndup(tok->str, tok->len);
	gvar->len  = tok->len;
	gvar->tp   = ty;
	if(consume("=")) {
		Node *node = expr();
		gvar->val = eval_const_expr(node);
	}
	gvar->next = globals;
	globals = gvar;
}

Type *_typename(Token *tok) {
    if (tok->len == 3 && !memcmp(tok->str, "int", 3)) return new_type(INT, NULL);
    if (tok->len == 4 && !memcmp(tok->str, "char", 4)) return new_type(CHAR, NULL);
    if (tok->len == 4 && !memcmp(tok->str, "void", 4)) return new_type(VOID, NULL);
    error_at(tok->str, "type required"); return NULL;
}
Type *type(Token *tok) { return _typename(tok); }
Type *declspec(void) {
    Token *tok = token;
    if (tok->kind == TK_TYPE) { token = token->next; return _typename(tok); }
    VarScope *v = find_name(tok);
    if (!v || !v->typedef_type) error_at(tok->str, "type required");
    token = token->next; return v->typedef_type;
}
static Type *type_suffix(Type *base) {
    if (!consume("[")) return base;
    int len = 0;
    if (!consume("]")) {
        len = eval_const_expr(assign()); expect(']');
        if (len <= 0) error_at(token->str, "array size must be positive");
    }
    Type *element = type_suffix(base);
    if (element->kind == VOID || (element->kind == ARRAY && !element->array_size))
        error_at(token->str, "array element must be complete");
    Type *tp = new_type(ARRAY, element); tp->array_size = len; return tp;
}
static Type *parse_declarator(Type *base, Token **name, bool abstract) {
    while (consume("*")) base = new_type(PTR, base);
    if (consume("(")) {
        Token *start = token;
        Type dummy = {0};
        parse_declarator(&dummy, name, abstract); expect(')');
        base = type_suffix(base);
        Token *end = token;
        token = start;
        Type *result = parse_declarator(base, name, abstract); expect(')');
        token = end; return result;
    }
    if (token->kind == TK_IDENT) *name = consume_ident();
    else if (!abstract) error_at(token->str, "identifier required");
    return type_suffix(base);
}
Type *declarator(Type *base, Token **name) { return parse_declarator(base, name, false); }
Type *type_name(void) {
    Token *name = NULL; Type *tp = parse_declarator(declspec(), &name, true);
    if (name) error_at(name->str, "type name cannot contain an identifier");
    return tp;
}

Node *decl(Type *tp){
    Token *tok = NULL;
    tp = declarator(tp, &tok);
    if (tp->kind == VOID) error_at(tok->str, "void object");
    bool infer_size = tp->kind == ARRAY && tp->array_size == 0;
    if (infer_size) { Type *copy = calloc(1, sizeof(Type)); *copy = *tp; tp = copy; }
	VarScope *binding = declare_name(tok);
	LVar *lvar = calloc(1, sizeof(LVar));
	lvar->name = tok->str;
	lvar->len = tok->len;
	lvar->tp = tp;
    binding->var = lvar;
	lvar->offset = local_offset(tp, locals ? locals->offset : 0, tok);
	lvar->next = locals;
	locals = lvar;

	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_DECL;
	node->offset = lvar->offset;
	node->tp = tp;
	if(consume("=")){
		node->kind = ND_LVAR;
		Node *rhs = assign();
		if (tp->kind == ARRAY) {
			if (tp->ptr_to->kind != CHAR || rhs->kind != ND_STR)
				error_at(tok->str, "配列の初期化には文字配列と文字列が必要です。");
			if (infer_size) {
				tp->array_size = rhs->val;
				lvar->offset = local_offset(tp, lvar->next ? lvar->next->offset : 0, tok);
				node->offset = lvar->offset;
			}
			// C permits an exact fit without the terminating NUL.
			if (rhs->val - 1 > tp->array_size)
				error_at(tok->str, "文字列が配列の初期化サイズを超えています。");
			node = new_node(ND_ARRAY_INIT, node, rhs);
		} else {
			node = new_node(ND_ASSIGN, node, rhs);
		}
		node->tp = tp;
	} else if (infer_size) {
		error_at(tok->str, "配列サイズの省略には初期化が必要です。");
	}
	return node;
}

Node *expr() {
	Node *node = assign();
	while (consume(",")) {
		node = new_node(ND_COMMA, node, assign());
		node->tp = node->rhs->tp;
		if (node->tp && node->tp->kind == ARRAY)
			node->tp = new_type(PTR, node->tp->ptr_to);
	}
    return node;
}

static Node *new_update(NodeKind kind, Node *lhs, Node *rhs, Token *op) {
	if (!lhs->tp || (lhs->kind != ND_LVAR && lhs->kind != ND_GVAR && lhs->kind != ND_DEREF) ||
	    (lhs->tp->kind != INT && lhs->tp->kind != CHAR && lhs->tp->kind != PTR))
		error_at(op->str, "左辺には書き換え可能な整数かポインタが必要です。");
	if (lhs->tp->kind == PTR) {
		if (kind == ND_MUL_ASSIGN || kind == ND_DIV_ASSIGN)
			error_at(op->str, "ポインタには乗算・除算の複合代入を使えません。");
		if (lhs->tp->ptr_to->kind == VOID)
			error_at(op->str, "voidポインタの要素サイズは不明です。");
	}
	if (rhs && (!rhs->tp || (rhs->tp->kind != INT && rhs->tp->kind != CHAR)))
		error_at(op->str, "複合代入の右辺には整数が必要です。");
	Node *node = new_node(kind, lhs, rhs);
	node->tp = lhs->tp;
	return node;
}

Node *assign() {
	Node *node = logical_or();
	if(consume("=")) {
		if (node->tp && node->tp->kind == ARRAY)
			error_at(token->str, "配列には代入できません。");
		node = new_node(ND_ASSIGN, node, assign());
		node->tp = node->lhs->tp;
	}
	else {
		Token *op = token;
		if (consume("+=")) return new_update(ND_ADD_ASSIGN, node, assign(), op);
		if (consume("-=")) return new_update(ND_SUB_ASSIGN, node, assign(), op);
		if (consume("*=")) return new_update(ND_MUL_ASSIGN, node, assign(), op);
		if (consume("/=")) return new_update(ND_DIV_ASSIGN, node, assign(), op);
	}
	return node;
}

Node *logical_or() {
	Node *node = logical_and();
	while (consume("||")) {
		node = new_node(ND_LOGOR, node, logical_and());
		node->tp = new_type(INT, NULL);
	}
	return node;
}

Node *logical_and() {
	Node *node = equality();
	while (consume("&&")) {
		node = new_node(ND_LOGAND, node, equality());
		node->tp = new_type(INT, NULL);
	}
	return node;
}

Node *equality() {
        Node *node = relational();

        for(;;) {
                if(consume("==")) node = new_node(ND_EQL, node, relational());
                else if(consume("!=")) node = new_node(ND_NEQL, node, relational());
                else return node;
        }
}

Node *relational() {
        Node *node = add();

        for(;;) {
			if(consume("<")) node = new_node(ND_LST, node, add());
			else if(consume("<=")) node = new_node(ND_LSE, node, add());
			else if(consume(">")) node = new_node(ND_LST, add(), node);
			else if(consume(">=")) node = new_node(ND_LSE, add(), node);
			else return node;
        }
}

static bool integer_type(Type *tp) { return tp && (tp->kind == INT || tp->kind == CHAR); }

Type *decay_type(Type *tp) {
    return tp && tp->kind == ARRAY ? new_type(PTR, tp->ptr_to) : tp;
}

static void pointer_element(Type *tp) {
    if (!tp->ptr_to || tp->ptr_to->kind == VOID || size_of(tp->ptr_to) <= 0)
        error_at(token->str, "pointer arithmetic requires a complete object type");
}

bool compatible_type(Type *a, Type *b) {
    if (!a || !b || a->kind != b->kind) return false;
    if (a->kind == PTR) return compatible_type(a->ptr_to, b->ptr_to);
    if (a->kind == ARRAY) return a->array_size == b->array_size && compatible_type(a->ptr_to, b->ptr_to);
    return true;
}

Node *new_add(Node *lhs, Node *rhs) {
    Type *lt = decay_type(lhs->tp), *rt = decay_type(rhs->tp);
    if (integer_type(lt) && rt && rt->kind == PTR) {
        Node *tmp = lhs; lhs = rhs; rhs = tmp;
        Type *tt = lt; lt = rt; rt = tt;
    }
    Node *node;
    if (lt && lt->kind == PTR && integer_type(rt)) {
        pointer_element(lt);
        node = new_node(ND_ADD, lhs, new_node(ND_MUL, rhs, new_node_num(size_of(lt->ptr_to))));
        node->tp = lt;
    } else {
        if (!integer_type(lt) || !integer_type(rt)) error_at(token->str, "invalid operands to addition");
        node = new_node(ND_ADD, lhs, rhs); node->tp = new_type(INT, NULL);
    }
    return node;
}

Node *new_sub(Node *lhs, Node *rhs) {
    Type *lt = decay_type(lhs->tp), *rt = decay_type(rhs->tp);
    if (lt && lt->kind == PTR) {
        pointer_element(lt);
        if (integer_type(rt)) {
            Node *node = new_node(ND_SUB, lhs, new_node(ND_MUL, rhs, new_node_num(size_of(lt->ptr_to))));
            node->tp = lt; return node;
        }
        if (rt && rt->kind == PTR && compatible_type(lt, rt)) {
            Node *diff = new_node(ND_SUB, lhs, rhs); diff->tp = new_type(INT, NULL);
            return new_node(ND_DIV, diff, new_node_num(size_of(lt->ptr_to)));
        }
    }
    if (!integer_type(lt) || !integer_type(rt)) error_at(token->str, "invalid operands to subtraction");
    Node *node = new_node(ND_SUB, lhs, rhs); node->tp = new_type(INT, NULL); return node;
}

Node *add() {
        Node *node = mul();

        for(;;) {
			if(consume("+")) node = new_add(node, mul());
			else if(consume("-")) node = new_sub(node, mul());
			else return node;
        }
}

Node *mul() {
        Node *node = unary();

        for(;;) {
			if(consume("*")) node = new_node(ND_MUL, node, unary());
			else if(consume("/")) node = new_node(ND_DIV, node, unary());
			else return node;
        }
}

Node *unary() {
	Token *op = token;
	if (consume("++")) return new_update(ND_PRE_INC, unary(), NULL, op);
	if (consume("--")) return new_update(ND_PRE_DEC, unary(), NULL, op);
	if (consume("!")) {
		Node *node = new_node(ND_NOT, unary(), NULL);
		node->tp = new_type(INT, NULL);
		return node;
	}
	if (consume("+")) {
		Node *operand = unary();
		if (!operand->tp || (operand->tp->kind != INT && operand->tp->kind != CHAR))
			error_at(op->str, "単項+には整数が必要です。");
		Node *node = new_node(ND_ADD, new_node_num(0), operand);
		node->tp = new_type(INT, NULL);
		return node;
	}
	if(consume("-")) {
		Node *node = new_node(ND_SUB, new_node_num(0), unary());
		node->tp = new_type(INT, NULL);
		return node;
	}
	if(consume("*")) 
	{
		Node *lhs = unary();
		Node *node = new_node(ND_DEREF, lhs, NULL);
		if (!lhs->tp || (lhs->tp->kind != PTR && lhs->tp->kind != ARRAY) || lhs->tp->ptr_to->kind == VOID)
            error_at(op->str, "dereference requires object pointer");
		node->tp = lhs->tp->ptr_to;
		return node;
	}
	if(consume("&")) {
		Node *node = new_node(ND_ADDR, unary(), NULL);
		node->tp = new_type(PTR, node->lhs->tp);
		return node;
	}
	if(consume("sizeof")) {
        if (token->kind == TK_RESERVED && token->len == 1 && token->str[0] == '(' && is_typename(token->next)) {
            expect('('); Type *tp = type_name(); expect(')'); return new_node_num(size_of(tp));
        }
        Node *operand = unary(); return new_node_num(size_of(operand->tp));
    }
	return primary();
}

static Node *primary_atom() {
    Node *node;
	if(consume("(")) {
		node = expr();
		if (!consume(")")) error_at(token->str, "')'で閉じてください");
		return node;
        }
	Token *tok = consume_ident();
	if (tok) {
        VarScope *binding = find_name(tok);
        if (binding && binding->typedef_type) error_at(tok->str, "typedef name is not an expression");
		if(tok->kind != TK_IDENT) 
			error_at(token->str, "変数じゃないです");
		Node *node = calloc(1, sizeof(Node));
		if(consume("(")) {
			node->kind = ND_CALL;
			node->func_name = strndup(tok->str, tok->len);
			node->tp = new_type(INT, NULL);
			if (current_function && current_function->func_name &&
			    !strcmp(current_function->func_name, node->func_name))
				node->tp = current_function->tp;
			for (int i = 0; code[i]; i++)
				if (!strcmp(code[i]->func_name, node->func_name)) {
					node->tp = code[i]->tp;
					break;
				}
			add_extern_funcs(node->func_name);
			if(consume(")")) return node;
			for(int i=0;!consume(")");i++){
				if(i > 0 && !consume(",")) error_at(token->str, "','が必要です。");
				node->call_args[i] = assign();
			}
			return node;
		}
		else node->kind = ND_LVAR;
		LVar *lvar = find_lvar(tok);
		if (lvar) {
			node->offset = lvar->offset;
			node->tp = lvar->tp;
		} else {
			GVar *gvar = find_gvar(tok);
			if(gvar){
				node->kind = ND_GVAR;
				node->tp = gvar->tp;
				node->gvar_name = gvar->name;
			}else{
				error_at(tok->str, "定義されていない変数です。");
			}
			// lvar = calloc(1, sizeof(LVar));
			// lvar->next = locals;
			// lvar->name = tok->str;
			// lvar->len = tok->len;
			// if (locals) lvar->offset = locals->offset + 8;
			// else lvar->offset = 8;
			// node->offset = lvar->offset;
			// locals = lvar;
		}

		return node;
	}
	else {
		tok = consume_kind(TK_STR);
		if(tok) {
			node = calloc(1, sizeof(Node));
			node->str = tok->contents;
			node->val = tok->len + 1;
			while (token->kind == TK_STR) {
				Token *next = consume_kind(TK_STR);
				char *joined = calloc(node->val + next->len, 1);
				memcpy(joined, node->str, node->val - 1);
				memcpy(joined + node->val - 1, next->contents, next->len);
				node->str = joined;
				node->val += next->len;
			}
			node->tp = new_type(ARRAY, new_type(CHAR, NULL));
			node->tp->array_size = node->val;
			node->kind = ND_STR;
			return node;
		}
		node = new_node_num(expect_number());
    }
    return node;
}

Node *primary() {
	Node *node = primary_atom();
	for (;;) {
		if (consume("[")) {
			Node *index = expr();
			expect(']');
			Node *address = new_add(node, index);
			if (!address->tp || address->tp->kind != PTR)
				error_at(token->str, "添字には配列かポインタが必要です。");
			node = new_node(ND_DEREF, address, NULL);
			node->tp = node->lhs->tp->ptr_to;
			continue;
		}
		Token *op = token;
		if (consume("++")) node = new_update(ND_POST_INC, node, NULL, op);
		else if (consume("--")) node = new_update(ND_POST_DEC, node, NULL, op);
		else break;
	}
	return node;
}

LVar *find_lvar(Token *tok) {
    VarScope *v = find_name(tok); return v ? v->var : NULL;
}
GVar *find_gvar(Token *tok) {
	for (GVar *gvar = globals;gvar;gvar = gvar->next) {
		if(gvar->len == tok->len && !memcmp(tok->str, gvar->name, gvar->len)) return gvar;
	}
	return NULL;
}

void error(char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
	va_end(ap);
        exit(1);
}

void error_at(char *loc, char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);

        int pos = loc - user_input;
        fprintf(stderr, "%s\n", user_input);
        fprintf(stderr, "%*s", pos, " ");
        fprintf(stderr, "^ ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
	va_end(ap);
        exit(1);
}

// consume() 
// 引数にop と TokenKindを渡してもいい？
bool consume(char *op) {
	// 1.tokenが予約語じゃない
	// 2.len(op) とlen(token->str)の長さが違う
	// 3.token->str とopが違う
        if (token->kind != TK_RESERVED || 
                strlen(op) != token->len ||
                memcmp(token->str, op, token->len))
                return false;
        token = token->next;
        return true;
}

Token *consume_kind(TokenKind tk)
{
	//token->kind がtkと違う
	if(token->kind != tk) return NULL;
	Token *ret = token;
	token = token->next;
	return ret;
}

Token *consume_ident() {
	if (token->kind != TK_IDENT) return NULL;
	Token *ret = token;
	token = token->next;
	return ret;
}

int consume_paramList(Node *node, Type *ty) {
    if (is_typename(token)) {
        Token *saved = token;
        Type *base = declspec();
        if (base->kind == VOID && consume(")")) return 0;
        token = saved;
    }
    int i;
    for (i = 0; !consume(")"); i++) {
        if (i > 0 && !consume(","))
            error_at(token->str, "','が必要です。");
		Token *tok;
        Type *param_ty = declspec();
        tok = NULL;
        param_ty = declarator(param_ty, &tok);
        param_ty = decay_type(param_ty);
        if (param_ty->kind == VOID)
            error_at(tok->str, "voidは単独の引数リストでのみ使えます。");
        if (!tok)
            error_at(token->str, "識別子が必要です。");
        LVar *var = calloc(1, sizeof(LVar));
        declare_name(tok)->var = var;
        var->name = tok->str;
        var->len  = tok->len;
        var->tp   = param_ty;
        var->offset = local_offset(param_ty, locals ? locals->offset : 0, tok);
        var->next = locals;
        locals = var;

        node->params[i] = var;
    }
    return i;
}

void expect(char op) {
        if (token->kind != TK_RESERVED || token->str[0] != op)
                error_at(token->str, "'%c'が続くべきです。", op);
        token = token->next;
}

int expect_number() {
        if (token->kind != TK_NUM)
                error_at(token->str, "数が続くべきです。");
        int val = token->val;
        token = token->next;
        return val;
}

bool at_eof() {
        return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str) {
        Token *tok = calloc(1, sizeof(Token));
        tok->kind = kind;
        tok->str = str;
        cur->next = tok;
        return tok;
}

static int read_escape(char **rest) {
	char *p = *rest;
	char *start = p;
	int value = 0;
	if ('0' <= *p && *p <= '7') {
		for (int i = 0; i < 3 && '0' <= *p && *p <= '7'; i++)
			value = value * 8 + (*p++ - '0');
	} else if (*p == 'x') {
		p++;
		if (!isxdigit((unsigned char)*p))
			error_at(start, "16進エスケープには数字が必要です。");
		while (isxdigit((unsigned char)*p)) {
			int digit = isdigit((unsigned char)*p) ? *p - '0' : tolower((unsigned char)*p) - 'a' + 10;
			value = value * 16 + digit;
			if (value > 255) error_at(start, "エスケープが1バイトの範囲を超えています。");
			p++;
		}
	} else {
		switch (*p++) {
		case 'a': value = 7; break;
		case 'b': value = 8; break;
		case 't': value = 9; break;
		case 'n': value = 10; break;
		case 'v': value = 11; break;
		case 'f': value = 12; break;
		case 'r': value = 13; break;
		case '\\': value = '\\'; break;
		case '\'': value = '\''; break;
		case '"': value = '"'; break;
		case '?': value = '?'; break;
		default: error_at(start, "不正なエスケープです。");
		}
	}
	if (value > 255) error_at(start, "エスケープが1バイトの範囲を超えています。");
	*rest = p;
	return value;
}

static Token *read_literal(Token *cur, char **rest) {
	char *start = *rest;
	char quote = *start;
	char *end = start + 1;
	for (; *end && *end != '\n' && *end != quote; end++) {
		if (*end == '\\') {
			end++;
			if (!*end || *end == '\n') break;
		}
	}
	if (*end != quote)
		error_at(start, "文字・文字列リテラルが閉じていません。");
	char *contents = calloc(end - start, 1);
	int len = 0;
	char *p = start + 1;
	while (p < end) {
		if (*p == '\\') {
			p++;
			contents[len++] = read_escape(&p);
		} else {
			contents[len++] = *p++;
		}
	}
	if (quote == '\'' && len != 1)
		error_at(start, "文字リテラルには1文字が必要です。");
	cur = new_token(quote == '\'' ? TK_NUM : TK_STR, cur, start);
	cur->len = len;
	cur->contents = contents;
	if (quote == '\'') cur->val = (signed char)contents[0];
	*rest = end + 1;
	return cur;
}

void tokenize(char *p) {
    Token head;
	head.next = NULL;
	Token *cur = &head;
        while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
        }
		if (match_token(p, "return")) {
			cur = new_token(TK_RESERVED, cur, p);
			cur->len = 6;
			p += 6;
			continue;
		}
		if (match_token(p, "break")) {
			cur = new_token(TK_RESERVED, cur, p);
			cur->len = 5;
			p += 5;
			continue;
		}
		if (match_token(p, "continue")) {
			cur = new_token(TK_RESERVED, cur, p);
			cur->len = 8;
			p += 8;
			continue;
		}
		if (match_token(p, "if")) {
			cur = new_token(TK_RESERVED, cur, p);
			cur->len = 2;
			p += 2;
			continue;
		}
		if (match_token(p, "while")) {
			cur = new_token(TK_RESERVED, cur, p);
			cur->len = 5;
			p += 5;
			continue;
		}
		if (match_token(p, "for")) {
			cur = new_token(TK_RESERVED, cur, p);
			cur->len = 3;
			p += 3;
			continue;
		}
		if (match_token(p, "else")) {
			cur = new_token(TK_RESERVED, cur, p);
			cur->len = 4;
			p += 4;
			continue;
		}
		if (*p == ',') {
			cur = new_token(TK_RESERVED, cur, p++);
			cur->len = 1;
			continue;
		}
		if ('a' <= *p && *p <= 'z' ||
			'A' <= *p && *p <= 'Z' ||
			(*p == '_')) {
			int i = 0;
			while(is_alnum(p[i])) {
				i++;
			}
			char *ident = strndup(p, i);
			if(is_type(ident)) cur = new_token(TK_TYPE, cur, p);
			else if (strcmp(ident, "sizeof") == 0 || strcmp(ident, "typedef") == 0) cur = new_token(TK_RESERVED, cur, p);
			else cur = new_token(TK_IDENT, cur, p);
			cur->len = i;
			p = p + i;
			continue;
		}
		if (*p == '(' || *p == ')') {
			cur = new_token(TK_RESERVED, cur, p++);
			cur->len = 1;
			continue;
		}

		if (!memcmp(p, ">=", 2) || !memcmp(p, "<=", 2) ||
				!memcmp(p, "++", 2) || !memcmp(p, "--", 2) ||
				!memcmp(p, "==", 2) || !memcmp(p, "!=", 2) ||
				!memcmp(p, "&&", 2) || !memcmp(p, "||", 2) ||
				!memcmp(p, "+=", 2) || !memcmp(p, "-=", 2) || !memcmp(p, "*=", 2) || !memcmp(p, "/=", 2)
			){
			cur = new_token(TK_RESERVED, cur, p);
			cur->len = 2;
			p = p + 2;
			continue;
		}

		if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
				*p == '<' || *p == '>' || *p == '&' || *p == '!') {
			cur = new_token(TK_RESERVED, cur, p++);
			cur->len = 1;
			continue;
		}
		if (*p == '=') {
			cur = new_token(TK_RESERVED, cur, p++);
			cur->len = 1;
			continue;
		}
		if (*p == ';' || *p == ',') {
			cur = new_token(TK_RESERVED, cur, p++);
			cur->len = 1;
			continue;
		}
		if (*p == '{' || *p == '}') {
			cur = new_token(TK_RESERVED, cur, p++);
			cur->len = 1;
			continue;
		}
		if (*p == '[' || *p == ']') {
			cur = new_token(TK_RESERVED, cur, p++);
			cur->len = 1;
			continue;
		}
		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			char buffer[20];
			cur->len = sprintf(buffer, "%d", cur->val);
			continue;
		}
		if (*p == '"' || *p == '\'') {
			cur = read_literal(cur, &p);
			continue;
		}
        error_at(p, "トークナイズできません");
    }
    new_token(TK_EOF, cur, p++);
	token = head.next;
}
