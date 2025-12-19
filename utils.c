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

