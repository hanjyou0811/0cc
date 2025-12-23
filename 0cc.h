#ifndef CC_H
#define CC_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#define MAX(x, y) ((x) < (y) ? (y) : (x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

typedef enum {
        TK_RESERVED,
//	TK_RETURN,
        TK_IDENT,
	TK_NUM,
        TK_EOF,
        TK_TYPE,
        TK_STR,
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
        INT,
        PTR,
        CHAR,
        STR,
        ARRAY,
}       Typename;
typedef struct Type Type;
struct Type {
        Typename kind;
        struct Type *ptr_to;
        size_t array_size;
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
	ND_CALL,        // call function
        ND_FUNC,	// func def
	ND_ADDR,	// &x
	ND_DEREF,	// *x
        ND_DEF,         // def val
        ND_DECL,        
        ND_SIZE,        //sizeof x
        ND_GVAR,
        ND_STR,
}       NodeKind;
typedef struct LVar LVar;
struct LVar {
	LVar *next;
	char *name;
	int len;
	int offset;
        Type *tp;
};
typedef struct GVar GVar;
struct GVar {
        GVar *next;

        char *name;
        int len;
        Type *tp;
        
        int val;        // int
        int *arr;       // arr
};
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
	Node *stmts[100];	// blcok
	Node *call_args[100];

        LVar *params[100];

        int val;        //kind == ND_NUM
        char *str;      //kind == ND_STR
	int offset;
	char *func_name;
        char *gvar_name;
        int argc;

        struct Type *tp;
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
Token *consume_kind(TokenKind tk);
Token *consume_ident();
int consume_paramList(Node *node, Type *ty);
void expect(char op);
int expect_number();
bool at_eof();
void tokenize(char *p);
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_add(Node *lhs, Node *rhs);
Node *new_sub(Node *lhs, Node *rhs);
void program();
Node *func();
Node *def_stmt(Type *tp);
Node *decl(Type *tp);
void global_decl();
void decl_global(Type *ty);
Node *block();
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
GVar *find_gvar(Token *tok);

Type *type(Token *tok);
Type *_typename(Token *tok);
Type *new_type(Typename typename, Type *next);


void gen(Node *node);
void gen_gval();

extern Token *token;
extern char *user_input;
extern Node *code[100];
extern LVar *locals;
extern GVar *globals;
extern int lavel_id;
extern const char *arg_addr[];

typedef struct extern_funcs extern_funcs;
struct extern_funcs {
        char *func_name;
        extern_funcs *nex;
};
extern extern_funcs *funcs;
extern_funcs *add_extern_funcs(const char *funcname);

// utils.c
int println(const char *fmt, ...);
int is_alnum(char c);
int is_type(char *ident);
int match_token(char *src, char *tgt);
char *strndup(const char *ptr, int len);
char *strdup(const char *s);
int size_of(Type *tp);
int is_function();
int has_value(Node *node);

#endif
