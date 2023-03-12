#include <stdint.h>
#include <math.h>

/*
 * Encode coordinates into a 32-bit fixed-point value of two 16-bit words: High
 * word is a 1-bit in the MSB, followed by a sign bit, followed by 14 bits of
 * value. Low word is a 0-bit in the MSB followed by 15 bits of value.
 *
 * 9 out of 15 value bits in the low word encode the fractional part of the
 * Neverball unit. 9 bits equals 512 distinct values, which is sufficient for
 * replay playback.
 *
 * High word         Low word
 * 10000000 00000000 00000000 00000000
 * ^^^^^^^^ ^^^^^^^^ ^^^^^^^^ ^^^^^^^^
 * ||              | |     |         `- fraction (9 bits, 512 values)
 * ||              | |     `- value (6 bits, 64 values)
 * ||              | `- high word bit (0 for low word)
 * ||              `- value (14 bits, 16384 values)
 * |`- sign bit
 * `- high word bit (1 for high word)
 *
 * This format can hold up to 1048576 Neverball units of 1/512 precision.
 *
 * Due to the nature of Neverball replays, the high word changes relatively
 * infrequently, which allows for a simple compression trick: we can omit the
 * high word from the data stream unless its value changes. That's why we
 * reserve the first bit to differentiate high word from low word.
 */
uint32_t tofixed(const float value)
{
	// A little trick to simplify calculation: divide by 64 because the low
	// word can hold 64 units. This maps the fraction to the entire low word,
	// just for the purposes of this conversion.
	const float scaled = value / 64.0f;
	float integer = 0.0f;
	float fraction = modff(scaled, &integer);

	uint16_t high = 1 << 15 | (signbit(value) ? 1 : 0) << 14 | (((uint16_t) fabsf(integer)) & 0x3fff);
	uint16_t low = ((uint16_t) fabsf(32768.0f * fraction)) & 0x7fff;

	return high << 16 | low;
}

float fromfixed(const uint32_t fixed)
{
	uint16_t high = (fixed >> 16) & 0xffff;
	uint16_t low = fixed & 0xffff;

	float integer = (float) (high & 0x3fff);
	float fraction = (float) (low & 0x7fff) / 32768.0f;

	float value = (integer + fraction) * 64.0f * ((high & (1 << 14)) ? -1.0f : +1.0f);

	return value;
}
