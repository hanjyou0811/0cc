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
	
}       NodeKind;
typedef struct Node Node;
struct Node {
        NodeKind kind;  //Node
        Node *lhs;      //left
        Node *rhs;      //right
        int val;        //kind == ND_NUM
	int offset;
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
Token *consume_ident();
void expect(char op);
int expect_number();
bool at_eof();
Token *tokenize(char *p);
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
void gen(Node *node);

extern Token *token;
extern char *user_input;
extern Node *code[100];

#endif
