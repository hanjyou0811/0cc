#ifndef CC_H
#define CC_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

typedef enum {
        TK_RESERVED,
//	TK_RETURN,
        TK_IDENT,
	TK_NUM,
        TK_EOF,
}       TokenKind;
typedef struct Token Token;
struct Token {
        TokenKind kind;
        Token *next;
        int val;
        char *str;
        int len;
};
typedef enum {
        ND_ADD,         // +
        ND_SUB,         // -
        ND_MUL,         // *
        ND_DIV,         // /
        ND_NUM,         // num
	ND_ASSIGN,	// = 
        ND_EQL,         // ==
        ND_NEQL,        // !=
        ND_LST,         // <
        ND_LSE,         // <=
        ND_LVAR, 	// Local var
	ND_RETURN,	// return
        ND_IF,		// if
        ND_WHILE,	// while
        ND_FOR,		// for
	ND_COMMA,	// ','
	ND_BLOCK,	// {}
	ND_FUNC
}       NodeKind;
typedef struct Node Node;
struct Node {
        NodeKind kind;  //Node

        Node *lhs;      //left
        Node *rhs;      //right

        Node *cond;     //condition (if / while / for)
        Node *then;     //then (if / while)
        Node *els;      //else (if)
        Node *init;     //initialization (for)
        Node *inc;      //increment (for)

        Node *body;     //body (while / for)
	Node *stmts[100];
        int val;        //kind == ND_NUM
	int offset;
	char *func_name;
};
typedef struct LVar LVar;
struct LVar {
	LVar *next;
	char *name;
	int len;
	int offset;
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
bool consume_kind(TokenKind tk);
Token *consume_ident();
void expect(char op);
int expect_number();
bool at_eof();
void tokenize(char *p);
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
void program();
Node *assign();
Node *expr();
Node *stmt();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();
LVar *find_lvar(Token *tok);

void gen(Node *node);

extern Token *token;
extern char *user_input;
extern Node *code[100];
extern LVar *locals;
extern int lavel_id;

// utils.c
int println(const char *fmt, ...);
int is_alnum(char c);
int match_token(char *src, char *tgt);

#endif
