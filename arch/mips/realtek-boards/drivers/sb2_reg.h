#ifndef SB2_REG_H
#define SB2_REG_H

#ifdef SIMULATOR
#else
#define SB2_REG_BASE 0xB800A000
#define SB2_TILE_ACCESS_SET ((volatile unsigned int *)0xB801A01C)
#endif

#define SB2_REG(r) (r)

#define PrepareTileStartAddrWriteData(a)  (((((a) >> 11) & 0xFFFF) << 11) | (1 << 27))
#define PreparePicWidthWriteData(w)  ((((w) & 0x7) << 7) | (1 << 10))
#define PreparePicIndexWriteData(i)  ((((i) & 0x3F) << 0) | (1 << 6))

#endif

