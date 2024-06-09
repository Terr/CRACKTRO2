#include <dos.h>

#include "vga.h"

void set_mode(unsigned char mode)
{
    union REGS regs;
    regs.h.ah = SET_MODE;
    regs.h.al = mode;
    int86(VIDEO_INT, &regs, &regs);
}

/* Unchain memory to enable planar addressing, aka Mode Y */
void unchain_vga(unsigned char far *VGA)
{
    unsigned short i;
    unsigned long *ptr=(unsigned long *)VGA;            /* used for faster screen clearing */

    outp(SC_INDEX, MEMORY_MODE);       /* turn off chain-4 mode */
    outp(SC_DATA, 0x06);

    /* Reset clock to 25MHz @ 60Hz, not stricly necessary but Abrash does it in his Mode X example */
    outp(MISC_OUTPUT, 0xE3);

    /* Undo reset(?) */
    outp(SC_INDEX, 0x00);
    outp(SC_DATA, 0x03);

    /* outpw() is not available in Turbo C */
    outport(SC_INDEX, ALL_PLANES);      /* set map mask to all 4 planes */

    for(i = 0; i < 0x8000; ++i) {       /* clear all 256K of memory */
                                        /* TODO is it actually 256k? 0x8000 * 4 = 128k */
        *ptr++ = 0;
    }

    outp(CRTC_INDEX, UNDERLINE_LOCATION);/* turn off long mode (aka dword mode) */
    outp(CRTC_DATA, 0x00);

    outp(CRTC_INDEX, MODE_CONTROL);      /* turn on byte mode */
    outp(CRTC_DATA, 0xE3);
}

void set_mode_x(void)
{
    char vsync_end = 0;
    /* Remove write protect bits on various VGA registers */
    outp(CRTC_INDEX, 0x11);
    vsync_end = inp(CRTC_DATA) & 0x7f;
    outp(CRTC_DATA, vsync_end);

    outp(CRTC_INDEX, V_RETRACE_END);
    outp(CRTC_DATA, 0x2c);

    outp(CRTC_INDEX, V_TOTAL);
    outp(CRTC_DATA, 0x0D);

    outp(CRTC_INDEX, OVERFLOW);
    outp(CRTC_DATA, 0x3E);

    outp(CRTC_INDEX, MAX_SCAN_LINE); /* "Cell height" */
    outp(CRTC_DATA, 0x41);

    outp(CRTC_INDEX, V_RETRACE_START);
    outp(CRTC_DATA, 0xEA);

    outp(CRTC_INDEX, V_RETRACE_END);
    outp(CRTC_DATA, 0xAC);

    outp(CRTC_INDEX, V_DISPLAY_END);
    outp(CRTC_DATA, 0xDF);

    outp(CRTC_INDEX, V_BLANK_START);
    outp(CRTC_DATA, 0xE7);

    outp(CRTC_INDEX, V_BLANK_END);
    outp(CRTC_DATA, 0x06);

    /* Set write protect back */
    outp(CRTC_INDEX, 0x11);
    vsync_end = inp(CRTC_DATA) & 0x80;
    outp(CRTC_DATA, vsync_end);
}
