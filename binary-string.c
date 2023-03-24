struct binary_str
{
	char buf[32 + 1];
};

struct binary_str binary_string(int value, int bits)
{
	struct binary_str str;
	char *p = str.buf;

	if (bits > 0 && bits < 32)
		value <<= (32 - bits);

	while (p < str.buf + sizeof (str.buf) - 1 && bits > 0)
	{
		*p++ = (value & (1 << 31)) ? '1' : '0';
		value <<= 1;
		bits--;
	}

	*p = 0;

	return str;
}

#define BINARY_STRING(value, bits) (binary_string((value), (bits)).buf)
