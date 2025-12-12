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

void program() {	
	int i = 0;
	while(!at_eof()) {
		code[i++] = stmt();
	}
	code[i] = NULL;
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
			node->init = expr();
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
	if(consume("return")) node = new_node(ND_RETURN, expr(), NULL);
	else node = expr();
	if (!consume(";")) error_at(token->str, "';'で終わっていないです");
	
	return node;
}

Node *expr() {
	Node *node = assign();
	if(consume(",")) node = new_node(ND_COMMA, node, expr());
        return node;
}

Node *assign() {
	Node *node = equality();
	if(consume("=")) node = new_node(ND_ASSIGN, node, assign());
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

Node *add() {
        Node *node = mul();

        for(;;) {
                if(consume("+")) node = new_node(ND_ADD, node, mul());
                else if(consume("-")) node = new_node(ND_SUB, node, mul());
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
		node->kind = ND_LVAR;
		LVar *lvar = find_lvar(tok);
		if (lvar) {
			node->offset = lvar->offset;
		} else {
			lvar = calloc(1, sizeof(LVar));
			lvar->next = locals;
			lvar->name = tok->str;
			lvar->len = tok->len;
			if (locals) lvar->offset = locals->offset + 8;
			else lvar->offset = 8;
			node->offset = lvar->offset;
			locals = lvar;
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
		if ('a' <= *p && *p <= 'z') {
			int i = 0;
			while(isalnum(p[i])) {
				i++;
			}
			cur = new_token(TK_IDENT, cur, p);
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
                        *p == '<' || *p == '>') {
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
