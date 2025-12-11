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
