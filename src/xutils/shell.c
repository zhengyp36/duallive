#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

int
shell(const char *fmt, ...)
{
	char cmd[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(cmd, sizeof(cmd), fmt, ap);
	va_end(ap);
	return (system(cmd));
}
