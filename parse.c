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

void program() {	
	int i = 0;
	while(!at_eof()) {
		code[i++] = stmt();
	}
	code[i] = NULL;
}

Node *stmt() {
	Node *node = expr();
	consume(";");
	return node;
}

Node *expr() {
        return assign();
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
                consume(")");
        }
	Token *tok = consume_ident();
	if (tok) {
		Node *node = calloc(1, sizeof(Node));
		node->kind = ND_LVAR;
		node->offset = (tok->str[0] - 'a' + 1) * 8;
		return node;
	}
	else {
                node = new_node_num(expect_number());
        }
        return node;
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

bool consume(char *op) {
        if (token->kind != TK_RESERVED ||
                strlen(op) != token->len ||
                memcmp(token->str, op, token->len))
                return false;
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
                error_at(token->str, "'%c'ではありません", op);
        token = token->next;
}

int expect_number() {
        if (token->kind != TK_NUM) \
                error_at(token->str, "数ではありません");
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
        while (*p && *p != ';') {
		if (isspace(*p)) {
                        p++;
                        continue;
                }
		
		if ('a' <= *p && *p <= 'z') {
			cur = new_token(TK_IDENT, cur, p++);
			cur->len = 1;
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
                if (isdigit(*p)) {
                        cur = new_token(TK_NUM, cur, p);
                        cur->val = strtol(p, &p, 10);
                        char buffer[20];
                        cur->len = sprintf(buffer, "%d", cur->val);
                        continue;
                }
		
                error_at(p, "トークナイズできません");
        }
	if(*p != ';') error_at(p, "不正な終わり方です。");
        new_token(TK_EOF, cur, p++);
	token = head.next;
}
