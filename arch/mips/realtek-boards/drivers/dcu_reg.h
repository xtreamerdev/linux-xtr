#ifndef DCU_REG_H
#define DCU_REG_H


#ifdef SIMULATOR
#else
#define DCU_REG_BASE 0xB8008000
#define DC_SYS_MISC ((volatile unsigned int *)0xB8008004)
#define DC_PICT_SET ((volatile unsigned int *)0xB8008000)
#define DC_PICT_SET_OFFSET ((volatile unsigned int *)0xB8008034)
#endif

#define DCU_REG(r) (r)

#define PreparePictSetWriteData(index, pitch, addr)  ((((index) & 0x3F) << 18) | ((((pitch) >> 8) & 0x1F) << 13) | (((addr) >> (11+((readl(DCU_REG(DC_SYS_MISC)) >> 16) & 0x3))) & 0x1FFF))

#define DC_PICT_SET_OFFSET_pict_set_num(data)                         (0x0000FC00&((data)<<10))
#define DC_PICT_SET_OFFSET_pict_set_offset_x(data)                    (0x000003FF&(data))
#define DC_PICT_SET_OFFSET_pict_set_offset_y(data)                    (0x03FF0000&((data)<<16))

#endif

