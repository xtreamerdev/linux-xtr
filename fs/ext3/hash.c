/*
 *  linux/fs/ext3/hash.c
 *
 * Copyright (C) 2002 by Theodore Ts'o
 *
 * This file is released under the GPL v2.
 * 
 * This file may be redistributed under the terms of the GNU Public
 * License.
 */

#include <linux/fs.h>
#include <linux/jbd.h>
#include <linux/sched.h>
#include <linux/ext3_fs.h>

#define DELTA 0x9E3779B9

static void TEA_transform(__u32 buf[4], __u32 const in[])
{
	__u32	sum = 0;
	__u32	b0 = buf[0], b1 = buf[1];
	__u32	a = in[0], b = in[1], c = in[2], d = in[3];
	int	n = 16;

	do {							
		sum += DELTA;					
		b0 += ((b1 << 4)+a) ^ (b1+sum) ^ ((b1 >> 5)+b);	
		b1 += ((b0 << 4)+c) ^ (b0+sum) ^ ((b0 >> 5)+d);	
	} while(--n);

	buf[0] += b0;
	buf[1] += b1;
}

/* F, G and H are basic MD4 functions: selection, majority, parity */
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) (((x) & (y)) + (((x) ^ (y)) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

/*
 * The generic round function.  The application is so specific that
 * we don't bother protecting all the arguments with parens, as is generally
 * good macro practice, in favor of extra legibility.
 * Rotation is separate from addition to prevent recomputation
 */
#define ROUND(f, a, b, c, d, x, s)	\
	(a += f(b, c, d) + x, a = (a << s) | (a >> (32-s)))
#define K1 0
#define K2 013240474631UL
#define K3 015666365641UL

/*
 * Basic cut-down MD4 transform.  Returns only 32 bits of result.
 */
static void halfMD4Transform (__u32 buf[4], __u32 const in[])
{
	__u32	a = buf[0], b = buf[1], c = buf[2], d = buf[3];

	/* Round 1 */
	ROUND(F, a, b, c, d, in[0] + K1,  3);
	ROUND(F, d, a, b, c, in[1] + K1,  7);
	ROUND(F, c, d, a, b, in[2] + K1, 11);
	ROUND(F, b, c, d, a, in[3] + K1, 19);
	ROUND(F, a, b, c, d, in[4] + K1,  3);
	ROUND(F, d, a, b, c, in[5] + K1,  7);
	ROUND(F, c, d, a, b, in[6] + K1, 11);
	ROUND(F, b, c, d, a, in[7] + K1, 19);

	/* Round 2 */
	ROUND(G, a, b, c, d, in[1] + K2,  3);
	ROUND(G, d, a, b, c, in[3] + K2,  5);
	ROUND(G, c, d, a, b, in[5] + K2,  9);
	ROUND(G, b, c, d, a, in[7] + K2, 13);
	ROUND(G, a, b, c, d, in[0] + K2,  3);
	ROUND(G, d, a, b, c, in[2] + K2,  5);
	ROUND(G, c, d, a, b, in[4] + K2,  9);
	ROUND(G, b, c, d, a, in[6] + K2, 13);

	/* Round 3 */
	ROUND(H, a, b, c, d, in[3] + K3,  3);
	ROUND(H, d, a, b, c, in[7] + K3,  9);
	ROUND(H, c, d, a, b, in[2] + K3, 11);
	ROUND(H, b, c, d, a, in[6] + K3, 15);
	ROUND(H, a, b, c, d, in[1] + K3,  3);
	ROUND(H, d, a, b, c, in[5] + K3,  9);
	ROUND(H, c, d, a, b, in[0] + K3, 11);
	ROUND(H, b, c, d, a, in[4] + K3, 15);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

#undef ROUND
#undef F
#undef G
#undef H
#undef K1
#undef K2
#undef K3

/* The old legacy hash */
static __u32 dx_hack_hash (const char *name, int len)
{
	__u32 hash0 = 0x12a3fe2d, hash1 = 0x37abe8f9;
	while (len--) {
		__u32 hash = hash1 + (hash0 ^ (*name++ * 7152373));
		
		if (hash & 0x80000000) hash -= 0x7fffffff;
		hash1 = hash0;
		hash0 = hash;
	}
	return (hash0 << 1);
}

static void str2hashbuf(const char *msg, int len, __u32 *buf, int num)
{
	__u32	pad, val;
	int	i;

	pad = (__u32)len | ((__u32)len << 8);
	pad |= pad << 16;

	val = pad;
	if (len > num*4)
		len = num * 4;
	for (i=0; i < len; i++) {
		if ((i % 4) == 0)
			val = pad;
		val = msg[i] + (val << 8);
		if ((i % 4) == 3) {
			*buf++ = val;
			val = pad;
			num--;
		}
	}
	if (--num >= 0)
		*buf++ = val;
	while (--num >= 0)
		*buf++ = pad;
}

/*
 * Returns the hash of a filename.  If len is 0 and name is NULL, then
 * this function can be used to test whether or not a hash version is
 * supported.
 * 
 * The seed is an 4 longword (32 bits) "secret" which can be used to
 * uniquify a hash.  If the seed is all zero's, then some default seed
 * may be used.
 * 
 * A particular hash version specifies whether or not the seed is
 * represented, and whether or not the returned hash is 32 bits or 64
 * bits.  32 bit hashes will return 0 for the minor hash.
 */
int ext3fs_dirhash(const char *name, int len, struct dx_hash_info *hinfo)
{
	__u32	hash;
	__u32	minor_hash = 0;
	const char	*p;
	int		i;
	__u32 		in[8], buf[4];

	/* Initialize the default seed for the hash checksum functions */
	buf[0] = 0x67452301;
	buf[1] = 0xefcdab89;
	buf[2] = 0x98badcfe;
	buf[3] = 0x10325476;

	/* Check to see if the seed is all zero's */
	if (hinfo->seed) {
		for (i=0; i < 4; i++) {
			if (hinfo->seed[i])
				break;
		}
		if (i < 4)
			memcpy(buf, hinfo->seed, sizeof(buf));
	}
		
	switch (hinfo->hash_version) {
	case DX_HASH_LEGACY:
		hash = dx_hack_hash(name, len);
		break;
	case DX_HASH_HALF_MD4:
		p = name;
		while (len > 0) {
			str2hashbuf(p, len, in, 8);
			halfMD4Transform(buf, in);
			len -= 32;
			p += 32;
		}
		minor_hash = buf[2];
		hash = buf[1];
		break;
	case DX_HASH_TEA:
		p = name;
		while (len > 0) {
			str2hashbuf(p, len, in, 4);
			TEA_transform(buf, in);
			len -= 16;
			p += 16;
		}
		hash = buf[0];
		minor_hash = buf[1];
		break;
	default:
		hinfo->hash = 0;
		return -1;
	}
	hinfo->hash = hash & ~1;
	hinfo->minor_hash = minor_hash;
	return 0;
}
