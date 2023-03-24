#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "./share/fixed.c"
#include "./share/vec3.c"

static void read_header(FILE *fp)
{

}

static const char *read_string(FILE *fp)
{
	static char buf[64] = "";
	char *p = buf;
	int c;

	while ((c = fgetc(fp)) > 0 && p < buf + sizeof (buf))
		*p++ = (char) c;

	*p = 0;

	return buf;
}

static int read_int(FILE *fp)
{
	int value = 0;

	value |=  (unsigned char) fgetc(fp);
	value |= ((unsigned char) fgetc(fp)) << 8;
	value |= ((unsigned char) fgetc(fp)) << 16;
	value |= ((unsigned char) fgetc(fp)) << 24;

	return value;
}

static void write_int(FILE *fp, int value)
{
	fputc( value        & 0xff, fp);
	fputc((value >> 8)  & 0xff, fp);
	fputc((value >> 16) & 0xff, fp);
	fputc((value >> 24) & 0xff, fp);
}

static void write_float(FILE *fp, float value)
{
	write_int(fp, *(int *) &value);
}

static void write_int_be(FILE *fp, int value)
{
	fputc((value >> 24) & 0xff, fp);
	fputc((value >> 16) & 0xff, fp);
	fputc((value >> 8)  & 0xff, fp);
	fputc( value        & 0xff, fp);
}

static float read_float(FILE *fp)
{
	int binary = read_int(fp);
	float value = *(float *) &binary;
	return value;
}

static short read_short(FILE *fp)
{
	short value = 0;

	value |=  (unsigned char) fgetc(fp);
	value |= ((unsigned char) fgetc(fp)) << 8;

	return value;
}

static void read_head(FILE *fp)
{
	fseek(fp, 24, SEEK_CUR);

	(void) read_string(fp);
	(void) read_string(fp);
	(void) read_string(fp);
	(void) read_string(fp);

	fseek(fp, 24, SEEK_CUR);
}

static int leading_zeroes(int value)
{
	int n = 0;

	while (n < 32 && !(value & 1 << 31))
	{
		value <<= 1;
		n++;
	}

	return n;
}

static int trailing_zeroes(int value)
{
	int n = 0;

	while (n < 32 && !(value & 1))
	{
		value >>= 1;
		n++;
	}

	return n;
}

struct uint16_mem
{
	uint16_t *buf;
	int len;
};

struct float_mem
{
	float *buf;
	int len;
};

static struct uint16_mem fixed_encode(float *buf, int n)
{
	struct uint16_mem mem = { NULL };

	// Enough space to fit N 32-bit values, but accessed via a 16-bit pointer.
	uint16_t *buffer = malloc(n * sizeof (uint32_t));
	int last = 0;

	if (!buffer)
		return mem;

	uint16_t curr_high = 0;

	for (int i = 0; i < n; ++i)
	{
		uint32_t fixed = tofixed(buf[i]);
		uint16_t high = (uint16_t) (fixed >> 16) & 0xffff;
		uint16_t low = (uint16_t) (fixed & 0xffff);

		if (high != curr_high)
		{
			curr_high = high;
			buffer[last++] = high;
		}

		buffer[last++] = fixed;
	}

	// Deallocate unused space.
	buffer = realloc(buffer, last * sizeof (*buffer));

	mem.buf = buffer;
	mem.len = last;

	return mem;
}

static struct float_mem fixed_decode(struct uint16_mem mem)
{
	struct float_mem floatbuf = { NULL };

	// Enough space to fit N 32-bit values.
	float *buffer = malloc(sizeof (float) * mem.len * 2);
	int last = 0;

	if (!buffer)
		return floatbuf;

	uint16_t curr_high = mem.len > 0 ? mem.buf[0] : 0;

	for (int i = 1; i < mem.len; ++i)
	{
		if (mem.buf[i] & (1 << 15))
		{
			curr_high = mem.buf[i++];
		}

		uint32_t value = (curr_high << 16) | mem.buf[i];

		buffer[last++] = fromfixed(value);
	}

	buffer = realloc(buffer, last * sizeof (*buffer));

	floatbuf.buf = buffer;
	floatbuf.len = last;

	return floatbuf;
}

static void xor_encode(int *buf, int n)
{
	int i;
	int prev = n > 0 ? buf[0] : 0;

	for (i = 1; i < n; ++i)
	{
		int xor = prev ^ buf[i];
		prev = buf[i];
		buf[i] = xor;
	}
}

static void xor_encode_u16(uint16_t *buf, int n)
{
	int i;
	uint16_t prev = n > 0 ? buf[0] : 0;

	for (i = 1; i < n; ++i)
	{
		uint16_t xor = prev ^ buf[i];
		prev = buf[i];
		buf[i] = xor;
	}
}

static void xor_decode_u16(uint16_t *buf, int n)
{
	int i;

	for (i = 1; i < n; ++i)
	{
		uint16_t xor = buf[i] ^ buf[i - 1];
		buf[i] = xor;
	}
}

static int xpose_int(int value)
{
	return ((value & 0x000000ff) << 24 |
		(value & 0x0000ff00) << 8 |
		(value & 0x00ff0000) >> 8 |
		(value & 0xff000000) >> 24);
}

static uint16_t xpose_u16(uint16_t value)
{
	return ((value & 0x00ff) << 8 |
		(value & 0xff00) >> 8);
}

static void xpose_encode(int *buf, int n)
{
	int i;

	for (i = 0; i < n; ++i)
		if (i % 2 == 1)
			buf[i] = xpose_int(buf[i]);
}

static void xpose_encode_u16(uint16_t *buf, int n)
{
	int i;

	for (i = 0; i < n; ++i)
		if (i % 2 == 1)
			buf[i] = xpose_u16(buf[i]);
}

static FILE *out1 = NULL;
static FILE *out2 = NULL;
static FILE *out3 = NULL;
static FILE *out4 = NULL;
static FILE *out5 = NULL;

static void read_cmds(FILE *fp)
{
	int c;

	int prev_binary = 0;

	int transpose = 0;

	long start = ftell(fp);

	// Count ball position commands to find out dynamic buffer size.

	int total = 0;

	while ((c = fgetc(fp)) >= 0)
	{
		short n = read_short(fp);

		if (c == 22) // ball position
			total++;

		fseek(fp, n, SEEK_CUR);
	}

	// Allocate and fill buffers.

	float *xfv = malloc(total * sizeof (*xfv));
	float *yfv = malloc(total * sizeof (*yfv));
	float *zfv = malloc(total * sizeof (*zfv));

	int *xiv = malloc(total * sizeof (*xiv));
	int *yiv = malloc(total * sizeof (*yiv));
	int *ziv = malloc(total * sizeof (*ziv));

	fseek(fp, start, SEEK_SET);

	int curr = 0;

	while ((c = fgetc(fp)) >= 0)
	{
		short n = read_short(fp);

		// 22 = ball position (vec3)
		// 23 = ball orientation (vec3: x axis, vec3: y axis, z axis is obtained via cross product)
		if (c == 22) // ball position
		{
			float p[3];

			p[0] = read_float(fp);
			p[1] = read_float(fp);
			p[2] = read_float(fp);

			// Array of XYZ positions
			// fwrite(p, sizeof (float), 3, out1);

			// xfv[curr] = p[0];
			// yfv[curr] = p[1];
			// zfv[curr] = p[2];

			curr++;
		}
		else if (c == 23)
		{
			float x[3];
			float y[3];
			float z[3];

			x[0] = read_float(fp);
			x[1] = read_float(fp);
			x[2] = read_float(fp);

			y[0] = read_float(fp);
			y[1] = read_float(fp);
			y[2] = read_float(fp);

			v_crs(z, x, y);
			v_nrm(z, z);

			float c = (x[0] + y[1] + z[2] - 1.0f) / 2.0f;
			float angle = facosf(c);
			float s = fsinf(angle);
			float axis[3] = { 0 };

			if (s == 0.0f)
			{
				// Angle is 0 or multiple of 180.
				//  TODO
				printf("TODO\n");
			}
			else
			{
				axis[0] = (z[1] - y[2]) / s;
				axis[1] = (x[2] - z[0]) / s;
				axis[2] = (y[0] - x[1]) / s;

				v_nrm(axis, axis);
			}

			printf("%f around (%f %f %f)\n", V_DEG(angle), axis[0], axis[1], axis[2]);
		}
		else
		{
			// Jump over.
			fseek(fp, n, SEEK_CUR);
		}
	}

	// Slightly better compressibility:
	// Array of X values (float)
	// Array of Y values (float)
	// Array of Z values (float)
	// fwrite(xfv, sizeof (*xfv), total, out2);
	// fwrite(yfv, sizeof (*yfv), total, out2);
	// fwrite(zfv, sizeof (*zfv), total, out2);

	// Much better compressibility:
	// Array of X values (fixed-point uint16_t)
	// Array of Y values (fixed-point uint16_t)
	// Array of Z values (fixed-point uint16_t)
	// struct uint16_mem xmem = fixed_encode(xfv, total);
	// struct uint16_mem ymem = fixed_encode(yfv, total);
	// struct uint16_mem zmem = fixed_encode(zfv, total);

	// fwrite(xmem.buf, sizeof (*xmem.buf), xmem.len, out3);
	// fwrite(ymem.buf, sizeof (*ymem.buf), ymem.len, out3);
	// fwrite(zmem.buf, sizeof (*zmem.buf), zmem.len, out3);

	// INSANE compressibility:
	// XOR-encoded array of X values (fixed-point uint16_t)
	// XOR-encoded array of Y values (fixed-point uint16_t)
	// XOR-encoded array of Z values (fixed-point uint16_t)
	// xor_encode_u16(xmem.buf, xmem.len);
	// xor_encode_u16(ymem.buf, ymem.len);
	// xor_encode_u16(zmem.buf, zmem.len);

	// fwrite(xmem.buf, sizeof (*xmem.buf), xmem.len, out4);
	// fwrite(ymem.buf, sizeof (*ymem.buf), ymem.len, out4);
	// fwrite(zmem.buf, sizeof (*zmem.buf), zmem.len, out4);
}

int main(int argc, char *argv[])
{
	FILE *fp = fopen("easy-V_02.nbr", "rb");

	// out1 = fopen("out1.bin", "wb");
	// out2 = fopen("out2.bin", "wb");
	// out3 = fopen("out3.bin", "wb");
	// out4 = fopen("out4.bin", "wb");
	// out5 = fopen("out5.bin", "wb");

	if (fp)
	{
		read_head(fp);
		read_cmds(fp); // ball position

		fclose(fp);
	}

	fclose(out1);
	fclose(out2);
	fclose(out3);

	return 0;
}
