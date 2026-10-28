#include "0cc.h"

int println(const char *fmt, ...)
{
	int res = 0;
	va_list args;
	va_start(args, fmt);
	res += vprintf(fmt, args) + printf("\n");
	va_end(args);
	return res;
}

int is_alnum(char c)
{
	return ('a' <= c && c <= 'z') || 
		('A' <= c && c <= 'Z') ||
		('0' <= c && c <= '9') ||
		(c == '_');
}

int is_type(char *ident){
    return strcmp(ident, "int") == 0 ||
           strcmp(ident, "char") == 0;
}

int match_token(char *str, char *tgt)
{
	int size1 = strlen(str), size2 = strlen(tgt);
	if (size1 < size2) return false;
	if(!memcmp(str, tgt, size2) && !is_alnum(str[size2])) {
		return true;
	}
	return false;
}

char *strndup(const char *ptr, int len)
{
        char *ret = calloc(len + 1, sizeof(char));
        memcpy(ret, ptr, len);
	return ret;
}

char *strdup(const char *s)
{
    size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if (!p) return NULL;
    memcpy(p, s, len);
    return p;
}

extern_funcs *add_extern_funcs(const char *funcname)
{
	extern_funcs *ptr = calloc(1, sizeof(extern_funcs));
	ptr->func_name = strdup(funcname);
	ptr->nex = funcs;
	funcs = ptr;
	return ptr;
}

int size_of(Type *tp) {
    switch (tp->kind) {
    case INT:
        return 4;
    case PTR:
        return 8;
	case CHAR:
		return 1;
    case ARRAY:
        return size_of(tp->ptr_to) * tp->array_size;
    }
}

int is_function()
{
	// func := type ident "(" ...
	Token *t = token;
	// type
	if(t->kind != TK_TYPE) return false;
	t = t->next;
	// ident
	if(t->kind != TK_IDENT) return false;
	t = t->next;
	// "("
	return (t->kind == TK_RESERVED && t->str[0] == '(');
}

char *read_file(char *path) {
  // ファイルを開く
  FILE *fp = fopen(path, "r");
  if (!fp)
    error("cannot open %s: %s", path, strerror(errno));

  // ファイルの長さを調べる
  if (fseek(fp, 0, SEEK_END) == -1)
    error("%s: fseek: %s", path, strerror(errno));
  size_t size = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1)
    error("%s: fseek: %s", path, strerror(errno));

  // ファイル内容を読み込む
  char *buf = calloc(1, size + 2);
  fread(buf, size, 1, fp);

  // ファイルが必ず"\n\0"で終わっているようにする
  if (size == 0 || buf[size - 1] != '\n')
    buf[size++] = '\n';
  buf[size] = '\0';
  fclose(fp);
  return buf;
}

int has_value(Node *node)
{
	switch (node->kind) {
	case ND_ASSIGN:
		return 1;
	case ND_STR:
	case ND_CALL:
	case ND_NUM:
	case ND_LVAR:
	case ND_GVAR:
	case ND_ADD:
	case ND_SUB:
	case ND_MUL:
	case ND_DIV:
	case ND_EQL:
	case ND_NEQL:
	case ND_LST:
	case ND_LSE:
	case ND_ADDR:
	case ND_DEREF:
	case ND_SIZE:
		return 1;
	default:
		return 0;
	}
}
