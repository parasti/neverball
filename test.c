#include <stdio.h>
#include "./share/fixed.c"
#include "binary-string.c"

int main(void)
{
	float value = 64.0;
	uint32_t fixed = tofixed(value);
	float single = fromfixed(fixed);

	printf("%f %s %f\n", value, BINARY_STRING(fixed, 32), single);

	return 0;
}
