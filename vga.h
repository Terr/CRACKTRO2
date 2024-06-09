#ifndef VGA_H
#define VGA_H

#define SET_MODE 0x00
#define VIDEO_INT 0x10
#define VGA_256_COLOR_MODE 0x13
#define TEXT_MODE 0x03
#define INPUT_STATUS 0x03da

#define SC_INDEX            0x03c4    /* VGA sequence controller */
#define SC_DATA             0x03c5
#define PALETTE_MASK_REG    0x03C6
#define PALETTE_READ_INDEX  0x03C7
#define PALETTE_WRITE_INDEX 0x03C8
#define PALETTE_COLORS      0x03C9
#define GC_INDEX            0x03ce    /* VGA graphics controller */
#define GC_DATA             0x03cf
#define CRTC_INDEX          0x03d4    /* VGA CRT controller */
#define CRTC_DATA           0x03d5
#define MISC_OUTPUT         0x03c2

#define MAP_MASK            0x02      /* Sequence controller registers */
#define ALL_PLANES          0xff02
#define MEMORY_MODE         0x04

#define LATCHES_ON          0x0008    /* Graphics controller registers */
#define LATCHES_OFF         0xff08

#define HIGH_ADDRESS        0x0C
#define LOW_ADDRESS         0x0D

#define V_TOTAL             0x06      /* CRT controller registers */
#define OVERFLOW            0x07
#define VRETRACE            0x08
#define MAX_SCAN_LINE       0x09
#define V_RETRACE_START     0x10
#define V_RETRACE_END       0x11
#define V_DISPLAY_END       0x12
#define UNDERLINE_LOCATION  0x14
#define V_BLANK_START       0x15
#define V_BLANK_END         0x16
#define MODE_CONTROL        0x17

#define DISPLAY_ENABLE      0x01      /* VGA input status bits */

void set_mode(unsigned char mode);
void unchain_vga(unsigned char far *VGA);
void set_mode_x(void);

#endif
