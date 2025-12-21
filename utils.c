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
	return (!memcmp(ident, "int", 3));
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
    case ARRAY:
        return size_of(tp->ptr_to) * tp->array_size;
    }
}

int max(int a, int b)
{
	if(a < b) return a;
	return b;
}