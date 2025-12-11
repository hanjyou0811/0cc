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
