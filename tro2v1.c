/* Needs to be compiled with the Large memory model (-ml) */

#include <dos.h>
#include <mem.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "letters.h"

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;

#define PLAY_MUSIC
/*#define USE_TIMER*/
/* The "ASM palette swap" method using interrupts to copy the palette is slightly faster but does not work real
 * hardware, or at least not on a C&T 65555 PCI chipset.
 *
 * On that chipset the screen flickers a bit but it's doesn't get to displaying the full image.
 */
#define USE_ASM_PALETTE_SWAP

#ifdef PLAY_MUSIC
void extern PreparePlayer(void);
void extern StopPlayer(void);
/*void extern InitPlayer();*/
/*void extern EndPlayer();*/
/*void extern PlayMusic();*/
#endif

/*void extern runGame();*/

#ifdef USE_TIMER
void extern ZTimerOn(void);
void extern ZTimerOff(void);
void extern ZTimerReport(void);
#endif

/* VGA video memory segment */
byte far *VGA=(byte far *)0xA0000000L;
/*byte far *VGA=(byte far *)0xA0000;*/
/*byte *VGA=(byte *)0xA0000000L;*/
/* 18.2Hz system clock */
word *my_clock=(word *)0x0000046C;

#define SET_MODE 0x00
#define VIDEO_INT 0x10
#define KEYBOARD_INT 0x9
#define VGA_256_COLOR_MODE 0x13
#define TEXT_MODE 0x03
#define INPUT_STATUS 0x03da
#define VRETRACE 0x08
#define NUM_COLORS 256
/*#define TEXT_PALETTE_SIZE 90*/
/*#define TEXT_PALETTE_COLORS 270*/
/*#define TEXT_PALETTE_SIZE 64*/
/*#define TEXT_PALETTE_ANGLE 6*/
#define TEXT_PALETTE_SIZE 64
#define TEXT_PALETTE_ANGLE 6
#define BLACK_PALETTE_SIZE 63
#define STARS_PALETTE_SIZE 64
/* 2 * TEXT_PALETTE_SIZE * 3 */
#define TEXT_PALETTE_COLORS 384

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
#define MAX_SCAN_LINE       0x09
#define V_RETRACE_START     0x10
#define V_RETRACE_END       0x11
#define V_DISPLAY_END       0x12
#define UNDERLINE_LOCATION  0x14
#define V_BLANK_START       0x15
#define V_BLANK_END         0x16
#define MODE_CONTROL        0x17

#define DISPLAY_ENABLE      0x01      /* VGA input status bits */

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define NUM_PIXELS 76800
#define SCREEN_DEPTH 64
#define PLANE_WIDTH 80

#define PI 3.14159
#define SINTABLE_SIZE 256
#define ZTABLE_FIXED_FRAC 6

#define SETPIX(x,y,c) *(VGA+(x)+(y<<8)+(y<<6))=c
#define GETPIX(x,y) *(VGA+(x)++(y<<8)+(y<<6))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#define NUM_LETTERS 13
#define NUM_STARS 69
/*#define NUM_STARS 1*/
#define STAR_SPEED 1
#define BITMAP_WIDTH 1080
#define BITMAP_HEIGHT 24
#define LETTER_SCROLL_SPEED 2
#define LETTER_WIDTH 24
#define LETTER_HEIGHT 24
#define LETTER_SPACE 1056
#define LETTER_PADDING 4
#define TEXT_Y_OFFSET 20
/*#define WIGGLE 10*/
#define WIGGLE 70

#define REFLECTION_ROWS 40
/*#define REFLECTION_ROW_STEP 6 * PLANE_WIDTH*/
#define REFLECTION_ROW_STEP 320
/*#define REFLECTION_SOURCE_START (SCREEN_HEIGHT - REFLECTION_ROWS - 21) * PLANE_WIDTH*/
#define REFLECTION_SOURCE_START 14960
/*#define REFLECTION_DESTINATION_START (SCREEN_HEIGHT - REFLECTION_ROWS - 20) * PLANE_WIDTH*/
#define REFLECTION_DESTINATION_START 15040

/* (Screen height - reflection rows - margin) * pixels per plane */
/*#define UPPER_AREA_PLANE_PIXELS (SCREEN_HEIGHT - REFLECTION_ROWS - 20) * PLANE_WIDTH*/
#define UPPER_AREA_PLANE_PIXELS 15040
/* (Screen height - (Screen height - reflection rows - margin)) * pixels per plane */
#define REFLECTION_AREA_PLANE_PIXELS 4160

typedef struct             /* the structure for a bitmap. */
{
  word width;
  word height;
  byte *data;
} BITMAP;

typedef struct
{
    short x;
    short letter_offset;
} SPRITE;

typedef struct
{
    short x;
    short y;
    short z;
    byte c;
} STAR;

/*char text[] =   "                 NOSTALGIA PROUDLY PRESENTZ: SIMCITY V1.07 +1 BY MAXIS!        CRACKED BY: LOADHIGH   TRAINED BY: LOADHIGH        ONLY NOSTALGIA CAN BRING YOU THE FINEST 12814-DAY RELEASEZ!        GREETZ TO ALL THE ELITEZ: REALITY TTE H0FFMAN ROOT42 TWINBEARD BRACKEEN AND EVERYONE ELSE WHO DESIREZ A GREET!        AND NOW FOR THE CRACKTRO CREDITZ:   MUSIX: MARVIN   RAD LIB: REALITY   CODE/GRAFIX: LOADHIGH          MMXXIV  ";*/
char text[] = "         OWO WHATS THIS# ANOTHER NOSTALGIA RELEASE# YOU BET!        NOSTALGIA PROUDLY PRESENTZ: SID MEIERS CIVILIZATION V474.05 +4 BY MICROPROSE!        CRACKED BY: LOADHIGH   TRAINED BY: LOADHIGH        ONLY NOSTALGIA CAN BRING YOU THE FINEST 12814-DAY RELEASEZ!        GREETZ FLY OUT TO ALL THE ELITEZ: REALITY TTE H0FFMAN ROOT42 ABRASH BRACKEEN QBMIKEHAWK AND EVERYONE ELSE WHO DESIREZ A GREET!        CRACKTRO CREDITZ:   MUSIX: MARVIN   RAD LIB: REALITY   CODE/GRAFIX: LOADHIGH          HACK THE PLANET!!!            MMXXIV  ";
/*char text[] = "         ABCDEFGHIJKLMNOPQRSTUVWXYZ";*/
/*char text[] = " BRBRBRBRBRBR HERE COMES THE CHINESE EARTHQUAKE";*/
/*char text[] = "1 2 3 4 5 6 7 8 9 0";*/
/*char text[] = "ACAB ACAB ACAB ACAB ACAB ACAB ";*/
/*char text[] = "ACAB ACAB ";*/
/*char text[] = "ACAB ";*/
/*char text[] = "  #### ????    ,./?!  ACAB";*/

/*static byte global_sin_index = 0;*/
static int frame_counter = 0;

void set_mode(byte mode)
{
    union REGS regs;
    regs.h.ah = SET_MODE;
    regs.h.al = mode;
    int86(VIDEO_INT, &regs, &regs);
}

/* Unchain memory to enable planar addressing, aka Mode Y */
void unchain_vga(void)
{
    word i;
    dword *ptr=(dword *)VGA;            /* used for faster screen clearing */

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
    byte vsync_end = 0;
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

void unrle(byte* compressed, BITMAP *bmp) {
    unsigned int i, j;
    unsigned int uncompressed_index = 0;
    byte dup_byte, dup_count;

    bmp->data = (byte *)malloc(LETTERS_UNCOMPRESSED);
    if (bmp->data == NULL) {
        printf("Error allocating memory for data.\n");
        exit(1);
    }

    for (i = 0; i < (unsigned int)LETTERS_COMPRESSED; i = i + 2) {
        dup_count = compressed[i];
        dup_byte = compressed[i + 1];

        for (j = 0; j < dup_count; ++j) {
            bmp->data[uncompressed_index] = dup_byte;
            uncompressed_index++;
        }
    }
}

void wait(int ticks)
{
    word start = *my_clock;

    while (*my_clock-start<ticks)
    {
        *my_clock=*my_clock;              /* this line is for some compilers
                                             that would otherwise ignore this
                                             loop */
    }
}

/*
void draw_letter(BITMAP *bmp, short letter_offset, byte *screen_buffer, int x,int y, word skip)
{
  int j;
  word screen_offset = (y<<8)+(y<<6)+x;
  word bitmap_offset = letter_offset + skip;

  for(j = 0; j < LETTER_HEIGHT; ++j)
  {
    memcpy(&screen_buffer[screen_offset], &bmp->data[bitmap_offset], LETTER_WIDTH - skip);

    bitmap_offset += bmp->width;
    screen_offset += SCREEN_WIDTH;
  }
}
*/

/*
void draw_letter_width(BITMAP *bmp, short letter_offset, byte *screen_buffer, int x,int y, word width)
{
  int j;
  word screen_offset = (y<<8)+(y<<6)+x;
  word bitmap_offset = letter_offset;

  for(j = 0; j < LETTER_HEIGHT; ++j)
  {
    memcpy(&screen_buffer[screen_offset], &bmp->data[bitmap_offset], width);

    bitmap_offset += bmp->width;
    screen_offset += SCREEN_WIDTH;
  }
}
*/

void wait_for_retrace(void)
{
    /* wait until done with vertical retrace */
    while  ((inp(INPUT_STATUS) & VRETRACE)) {};
    /* wait until done refreshing */
    while (!(inp(INPUT_STATUS) & VRETRACE)) {};
}

void set_palette(void) {
    byte red, green, blue;
    short angle;
    int i = 0;

    /* Set palette */
    outp(PALETTE_MASK_REG, 0xFF);
    outp(PALETTE_WRITE_INDEX, 0);

    /* Black (index 0) */
    outp(PALETTE_COLORS, 0); /* R */
    outp(PALETTE_COLORS, 0); /* G */
    outp(PALETTE_COLORS, 0); /* B */

    /* Rainbow palette (index 1-63) */
    /*for (angle = 0, i = 0; i < TEXT_PALETTE_SIZE; i++,angle += 5) {*/
    for (angle = 0, i = 0; i < TEXT_PALETTE_SIZE - 1; i++, angle += TEXT_PALETTE_ANGLE) {
        if (angle<60) {red = 255; green = ceil(angle*4.25-0.01); blue = 0;} else
        if (angle<120) {red = ceil((120-angle)*4.25-0.01); green = 255; blue = 0;} else
        if (angle<180) {red = 0, green = 255; blue = ceil((angle-120)*4.25-0.01);} else
        if (angle<240) {red = 0, green = ceil((240-angle)*4.25-0.01); blue = 255;} else
        if (angle<300) {red = ceil((angle-240)*4.25-0.01), green = 0; blue = 255;} else
                        {red = 255, green = 0; blue = ceil((360-angle)*4.25-0.01);}

        outp(PALETTE_COLORS, red >> 2); /* R to 18-bit */
        outp(PALETTE_COLORS, green >> 2); /* G */
        outp(PALETTE_COLORS, blue >> 2); /* B */
    }

    /* Dark blue (index 64) */
    outp(PALETTE_COLORS, 0); /* R */
    outp(PALETTE_COLORS, 0); /* G */
    outp(PALETTE_COLORS, 150); /* B */

    /* Water-tinted rainbow palette (index 65-128) */
    for (angle = 0, i = 0; i < TEXT_PALETTE_SIZE - 1; i++, angle += TEXT_PALETTE_ANGLE) {
        /*if (angle<60) {red = 255; green = ceil(angle*4.25-0.01); blue = 30;} else*/
        /*if (angle<120) {red = ceil((120-angle)*4.25-0.01); green = 255; blue = 60;} else*/
        if (angle<120) {red = 0, green = ceil((120-angle)*4.25-0.01); blue = 200;} else
        if (angle<180) {red = 0, green = ceil((240-angle)*4.25-0.01); blue = 210;} else
        if (angle<240) {red = 0, green = ceil((240-angle)*4.25-0.01); blue = 255;} else
        /*if (angle<240) {red = ceil((120-angle)*4.25-0.01); green = ceil((120-angle)*4.25-0.01); blue = ceil((120-angle)*4.25-0.01);} else*/
                        {red = 0, green = 0; blue = ceil((360-angle)*4.25-0.01);}

        outp(PALETTE_COLORS, red >> 2); /* R to 18-bit */
        outp(PALETTE_COLORS, green >> 2); /* G */
        outp(PALETTE_COLORS, blue >> 2); /* B */
    }

    /* More black (index 128-...)  */
    /*for (i = 0; i < BLACK_PALETTE_SIZE; ++i) {*/
        /*outp(PALETTE_COLORS, 0);*/
        /*outp(PALETTE_COLORS, 0);*/
        /*outp(PALETTE_COLORS, 0);*/
    /*}*/

    /* Grey tones - 128, used for stars */
    /*for (i = 0; i < STARS_PALETTE_SIZE; ++i) {*/
        /*outp(PALETTE_COLORS, i);*/
        /*outp(PALETTE_COLORS, i);*/
        /*outp(PALETTE_COLORS, i);*/
    /*}*/
}

void calculate_sintable(short *table, int table_size)
{
    int i;
    for (i = 0; i < table_size; ++i) {
        /*table[i] = TEXT_Y_OFFSET + WIGGLE * sin((float) ((PI / 256) * i) * 8);*/
        /*table[i] = TEXT_Y_OFFSET + WIGGLE * sin((float) ((PI / 512) * i) * 8);*/

        /*table[i] = TEXT_Y_OFFSET + WIGGLE * sin((float) ((PI / 512) * i) * 4);*/
        table[i] = TEXT_Y_OFFSET + WIGGLE * (1 + (sin(((PI / 512) * i) * 4)) / 1);
        table[i] = ((table[i]) << 6) +  ((table[i]) << 4);
    }
}

void calculate_ztable(short *table, int table_size) {
    int i;
    float dividend = (float)SCREEN_DEPTH;
    table[0] = 0.0;
    for (i = 1; i < table_size; ++i) {
        /*table[i] = (dividend / (i * 1)) * (1 << ZTABLE_FIXED_FRAC);*/
        table[i] = (dividend / (i * 1));
    }
}

void calculate_distortion_table(short *table, int table_size) {
    int i;
    for (i = 0; i < table_size; ++i) {
        /*table[i] = (i % 12) >> 2;*/
        table[i] = (i % 10) >> 2;
    }
}

void flip_pages(word *visible_page, word *non_visible_page) {
    word temp = *visible_page;
    *visible_page = *non_visible_page;
    *non_visible_page = temp;
}

void mainloop(BITMAP bmp, short *sintable, short *ztable, short *distortion_table) {
    short x = 0, y = 0;
    short start_x;
    short rx;
    short r_from, r_to;
    short i;
    byte j = 0, ri = 0;
    byte plane = 0;
    word ci = 0;
    byte cy = 0;
    byte color_offset = 0, alt_color_offset = 0;
    short text_index = 0;
    byte distortion;
    byte distortion_plane;
    SPRITE letters[NUM_LETTERS];
    SPRITE letter;
    STAR stars[NUM_STARS];
    STAR star;
    word screen_offset, bitmap_offset;
    word copy_source, copy_destination;
    /*byte r, g, b;*/
    byte palette[TEXT_PALETTE_COLORS];
    byte alt_palette[TEXT_PALETTE_COLORS];
    /* VGA pages */
    word temp = 0;
    word visible_page = 0;
    word non_visible_page = NUM_PIXELS / 4;
    word high_address, low_address;
    /*FILE *log = fopen("debug.log", "w");*/
#ifdef USE_ASM_PALETTE_SWAP
    word palette_seg = FP_SEG(palette);
    word palette_off = FP_OFF(palette);
    word alt_palette_seg = FP_SEG(alt_palette);
    word alt_palette_off = FP_OFF(alt_palette);
#endif

    /* Initial letters */
    for (ri = 0; ri < NUM_LETTERS; ++ri) {
        letters[ri].x = (short)((LETTER_WIDTH + LETTER_PADDING) * ri);

        if (text[text_index] >= 65) {
            letters[ri].letter_offset = (short)((text[text_index] - 65) << 4) + (short)((text[text_index] - 65) << 3);
        } else if (text[text_index] >= 48) {
            /* Digits + colon */
            letters[ri].letter_offset = (short)((text[text_index] - 22) << 4) + ((text[text_index] - 22) << 3);
        } else if (text[text_index] == 33) {
            /* Exclamation mark */
            letters[ri].letter_offset = 888;
        } else if (text[text_index] == 43) {
            /* Plus sign */
            letters[ri].letter_offset = 912;
        } else if (text[text_index] == 46) {
            /* Dot */
            letters[ri].letter_offset = 936;
        } else if (text[text_index] == 45) {
            /* Minus sign */
            letters[ri].letter_offset = 960;
        } else if (text[text_index] == 47) {
            /* Slash */
            letters[ri].letter_offset = 984;
        } else if (text[text_index] == 44) {
            /* Comma */
            letters[ri].letter_offset = 1008;
        } else if (text[text_index] == 35) {
            /* Question mark, but ASCII code of '?' isn't recognize so use a '#' instead */
            letters[ri].letter_offset = 1032;
        } else {
            /* Space */
            letters[ri].letter_offset = LETTER_SPACE;
        }
        /*letters[ri].global_sin_index = ri * 2;*/
        /*letters[ri].global_sin_index = ri;*/
        text_index++;
    }

    /* Initial stars */
    /*
    randomize();
    for (ri = 0; ri < NUM_STARS; ++ri) {
        stars[ri].x = random(PLANE_WIDTH - 1);
        stars[ri].y = random(SCREEN_HEIGHT - 1);
        stars[ri].z = random(SCREEN_DEPTH - 2);
        stars[ri].c = 255 - stars[ri].z;
    }
    */

    /* Store palette for cycling */
    disable();
    outp(PALETTE_READ_INDEX, 0);
    for (ci = 0; ci < TEXT_PALETTE_COLORS; ++ci) {
        palette[ci] = inp(PALETTE_COLORS);
    }

    outp(PALETTE_READ_INDEX, TEXT_PALETTE_SIZE);
    for (ci = 0; ci < 3 * TEXT_PALETTE_SIZE; ++ci) {
        alt_palette[ci] = inp(PALETTE_COLORS);
    }
    outp(PALETTE_READ_INDEX, 0);
    for (ci = 3 * TEXT_PALETTE_SIZE; ci < TEXT_PALETTE_COLORS; ++ci) {
        alt_palette[ci] = inp(PALETTE_COLORS);
    }
    enable();

    outport(SC_INDEX, ALL_PLANES);
    disable();

    while (1) {
        #ifdef USE_TIMER
        ZTimerOn();
        #endif

        /* Interrupts are still disabled at this point */
        /* Wait for start of next vertical trace(?) */
        /*while (!(inp(INPUT_STATUS) & VRETRACE));*/

#ifdef USE_ASM_PALETTE_SWAP
        if ((frame_counter & 1) == 0) {
            color_offset = TEXT_PALETTE_SIZE;
            alt_color_offset = 0;

            asm push ds
            asm push es

            asm mov ax, palette_seg
            asm mov es, ax
            asm mov dx, palette_off
            asm mov ax, 1012h
            asm sub bx, bx
            asm mov cx, 80h /* 128 colors */
            asm int 10h

            asm pop es
            asm pop ds
        } else {
            color_offset = 0;
            alt_color_offset = TEXT_PALETTE_SIZE;

            asm push ds
            asm push es

            asm mov ax, alt_palette_seg
            asm mov es, ax
            asm mov dx, alt_palette_off
            asm mov ax, 1012h
            asm sub bx, bx
            asm mov cx, 80h /* 128 colors */
            asm int 10h

            asm pop es
            asm pop ds
        }
#else
        /* Swap the two palette sets */
        if ((frame_counter & 1) == 0) {
            color_offset = TEXT_PALETTE_SIZE;
            alt_color_offset = 0;

            outp(PALETTE_WRITE_INDEX, 0);
            for (ci = 0; ci < 6 * TEXT_PALETTE_SIZE;) {
                outportb(PALETTE_COLORS, palette[ci++]);
                outportb(PALETTE_COLORS, palette[ci++]);
                outportb(PALETTE_COLORS, palette[ci++]);
            }
        } else {
            color_offset = 0;
            alt_color_offset = TEXT_PALETTE_SIZE;

            outp(PALETTE_WRITE_INDEX, 0);
            for (ci = 0; ci < 6 * TEXT_PALETTE_SIZE;) {
                outportb(PALETTE_COLORS, alt_palette[ci++]);
                outportb(PALETTE_COLORS, alt_palette[ci++]);
                outportb(PALETTE_COLORS, alt_palette[ci++]);
            }
        }
#endif

        enable();

        /* Read keyboard input */
        geninterrupt(KEYBOARD_INT);
        switch (inp(0x60)) {
            case 0x1:
                /*fclose(log);*/
                return;
        }

        /* Clear page */
        /* All planes should already be selected because of the latch copy at the end of the loop */
        /*outport(SC_INDEX, ALL_PLANES);*/
        memset(&VGA[non_visible_page], color_offset, UPPER_AREA_PLANE_PIXELS);
        /* Ensures that the reflection background is fully filled with the water color */
        memset(&VGA[non_visible_page+UPPER_AREA_PLANE_PIXELS], alt_color_offset, REFLECTION_AREA_PLANE_PIXELS);

        /* First update the position of the letters */
        for (ri = 0; ri < NUM_LETTERS; ++ri) {
            letter = letters[ri];

            letter.x -= LETTER_SCROLL_SPEED;

            if (letter.x <= -LETTER_WIDTH) {
                /* letter is completely offscreen */
                letter.x = LETTER_PADDING + (LETTER_WIDTH + LETTER_PADDING) * (NUM_LETTERS - 1);

                if (text[text_index] >= 65) {
                    letter.letter_offset = (short)((text[text_index] - 65) << 4) + (short)((text[text_index] - 65) << 3);
                } else if (text[text_index] >= 48) {
                    /* Digits + colon */
                    letter.letter_offset = (short)((text[text_index] - 22) << 4) + ((text[text_index] - 22) << 3);
                } else if (text[text_index] == 33) {
                    /* Exclamation mark */
                    letter.letter_offset = 888;
                } else if (text[text_index] == 43) {
                    /* Plus sign */
                    letter.letter_offset = 912;
                } else if (text[text_index] == 46) {
                    /* Dot */
                    letter.letter_offset = 936;
                } else if (text[text_index] == 45) {
                    /* Minus sign */
                    letter.letter_offset = 960;
                } else if (text[text_index] == 47) {
                    /* Slash */
                    letter.letter_offset = 984;
                } else if (text[text_index] == 44) {
                    /* Comma */
                    letter.letter_offset = 1008;
                } else if (text[text_index] == 35) {
                    /* Question mark, but ASCII code of '?' isn't recognize so use a '#' instead */
                    letter.letter_offset = 1032;
                } else {
                    /* Space */
                    letter.letter_offset = LETTER_SPACE;
                }

                text_index++;
            }

            letters[ri] = letter;

            if (text_index == sizeof(text) - 1) {
                text_index = 0;
            }
        }

        /* Calculate the new position of every star */
        /*for (ri = 0; ri < NUM_STARS; ++ri) {*/
            /*if (stars[ri].x >= PLANE_WIDTH) {*/
                /*[> Star went out of bounds so create a new one, and draw it again in the next frame <]*/
                /*[> The reason for not drawing it now has to do with the "star.z" division needs to be done, and doing it again for this new star would slow things down <]*/
                /*stars[ri].x = 0;*/
                /*stars[ri].y = random(SCREEN_HEIGHT - 1);*/
                /*stars[ri].z = random(SCREEN_DEPTH - 2);*/
                /*[>stars[ri].c = TEXT_PALETTE_SIZE + BLACK_PALETTE_SIZE;<]*/
                /*stars[ri].c = 255 - stars[ri].z;*/
            /*}*/

            /*[>stars[ri].x += ztable[stars[ri].z + 1] >> ZTABLE_FIXED_FRAC;<]*/
            /*stars[ri].x += ztable[stars[ri].z + 1];*/
        /*}*/

        /* TODO */
        /*flip_pages(&visible_page, &non_visible_page);*/
        /*memset(&VGA[visible_page], 0, UPPER_AREA_PLANE_PIXELS);*/

        /* Draw letters and stars */
        for (plane = 0; plane < 4; ++plane) {
        /*for (plane = 0; plane < 1; ++plane) {*/
            /* Select the next plane */
            /*outport(SC_INDEX, 0x0102);*/
            outport(SC_INDEX, ((1 << plane) << 8) + MAP_MASK);
            /*outp(SC_INDEX, MAP_MASK);*/
            /*outp(SC_DATA, 1 << plane);*/

            /*for (ri = 0; ri < NUM_STARS; ++ri) {*/
                /*screen_offset = non_visible_page + (stars[ri].y << 6) + (stars[ri].y << 4) + (stars[ri].x);*/
                /*[> TODO De sterren op een of andere manier indexeren zodat we precies weten welke er op welke plane getekend moeten worden <]*/
                /*if (screen_offset % 4 == plane) {*/
                    /*[>screen_offset = non_visible_page + (stars[ri].y >> 6) + (stars[ri].y >> 4) + (stars[ri].x >> 2);<]*/
                    /*[>screen_offset = non_visible_page + stars[ri].y + (stars[ri].x >> 2);<]*/

                    /*[>VGA[screen_offset] = stars[ri].c;<]*/
                /*}*/
            /*}*/

            for (ri = 0; ri < NUM_LETTERS; ++ri) {
                letter = letters[ri];

                /*start_x = ((letter.x / 4) * 4) - letter.x;*/
                start_x = (letter.x - (letter.x & 3)) - letter.x;

                /*fprintf(log, "%d -> %d\n", letter.x, start_x);*/

                if (letter.x < 0) {
                    /* Left side of the letter is offscreen */
                    r_from = (letter.x - (LETTER_WIDTH + letter.x) & 3) - letter.x;
                    r_to = LETTER_WIDTH;
                } else if (letter.x + LETTER_WIDTH > SCREEN_WIDTH) {
                    /* Right side of the letter is offscreen */
                    r_from = start_x;
                    r_to = LETTER_WIDTH - ((letter.x + LETTER_WIDTH) - SCREEN_WIDTH);
                } else {
                    /* Letter is completely on screen */
                    r_from = start_x;
                    r_to = LETTER_WIDTH;
                }

                if (r_from + plane < 0) {
                    r_from += 4;
                }

                for (i = r_from + plane; i < r_to; i = i + 4) {

                    /* This holds the column to draw on the currently selected plane */
                    rx = letter.x + i;

                    y = sintable[(byte)rx];
                    /*y = sintable[(byte)(global_sin_index + rx)];*/
                    /*y = 8000;*/
                    /* Breng de Y terug naar een kleinere waarde zodat we de Y-coordinaat als kleurcode kunnen gebruiken */
                    cy = 1 + ((y >> 8) + (y >> 8) >> 2) + color_offset;

                    bitmap_offset = letter.letter_offset + i;
                    screen_offset = non_visible_page + y + (rx >> 2);

                    for(j = 0; j < LETTER_HEIGHT; ++j) {
                        if (bmp.data[bitmap_offset] > 0) {
                            /* Deze zet de kleur 'vast' per Y coordinaat */
                            /*VGA[screen_offset] = ((cy + j) & 255);*/

                            VGA[screen_offset] = (cy + j);

                            /* Dit kleurt per plane */
                            /* Rood geel groen blauw */
                            /*VGA[screen_offset] = 1 + (plane*32) & 127;*/
                        }

                        screen_offset += PLANE_WIDTH;
                        bitmap_offset += BITMAP_WIDTH;
                    }
                }
            }
        }

        /* Copy pixels from the other page using latches */
        /* Tell the VGA that all writes are to be done with bits from the latches, and none from the CPU */
        outp(GC_INDEX, 0x08);
        outp(GC_DATA, 0x00);

        /* Select all planes */
        outp(SC_INDEX, MAP_MASK);
        outp(SC_DATA, 0xFF);

        copy_source = visible_page + REFLECTION_SOURCE_START;
        copy_destination = non_visible_page + REFLECTION_DESTINATION_START;

        for (y = 0; y < REFLECTION_ROWS; ++y) {
            distortion_plane = distortion_table[y];
            for (x = 0; x < PLANE_WIDTH - distortion_plane; ++x) {
                temp = VGA[copy_source + x + distortion_plane];
                VGA[copy_destination + x] = 0;
            }

            /* Skip 8 rows, compresses the reflected image into a smaller area,
             * as if looking at it from an angle */
            copy_source -= REFLECTION_ROW_STEP;
            copy_destination += PLANE_WIDTH;
        }

        /* Restore write behaviour: copy data from normal memory/CPU registers */
        outp(GC_DATA, 0xFF);
        /*wait_for_retrace();*/

        /*flip_pages(&visible_page, &non_visible_page);*/
        temp = visible_page;
        visible_page = non_visible_page;
        non_visible_page = temp;

        high_address = HIGH_ADDRESS | (visible_page & 0xFF00);
        low_address = LOW_ADDRESS | (visible_page << 8);

        #ifdef USE_TIMER
        ZTimerOff();
        #endif

        disable();
        /* Wait for end of current vertical trace(?) */
        while ((inp(INPUT_STATUS) & DISPLAY_ENABLE));
        outport(CRTC_INDEX, high_address);
        outport(CRTC_INDEX, low_address);
        /* Wait for start of next vertical trace(?) */
        while (!(inp(INPUT_STATUS) & VRETRACE));

        /*getch();*/

        /*++global_sin_index;*/
        ++frame_counter;

        /*break;*/
    }
}

void validate_checksum() {
    int i, j = 0;
    byte correct[] = {38, 6, 14, 29};
    byte checksum[4] = {0};
    /* "FUCKINGS TO LAMERS!!! " + 69 char shift */
    byte msg[] = {139, 154, 136, 144, 142, 147, 140, 152, 101, 153, 148, 101, 145, 134, 146, 138, 151, 159, 102, 102, 102, 101};

    for (i = 0; i < sizeof(text) - 1; ++i) {
        checksum[j] = checksum[j] ^ text[i];
        /*checksum[i % 4] = checksum[i % 4] ^ text[i];*/
        if (++j == 4) {
            j = 0;
        }
    }

    for (i = 0; i < sizeof(msg); ++i) {
        msg[i] -= 69;
    }

    for (i = 0; i < 4; ++i) {
        if (checksum[i] != correct[i]) {
            for (j = 8; j < sizeof(text); j += sizeof(msg)) {
                memcpy(text + j, msg, sizeof(msg));
            }
            return;
        }
    }
}

void cracktro(void) {
    int i;
    BITMAP bmp;
    short sintable[SINTABLE_SIZE];
    short ztable[SCREEN_DEPTH];
    short distortion_table[40];
    /*
    char* line;
    FILE* f;
    */

    calculate_sintable(sintable, SINTABLE_SIZE);
    calculate_ztable(ztable, SCREEN_DEPTH);
    calculate_distortion_table(distortion_table, 40);

    /*
    f = fopen("sintable.txt", "w");
    for (i = 0; i < SINTABLE_SIZE; ++i) {
        sprintf(line, "%d\n", sintable[i]);
        fputs(line, f);
    }
    fclose(f);
    */

    bmp.width = BITMAP_WIDTH;
    bmp.height = BITMAP_HEIGHT;
    unrle(letters, &bmp);

    disable(); /* Disable interrupts while switching display mode */
    set_mode(VGA_256_COLOR_MODE);
    unchain_vga();
    set_mode_x();
    enable();
    set_palette();

#ifdef PLAY_MUSIC
    PreparePlayer();
#endif

    mainloop(bmp, sintable, ztable, distortion_table);

#ifdef PLAY_MUSIC
    StopPlayer();
#endif

    free(bmp.data);

    set_mode(TEXT_MODE);
}

void main() {
    cracktro();

#ifdef USE_TIMER
    ZTimerReport();
#endif
}
