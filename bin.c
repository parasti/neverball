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
static FILE *out6 = NULL;
static FILE *out7 = NULL;
static FILE *out8 = NULL;
static FILE *out9 = NULL;
static FILE *out10 = NULL;

static void read_positions(FILE *fp)
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

		if (c == 22) // ball position
		{
			float p[3];

			// Interleaved array of XYZ values.

			p[0] = read_float(fp);
			p[1] = read_float(fp);
			p[2] = read_float(fp);

			fwrite(p, sizeof (float), 3, out1);

			// Separate arrays of X, Y and Z values.

			xfv[curr] = p[0];
			yfv[curr] = p[1];
			zfv[curr] = p[2];

			curr++;
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
	fwrite(xfv, sizeof (*xfv), total, out2);
	fwrite(yfv, sizeof (*yfv), total, out2);
	fwrite(zfv, sizeof (*zfv), total, out2);

	// Much better compressibility:
	// Array of X values (fixed-point uint16_t)
	// Array of Y values (fixed-point uint16_t)
	// Array of Z values (fixed-point uint16_t)
	struct uint16_mem xmem = fixed_encode(xfv, total);
	struct uint16_mem ymem = fixed_encode(yfv, total);
	struct uint16_mem zmem = fixed_encode(zfv, total);

	fwrite(xmem.buf, sizeof (*xmem.buf), xmem.len, out3);
	fwrite(ymem.buf, sizeof (*ymem.buf), ymem.len, out3);
	fwrite(zmem.buf, sizeof (*zmem.buf), zmem.len, out3);

	// INSANE compressibility:
	// XOR-encoded array of X values (fixed-point uint16_t)
	// XOR-encoded array of Y values (fixed-point uint16_t)
	// XOR-encoded array of Z values (fixed-point uint16_t)
	xor_encode_u16(xmem.buf, xmem.len);
	xor_encode_u16(ymem.buf, ymem.len);
	xor_encode_u16(zmem.buf, zmem.len);

	fwrite(xmem.buf, sizeof (*xmem.buf), xmem.len, out4);
	fwrite(ymem.buf, sizeof (*ymem.buf), ymem.len, out4);
	fwrite(zmem.buf, sizeof (*zmem.buf), zmem.len, out4);
}

static void read_ball_basis(FILE *fp)
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

		if (c == 23) // ball basis
			total++;

		fseek(fp, n, SEEK_CUR);
	}

	// Deinterlaved float buffers.

	float *x0fv = malloc(total * sizeof (*x0fv));
	float *x1fv = malloc(total * sizeof (*x1fv));
	float *x2fv = malloc(total * sizeof (*x2fv));
	float *y0fv = malloc(total * sizeof (*y0fv));
	float *y1fv = malloc(total * sizeof (*y1fv));
	float *y2fv = malloc(total * sizeof (*y2fv));

	// Deinterleaved int buffers.

	int *x0iv = malloc(total * sizeof (*x0iv));
	int *x1iv = malloc(total * sizeof (*x1iv));
	int *x2iv = malloc(total * sizeof (*x2iv));
	int *y0iv = malloc(total * sizeof (*y0iv));
	int *y1iv = malloc(total * sizeof (*y1iv));
	int *y2iv = malloc(total * sizeof (*y2iv));

	// Deinterleaved axis-angle float buffers.

	float *a0fv = malloc(total * sizeof (float));
	float *a1fv = malloc(total * sizeof (float));
	float *a2fv = malloc(total * sizeof (float));
	float *a3fv = malloc(total * sizeof (float));

	// Deinterleaved axis-angle int buffers.

	int *a0iv = malloc(total * sizeof (int));
	int *a1iv = malloc(total * sizeof (int));
	int *a2iv = malloc(total * sizeof (int));
	int *a3iv = malloc(total * sizeof (int));

	// Deinterleaved axis-angle u16 buffers.

	uint16_t *a0uv = malloc(total * sizeof (*a0uv));
	uint16_t *a1uv = malloc(total * sizeof (*a1uv));
	uint16_t *a2uv = malloc(total * sizeof (*a2uv));
	uint16_t *a3uv = malloc(total * sizeof (*a3uv));

        // Spherical coords int buffers.

        int *siv = malloc(total * sizeof (int));

	fseek(fp, start, SEEK_SET);

	int curr = 0;

	while ((c = fgetc(fp)) >= 0)
	{
		short n = read_short(fp);

		if (c == 23) // ball orientation
		{
			float e[2][3];

			e[0][0] = read_float(fp);
			e[0][1] = read_float(fp);
			e[0][2] = read_float(fp);

			e[1][0] = read_float(fp);
			e[1][1] = read_float(fp);
			e[1][2] = read_float(fp);

			// Just write the 6 floats out again.

			fwrite(e, sizeof (float), 6, out1);

			// Deinterleaved floats.

			x0fv[curr] = e[0][0];
			x1fv[curr] = e[0][1];
			x2fv[curr] = e[0][2];
			y0fv[curr] = e[1][0];
			y1fv[curr] = e[1][1];
			y2fv[curr] = e[1][2];

			// Deinterleaved ints.

			x0iv[curr] = *(int *) &x0fv[curr];
			x1iv[curr] = *(int *) &x1fv[curr];
			x2iv[curr] = *(int *) &x2fv[curr];
			y0iv[curr] = *(int *) &y0fv[curr];
			y1iv[curr] = *(int *) &y1fv[curr];
			y2iv[curr] = *(int *) &y2fv[curr];

			// Axis-angle

			float u[4], v[3], a, aa;

			a = v_axisangle(v, e[0], e[1]);
			aa = a / V_PI;

			u[0] = v[0];
			u[1] = v[1];
			u[2] = v[2];
			u[3] = a;

			fwrite(u, sizeof (float), 4, out3);

			a0fv[curr] = v[0];
			a1fv[curr] = v[1];
			a2fv[curr] = v[2];
			a3fv[curr] = aa;

			// Deinterleaved ints.

			a0iv[curr] = *(int *) &v[0];
			a1iv[curr] = *(int *) &v[1];
			a2iv[curr] = *(int *) &v[2];
			a3iv[curr] = *(int *) &aa;

			// Deinterleaved uint16s.

			a0uv[curr] = 0x7fff + v[0] * 0x7fff;
			a1uv[curr] = 0x7fff + v[1] * 0x7fff;
			a2uv[curr] = 0x7fff + v[2] * 0x7fff;
			a3uv[curr] = 0x7fff + aa * 0x7fff;

			// Spherical coords and angle packed into a 32-bit int.

                        float theta, phi, sa;

                        theta = facosf(v[2]) / V_PI;
                        phi = fatan2f(v[1], v[0]) / V_PI;
                        sa = a / V_PI;

                        // 11 bits per coord, 10 bits for angle.
                        unsigned short utheta = 0x3ff + 0x3ff * theta;
                        unsigned short uphi = 0x3ff + 0x3ff * theta;
                        unsigned short uangle = 0x1ff + 0x1ff * sa;

                        siv[curr] = (utheta << 21 | uphi << 10 | uangle);

			curr++;
		}
		else
		{
			// Jump over.
			fseek(fp, n, SEEK_CUR);
		}
	}

	// Deinterleaved floats: nearly the same compression, by 0.0007554785686918475

	fwrite(x0fv, sizeof (*x0fv), total, out2);
	fwrite(x1fv, sizeof (*x1fv), total, out2);
	fwrite(x2fv, sizeof (*x2fv), total, out2);
	fwrite(y0fv, sizeof (*y0fv), total, out2);
	fwrite(y1fv, sizeof (*y1fv), total, out2);
	fwrite(y2fv, sizeof (*y2fv), total, out2);

	// XOR-encoded deinterleaved ints: slightly better compression, by 0.058587363002056025

	xor_encode(x0iv, total);
	xor_encode(x1iv, total);
	xor_encode(x2iv, total);
	xor_encode(y0iv, total);
	xor_encode(y1iv, total);
	xor_encode(y2iv, total);

	fwrite(x0iv, sizeof (*x0iv), total, out4);
	fwrite(x1iv, sizeof (*x1iv), total, out4);
	fwrite(x2iv, sizeof (*x2iv), total, out4);
	fwrite(y0iv, sizeof (*y0iv), total, out4);
	fwrite(y1iv, sizeof (*y1iv), total, out4);
	fwrite(y2iv, sizeof (*y2iv), total, out4);

	// Deinterleaved axis-angle.

	fwrite(a0fv, sizeof (float), total, out5);
	fwrite(a1fv, sizeof (float), total, out5);
	fwrite(a2fv, sizeof (float), total, out5);
	fwrite(a3fv, sizeof (float), total, out5);

	// XOR-encoded deinterleaved axis-angle.

	xor_encode(a0iv, total);
	xor_encode(a1iv, total);
	xor_encode(a2iv, total);
	xor_encode(a3iv, total);

	fwrite(a0iv, sizeof (int), total, out6);
	fwrite(a1iv, sizeof (int), total, out6);
	fwrite(a2iv, sizeof (int), total, out6);
	fwrite(a3iv, sizeof (int), total, out6);

        // Transposed deinterleaved axis-angle.

	xpose_encode(a0iv, total);
	xpose_encode(a1iv, total);
	xpose_encode(a2iv, total);
	xpose_encode(a3iv, total);

	fwrite(a0iv, sizeof (int), total, out7);
	fwrite(a1iv, sizeof (int), total, out7);
	fwrite(a2iv, sizeof (int), total, out7);
	fwrite(a3iv, sizeof (int), total, out7);

	// Unsigned 16-bit deinterleaved axis-angle.

	fwrite(a0uv, sizeof (uint16_t), total, out8);
	fwrite(a1uv, sizeof (uint16_t), total, out8);
	fwrite(a2uv, sizeof (uint16_t), total, out8);
	fwrite(a3uv, sizeof (uint16_t), total, out8);

        // XOR-encoded 16-bit deinterleaved axis-angle.

	xor_encode_u16(a0uv, total);
	xor_encode_u16(a1uv, total);
	xor_encode_u16(a2uv, total);
	xor_encode_u16(a3uv, total);

	fwrite(a0uv, sizeof (uint16_t), total, out9);
	fwrite(a1uv, sizeof (uint16_t), total, out9);
	fwrite(a2uv, sizeof (uint16_t), total, out9);
	fwrite(a3uv, sizeof (uint16_t), total, out9);

	// XOR-encoded packed 32-bit axis-angle.

        xor_encode(siv, total);

        fwrite(siv, sizeof (int), total, out10);
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s input.nbr\n", argv[0]);
		return 0;
	}

	FILE *fp = fopen(argv[1], "rb");

	out1 = fopen("out1.bin", "wb");
	out2 = fopen("out2.bin", "wb");
	out3 = fopen("out3.bin", "wb");
	out4 = fopen("out4.bin", "wb");
	out5 = fopen("out5.bin", "wb");
	out6 = fopen("out6.bin", "wb");
	out7 = fopen("out7.bin", "wb");
	out8 = fopen("out8.bin", "wb");
	out9 = fopen("out9.bin", "wb");
	out10 = fopen("outA.bin", "wb");

	if (fp)
	{
		read_head(fp);
		// read_positions(fp);
		read_ball_basis(fp);

		fclose(fp);
	}

	fclose(out1);
	fclose(out2);
	fclose(out3);
	fclose(out4);
	fclose(out5);
	fclose(out6);
	fclose(out7);
	fclose(out8);
	fclose(out9);
	fclose(out10);

	return 0;
}
