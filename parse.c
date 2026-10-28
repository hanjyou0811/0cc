#include "0cc.h"

Node *code[100];

Node *new_node(NodeKind kind, Node *lhs, Node *rhs){
	Node *node      = calloc(1, sizeof(Node));
	node->kind      = kind;
	node->lhs       = lhs;
	node->rhs       = rhs;
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
	while(!at_eof()) {
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
	Token *tok = consume_kind(TK_TYPE);
	if(!tok){
		error_at(token->str, "型を書いてください。");
	}
	Type *ty = type(tok);
	tok = consume_kind(TK_IDENT);
	if(!tok || tok->kind != TK_IDENT) error_at(token->str, "識別子が来るべきです。");
	node->kind = ND_FUNC;
	node->func_name = strndup(tok->str, tok->len);
	node->tp = ty;
	if(!consume("(")) 
		error_at(token->str, "'('が来るべきです。");
	if(!consume(")")) 
		node->argc = consume_paramList(node, ty);
	node->body = block();
	return node;
}

Node *block(){
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;
	LVar *saved = locals;
	if(!consume("{")) error_at(token->str, "'{'が必要です。");
    int i = 0;
    while (!consume("}")) {
        node->stmts[i++] = stmt();
    }
    node->stmts[i] = NULL;
	locals = saved;
	return node;
}

Node *stmt() {
	Node *node;
	
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
		Node *then = stmt();
		node = new_node_while(cond, then);
		return node;
	}
	if (consume("for")) {
		node = new_node(ND_FOR, NULL, NULL);
		if (!consume("(")) error_at(token->str, "'('ではじめてください");
		// init
		if (!consume(";")) {
			Token *tok = consume_kind(TK_TYPE);
			if(!tok) node->init = expr();
			else {
				Type *ty = type(tok);
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
		node->body = stmt();
		return node;
	}
	if (token->str[0] == '{') {
		return block();
	}
	Token *tok = consume_kind(TK_TYPE);
	if(consume("return")) node = new_node(ND_RETURN, expr(), NULL);
	else if(tok) { 
		Type *ty = type(tok);
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
	Token *tok = consume_kind(TK_TYPE);
	if(!tok) {
		error_at(token->str, "型が必要です。");
	}
	Type *ty = type(tok);
	decl_global(ty);
	expect(';');
}

void decl_global(Type *ty)
{
	Token *tok = consume_ident();
	GVar *gvar = calloc(1, sizeof(GVar));
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

Type *type(Token *tok)
{
	Type *ty = _typename(tok);
	while(consume("*")){
		ty = new_type(PTR, ty);
	}
	return ty;
}

Type *_typename(Token *tok)
{
	Typename tp;
	if(!memcmp(tok->str, "int", 3)) tp = INT;
	else if(!memcmp(tok->str, "char", 4)) tp = CHAR;
	Type *ty = new_type(tp, NULL);
	return ty;
}

Node *decl(Type *tp){
	Token *tok = consume_ident();
	if(!tok) error_at(token->str, "識別子が来るべきです。");
	bool infer_size = false;
	if(consume("[")){
		int len = 0;
		if (consume("]")) infer_size = true;
		else {
			len = eval_const_expr(expr());
			expect(']');
			if (len <= 0) error_at(tok->str, "配列サイズは正の整数が必要です。");
		}
		Type *arr = new_type(ARRAY, tp);
		arr->array_size = len;
		tp = arr;
	}
	if (find_lvar(tok) || find_gvar(tok))
		error_at(tok->str, "既に定義されている変数です。");
	LVar *lvar = calloc(1, sizeof(LVar));
	lvar->name = tok->str;
	lvar->len = tok->len;
	lvar->tp = tp;
	lvar->offset = (locals ? locals->offset : 0) + MAX(size_of(tp), 8);
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
				lvar->offset = (lvar->next ? lvar->next->offset : 0) + MAX(size_of(tp), 8);
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
	}
    return node;
}

Node *assign() {
	Node *node = equality();
	if(consume("=")) {
		if (node->tp && node->tp->kind == ARRAY)
			error_at(token->str, "配列には代入できません。");
		node = new_node(ND_ASSIGN, node, assign());
		node->tp = node->lhs->tp;
	}
	else if(consume("+=")) {
		Node *rhs = new_add(node, assign());
		node = new_node(ND_ASSIGN, node, rhs);
		node->tp = node->lhs->tp;
	}else if(consume("-=")) {
		Node *rhs = new_sub(node, assign());
		node = new_node(ND_ASSIGN, node, rhs);
		node->tp = node->lhs->tp;
	}else if(consume("*=")) {
		Node *rhs = new_node(ND_MUL, node, assign());
		node = new_node(ND_ASSIGN, node, rhs);
		node->tp = node->lhs->tp;
	}else if(consume("/=")) {
		Node *rhs = new_node(ND_DIV, node,assign());
		node = new_node(ND_ASSIGN, node, rhs);
		node->tp = node->lhs->tp;
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

Node *new_add(Node *lhs, Node *rhs)
{
	Node *node = new_node(ND_ADD, lhs, rhs);
	if(lhs->tp && rhs->tp){
		if (lhs->tp->kind == INT && (rhs->tp->kind == PTR || rhs->tp->kind == ARRAY))
		{
			Node *tmp = node->lhs;
			node->lhs = node->rhs;
			node->rhs = tmp;
		}
		if((lhs->tp->kind == ARRAY || lhs->tp->kind == PTR) && rhs->tp->kind == INT)
		{
			int size = size_of(lhs->tp->ptr_to);
			Node *mul = new_node(ND_MUL, rhs, new_node_num(size));
			mul->tp = INT;
			node->rhs = mul;
			node->tp = lhs->tp;
			return node;
		}
	}
	node->tp = new_type(INT, NULL);
	return node;
}

Node *new_sub(Node *lhs, Node *rhs)
{
	Node *node = new_node(ND_SUB, lhs, rhs);
	if(lhs->tp && rhs->tp){
		if ((lhs->tp->kind == PTR || lhs->tp->kind == ARRAY) && rhs->tp->kind == INT)
		{
			int size = size_of(lhs->tp->ptr_to);
			rhs = new_node(ND_MUL, rhs, new_node_num(size));
			rhs->tp = INT;
			node->rhs = rhs;
			node->tp = lhs->tp;
			return node;
		}
		if((lhs->tp->kind == PTR || lhs->tp->kind == ARRAY) && (rhs->tp->kind == PTR || rhs->tp->kind == ARRAY)) {
			Node *div = new_node(ND_DIV, node, new_node_num(size_of(lhs->tp->ptr_to)));
			div->tp = INT;
			return div;
		}
	}
	node->tp = new_type(INT, NULL);
	return node;
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
	if(consume("+")) return primary();
	if(consume("-")) return new_node(ND_SUB, new_node_num(0), primary());
	if(consume("*")) 
	{
		Node *lhs = unary();
		Node *node = new_node(ND_DEREF, lhs, NULL);
		node->tp = lhs->tp->ptr_to;
		return node;
	}
	if(consume("&")) {
		Node *node = new_node(ND_ADDR, unary(), NULL);
		node->tp = new_type(PTR, node->lhs->tp);
		return node;
	}
	if(consume("sizeof")) {
		Node *node = new_node(ND_SIZE, unary(), NULL);
		node->tp = new_type(INT, NULL);
		return node;
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
		if(tok->kind != TK_IDENT) 
			error_at(token->str, "変数じゃないです");
		Node *node = calloc(1, sizeof(Node));
		if(consume("(")) {
			node->kind = ND_CALL;
			node->func_name = strndup(tok->str, tok->len);
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
	while (consume("[")) {
		Node *index = expr();
		expect(']');
		if (!node->tp || (node->tp->kind != PTR && node->tp->kind != ARRAY))
			error_at(token->str, "添字には配列かポインタが必要です。");
		node = new_node(ND_DEREF, new_add(node, index), NULL);
		node->tp = node->lhs->tp->ptr_to;
	}
	return node;
}

LVar *find_lvar(Token *tok) {
	for (LVar *var = locals; var; var = var->next) {
		if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) return var;
	}
	return NULL;
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
    int i;
    for (i = 0; !consume(")"); i++) {
        if (i > 0 && !consume(","))
            error_at(token->str, "','が必要です。");
		Token *tok = consume_kind(TK_TYPE);
        if(!tok)
			error_at(token->str, "型が必要です。");
        Type *param_ty = type(tok);
        tok = consume_ident();
        if (!tok)
            error_at(token->str, "識別子が必要です。");
        LVar *var = calloc(1, sizeof(LVar));
        var->name = tok->str;
        var->len  = tok->len;
        var->tp   = param_ty;
        if (locals)
            var->offset = locals->offset + MAX(size_of(locals->tp), 8);
        else
            var->offset = MAX(size_of(param_ty), 8);
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
			else if (strcmp(ident, "sizeof") == 0) cur = new_token(TK_RESERVED, cur, p);
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
				!memcmp(p, "==", 2) || !memcmp(p, "!=", 2) ||
				!memcmp(p, "+=", 2) || !memcmp(p, "-=", 2) || !memcmp(p, "*=", 2) || !memcmp(p, "/=", 2)
			){
			cur = new_token(TK_RESERVED, cur, p);
			cur->len = 2;
			p = p + 2;
			continue;
		}

		if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
				*p == '<' || *p == '>' || *p == '&') {
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
