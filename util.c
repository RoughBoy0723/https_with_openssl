// util.c
#include "common.h"

void printErr(const char *msg) 
{
	int flags, line;
	const char *data, *file, *func;
	unsigned long code;

	code = ERR_get_error_all(&file, &line, &func, &data, &flags);
	while (code) {
		printf("error code: %lu in %s func %s line %d.\n", code, file, func, line);
		if (data && (flags & ERR_TXT_STRING))		// if (data가 문자열을 포함)
			printf("\terror data: %s\n", data);
		code = ERR_get_error_all(&file, &line, &func, &data, &flags);
	}
}
