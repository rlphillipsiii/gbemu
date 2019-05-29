/*
 * memmap.h
 *
 *  Created on: May 26, 2019
 *      Author: Robert Phillips III
 */

#ifndef MEMMAP_H_
#define MEMMAP_H_

#define BIOS_SIZE    256
#define BIOS_OFFSET 0x0000

#define ROM_0_SIZE   16384
#define ROM_0_OFFSET 0x0000

#define ROM_1_SIZE   ROM_0_SIZE
#define ROM_1_OFFSET 0x4000

#define GPU_RAM_SIZE   8192
#define GPU_RAM_OFFSET 0x8000

#define EXT_RAM_SIZE   8192
#define EXT_RAM_OFFSET 0xA000

#define WORKING_RAM_SIZE   8192
#define WORKING_RAM_OFFSET 0xC000

#define GRAPHICS_RAM_SIZE   160
#define GRAPHICS_RAM_OFFSET 0xFE00

#define IO_SIZE   128
#define IO_OFFSET 0xFF00

#define ZRAM_SIZE   128
#define ZRAM_OFFSET 0xFF80

#endif /* SRC_HARDWARE_MEMMAP_H_ */
