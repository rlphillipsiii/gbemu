/*
 * memmap.h
 *
 *  Created on: May 26, 2019
 *      Author: Robert Phillips III
 */

#ifndef MEMMAP_H_
#define MEMMAP_H_

#define BIOS_SIZE   256
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

#define INTERRUPT_FLAGS_ADDRESS 0xFF0F
#define INTERRUPT_MASK_ADDRESS  0xFFFF

#define SOUND_CONTROLLER_CHANNEL 0xFF24
#define SOUND_CONTROLLER_OUTPUT  0xFF25
#define SOUND_CONTROLLER_ENABLE  0xFF26

#define SOUND_CONTROLLER_CH1_SWEEP    0xFF10
#define SOUND_CONTROLLER_CH1_PATTERN  0xFF11
#define SOUND_CONTROLLER_CH1_ENVELOPE 0xFF12
#define SOUND_CONTROLLER_CH1_FREQ_LO  0xFF13
#define SOUND_CONTROLLER_CH1_FREQ_HI  0xFF14
#define SOUND_CONTROLLER_CH2_PATTERN  0xFF16
#define SOUND_CONTROLLER_CH2_ENVELOPE 0xFF17
#define SOUND_CONTROLLER_CH2_FREQ_LO  0xFF18
#define SOUND_CONTROLLER_CH2_FREQ_HI  0xFF19
#define SOUND_CONTROLLER_CH3_ENABLE   0xFF1A
#define SOUND_CONTROLLER_CH3_LENGTH   0xFF1B
#define SOUND_CONTROLLER_CH3_LEVEL    0xFF1C
#define SOUND_CONTROLLER_CH3_FREQ_LO  0xFF1D
#define SOUND_CONTROLLER_CH3_FREQ_HI  0xFF1E
#define SOUND_CONTROLLER_CH3_ARB      0xFF30

#define SOUND_CONTROLLER_ARB_BYTES 16

#define GPU_CONTROL_ADDRESS  0xFF40
#define GPU_STATUS_ADDRESS   0xFF41
#define GPU_SCROLLX_ADDRESS  0xFF42
#define GPU_SCROLLY_ADDRESS  0xFF43
#define GPU_SCANLINE_ADDRESS 0xFF44
#define GPU_PALETTE_ADDRESS  0xFF47
#define GPU_OBP1_ADDRESS     0xFF48
#define GPU_OBP2_ADDRESS     0xFF49

#define GPU_SPRITE_TABLE_ADDRESS 0xFE00

#endif /* MEMMAP_H_ */
