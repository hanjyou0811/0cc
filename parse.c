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
		code[i++] = func();
	}
	code[i] = NULL;
}

Node *func() {
	locals = NULL;
	Node *node = calloc(1, sizeof(Node));
	if(!consume_kind(TK_TYPE)){
		error_at(token->str, "型を書いてください。");
	}
	Type *ty = type();
    Token *tok = consume_ident();
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
			if(!consume_kind(TK_TYPE)) node->init = expr();
			else {
				Type *ty = type();
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
	if(consume("return")) node = new_node(ND_RETURN, expr(), NULL);
	else if(consume_kind(TK_TYPE)) { 
		Type *ty = type();
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

Type *type()
{
	Type *ty = _typename();
	while(consume("*")){
		ty = new_type(PTR, ty);
	}
	return ty;
}

Type *_typename()
{
	Type *ty = new_type(INT, NULL);
	return ty;
}

Node *decl(Type *tp){
	Token *tok = consume_ident();
	if(!tok) error_at(token->str, "識別子が来るべきです。");
	if(consume("[")){
		Node *len_expr = expr();
		expect(']');
		int len = eval_const_expr(len_expr);
		Type *arr = new_type(ARRAY, tp);
		arr->array_size = len;
		tp = arr;
	}
	if (find_lvar(tok))
		error_at(tok->str, "既に定義されている変数です。");
	LVar *lvar = calloc(1, sizeof(LVar));
	lvar->name = tok->str;
	lvar->len = tok->len;
	lvar->tp	= tp;
	if(locals)
		lvar->offset = locals->offset + MAX(size_of(locals->tp), 8);
	else
		lvar->offset = MAX(size_of(tp), 8);
	lvar->next = locals;
	locals = lvar;

	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_DECL;
	node->offset = lvar->offset;
	node->tp = tp;
	if(consume("=")){
		node->kind = ND_LVAR;
		node = new_node(ND_ASSIGN, node, assign());
		node->tp = tp;
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
		node = new_node(ND_ASSIGN, node, assign());
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
			Node *tmp = lhs;
			lhs = rhs;
			rhs = tmp;
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
			node->tp = INT;
			return node;
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
	if(consume("*")) return new_node(ND_DEREF, unary(), NULL);
	if(consume("&")) return new_node(ND_ADDR, unary(), NULL);
	if(consume("sizeof")) {
		Node *node = new_node(ND_SIZE, unary(), NULL);
		node->tp = new_type(INT, NULL);
		return node;
	}
	return primary();
}

Node *primary() {
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
			error_at(tok->str, "定義されていない変数です。");
			// lvar = calloc(1, sizeof(LVar));
			// lvar->next = locals;
			// lvar->name = tok->str;
			// lvar->len = tok->len;
			// if (locals) lvar->offset = locals->offset + 8;
			// else lvar->offset = 8;
			// node->offset = lvar->offset;
			// locals = lvar;
		}
		if(consume("[")) {
			Node *len_expr = expr();
			expect(']');
			node = new_node(ND_DEREF, new_add(node, len_expr), NULL);
			node->tp = node->lhs->tp->ptr_to;
		}
		return node;
	}
	else {
		node = new_node_num(expect_number());
    }
    return node;
}

LVar *find_lvar(Token *tok) {
	for (LVar *var = locals; var; var = var->next) {
		if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) return var;
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

bool consume_kind(TokenKind tk)
{
	//token->kind がtkと違う
	if(token->kind != tk) return false;
	token = token->next;
	return true;
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
        if(!consume_kind(TK_TYPE))
			error_at(token->str, "型が必要です。");
        Type *param_ty = type();
        Token *tok = consume_ident();
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
		if ('a' <= *p && *p <= 'z' || (*p == '_')) {
			int i = 0;
			while(is_alnum(p[i])) {
				i++;
			}
			char *ident = strndup(p, i);
			if(is_type(ident)) cur = new_token(TK_TYPE, cur, p);
			else if(!memcmp(ident, "sizeof", 6)) cur = new_token(TK_RESERVED, cur, p);
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
				!memcmp(p, "==", 2) || !memcmp(p, "!=", 2)){
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
		
        error_at(p, "トークナイズできません");
        }
        new_token(TK_EOF, cur, p++);
	token = head.next;
}
