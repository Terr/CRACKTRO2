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

void extern PreparePlayer();
void extern StopPlayer();

/*void extern runGame();*/

/*#define USE_TIMER*/

#ifdef USE_TIMER
void extern ZTimerOn();
void extern ZTimerOff();
void extern ZTimerReport();
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
#define TEXT_PALETTE_SIZE 90
#define TEXT_PALETTE_COLORS 270

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

#define MAP_MASK            0x02      /* Sequence controller registers */
#define ALL_PLANES          0xff02
#define MEMORY_MODE         0x04

#define LATCHES_ON          0x0008    /* Graphics controller registers */
#define LATCHES_OFF         0xff08

#define HIGH_ADDRESS        0x0C      /* CRT controller registers */
#define LOW_ADDRESS         0x0D
#define UNDERLINE_LOCATION  0x14
#define MODE_CONTROL        0x17

#define DISPLAY_ENABLE      0x01      /* VGA input status bits */

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define NUM_PIXELS SCREEN_WIDTH * SCREEN_HEIGHT
#define SCREEN_DEPTH 128
#define HALF_WIDTH 160
#define HALF_HEIGHT 100
#define PLANE_WIDTH 80

#define PI 3.14159
#define SINTABLE_SIZE 256
#define ZTABLE_FIXED_FRAC 6

#define SETPIX(x,y,c) *(VGA+(x)+(y<<8)+(y<<6))=c
#define GETPIX(x,y) *(VGA+(x)++(y<<8)+(y<<6))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#define NUM_LETTERS 16
#define NUM_STARS 69
/*#define NUM_STARS 1*/
#define STAR_SPEED 1
#define LETTER_SCROLL_SPEED 3
#define LETTER_WIDTH 30
#define LETTER_HEIGHT 30
#define LETTER_SPACE 2000
#define LETTER_PADDING 4
#define TEXT_Y_OFFSET 78
/*#define WIGGLE 10*/
#define WIGGLE 20

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
    short prevx; /* The screen coordinates the star was drawn in the previous frame */
    short prevy;
    byte c;
} STAR;

/*char text[] =   "                 NOSTALGIA PROUDLY PRESENTZ: SIMCITY V1.07 +1 BY MAXIS!        CRACKED BY: LOADHIGH   TRAINED BY: LOADHIGH        ONLY NOSTALGIA CAN BRING YOU THE FINEST 12814-DAY RELEASEZ!        GREETZ TO ALL THE ELITEZ: REALITY TTE H0FFMAN ROOT42 TWINBEARD BRACKEEN AND EVERYONE ELSE WHO DESIREZ A GREET!        AND NOW FOR THE CRACKTRO CREDITZ:   MUSIX: MARVIN   RAD LIB: REALITY   CODE/GRAFIX: LOADHIGH          MMXXIV  ";*/
/*char text[] =   "NOSTALGIA PROUDLY PRESENTZ: SIMCITY V1.07 +1 BY MAXIS!        CRACKED BY: LOADHIGH   TRAINED BY: LOADHIGH        ONLY NOSTALGIA CAN BRING YOU THE FINEST 12814-DAY RELEASEZ!        GREETZ TO ALL THE ELITEZ: REALITY TTE H0FFMAN ROOT42 TWINBEARD BRACKEEN AND EVERYONE ELSE WHO DESIREZ A GREET!        AND NOW FOR THE CRACKTRO CREDITZ:   MUSIX: MARVIN   RAD LIB: REALITY   CODE/GRAFIX: LOADHIGH          MMXXIV  ";*/
/*char text[] = "         ABCDEFGHIJKLMNOPQRSTUVWXYZ";*/
/*char text[] = " BRBRBRBRBRBR HERE COMES THE CHINESE EARTHQUAKE";*/
/*char text[] = "1 2 3 4 5 6 7 8 9 0";*/
/*char text[] = "ACAB ACAB ACAB ACAB ACAB ACAB";*/
char text[] = "ACAB ACAB";
/*char text[] = "A";*/

static byte sin_index = 0;

void set_mode(byte mode)
{
    union REGS regs;
    regs.h.ah = SET_MODE;
    regs.h.al = mode;
    int86(VIDEO_INT, &regs, &regs);
}

/* Unchain memory to enable planar addressing, aka Mode Y */
void set_unchained_mode(void)
{
    word i;
    dword *ptr=(dword *)VGA;            /* used for faster screen clearing */

    outp(SC_INDEX,  MEMORY_MODE);       /* turn off chain-4 mode */
    outp(SC_DATA,   0x06);

    /* outpw() is not available in Turbo C */
    outport(SC_INDEX, ALL_PLANES);      /* set map mask to all 4 planes */

    for(i=0; i<0x4000; ++i) {           /* clear all 256K of memory */
        *ptr++ = 0;
    }

    outp(CRTC_INDEX,UNDERLINE_LOCATION);/* turn off long mode */
    outp(CRTC_DATA, 0x00);

    outp(CRTC_INDEX,MODE_CONTROL);      /* turn on byte mode */
    outp(CRTC_DATA, 0xe3);
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

void set_palette() {
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

    /* Rainbow palette (index 1-90) */
    for (angle = 0, i = 0; i < TEXT_PALETTE_SIZE; i++,angle += 4) {
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

    /* More black (index 90-127)  */
    for (i = 0; i < 37; ++i) {
        outp(PALETTE_COLORS, 0);
        outp(PALETTE_COLORS, 0);
        outp(PALETTE_COLORS, 0);
    }

    /* Grey tones - 128, used for stars */
    for (i = 0; i < 64; ++i) {
        outp(PALETTE_COLORS, i);
        outp(PALETTE_COLORS, i);
        outp(PALETTE_COLORS, i);
        outp(PALETTE_COLORS, i);
        outp(PALETTE_COLORS, i);
        outp(PALETTE_COLORS, i);
    }
}

void calculate_sintable(byte *table, int table_size)
{
    int i;
    for (i = 0; i < table_size; ++i) {
        /*table[i] = TEXT_Y_OFFSET + WIGGLE * sin((float) ((PI / 256) * i) * 8);*/
        table[i] = TEXT_Y_OFFSET + WIGGLE * sin((float) ((PI / 512) * i) * 4);
        /*table[i] = TEXT_Y_OFFSET + WIGGLE * sin((float) ((PI / 512) * i) * 8);*/
    }
}

void calculate_ztable(short *table, int table_size) {
    int i;
    float dividend = (float)SCREEN_DEPTH;
    table[0] = 0.0;
    for (i = 1; i < table_size; ++i) {
        table[i] = (dividend / i) * (1 << ZTABLE_FIXED_FRAC);
    }
}

void flip_pages(word *visible_page, word *non_visible_page) {
    word temp = *visible_page;
    *visible_page = *non_visible_page;
    *non_visible_page = temp;
}

void mainloop(BITMAP *bmp, byte *sintable) {
    short x, y;
    short rx;
    byte i = 0, j = 0, ri = 0;
    byte plane = 0;
    short ci = 0;
    short color_cycle = 0;
    short text_index = 0;
    SPRITE letters[NUM_LETTERS];
    SPRITE letter;
    STAR stars[NUM_STARS];
    STAR star;
    word screen_offset, bitmap_offset;
    /*byte r, g, b;*/
    byte palette[TEXT_PALETTE_COLORS];
    /* VGA pages */
    word temp = 0;
    word visible_page = 0;
    word non_visible_page = NUM_PIXELS / 4;
    word high_address,low_address;

    /* Initial letters */
    for (ri = 0; ri < NUM_LETTERS; ++ri) {
        letters[ri].x = (short)((LETTER_WIDTH + LETTER_PADDING) * ri);
        if (text[ri] >= 65) {
            /* Letters */
            letters[ri].letter_offset = (short)((text[text_index] - 65) * 32);
        } else if (text[ri] >= 48) {
            /* Digits */
            letters[ri].letter_offset = (short)((26 + text[text_index] - 48) * 32);
        } else {
            /* Space */
            letters[ri].letter_offset = LETTER_SPACE;
        }
        /*letters[ri].sin_index = ri * 2;*/
        /*letters[ri].sin_index = ri;*/
        text_index++;
    }

    /* Store palette for cycling */
    outp(PALETTE_READ_INDEX, 1);
    for (ci = 0; ci < TEXT_PALETTE_COLORS; ++ci) {
        palette[ci] = inp(PALETTE_COLORS);
    }

    while (1) {
        #ifdef USE_TIMER
        ZTimerOn();
        #endif

        /* Read keyboard input */
        geninterrupt(KEYBOARD_INT);
        switch (inp(0x60)) {
            case 0x1:
                return;
        }

        /* Cycle the palette of the letters */
        /*
        outp(PALETTE_WRITE_INDEX, 1);
        for (ci = color_cycle; ci < TEXT_PALETTE_COLORS; ++ci) {
            outportb(PALETTE_COLORS, palette[ci]);
        }
        for (ci = 0; ci < color_cycle; ++ci) {
            outportb(PALETTE_COLORS, palette[ci]);
        }
        color_cycle += 3;
        if (color_cycle == TEXT_PALETTE_COLORS) {
            color_cycle = 0;
        }
        */

        /* Clear page */
        outport(SC_INDEX, ALL_PLANES);
        memset(&VGA[non_visible_page], 0, NUM_PIXELS / 4);

        /* First update the position of the letters, then draw them to the four planes */
        for (ri = 0; ri < NUM_LETTERS; ++ri) {
            letter = letters[ri];

            letter.x -= LETTER_SCROLL_SPEED;

            if (letter.x <= -LETTER_WIDTH) {
                /* letter is completely offscreen */
                letter.x = LETTER_PADDING + (LETTER_WIDTH + LETTER_PADDING) * (NUM_LETTERS - 1);

                if (text[text_index] >= 65) {
                    letter.letter_offset = (short)((text[text_index] - 65) * 32);
                } else if (text[text_index] >= 48) {
                    /* Digits + colon */
                    letter.letter_offset = (short)((text[text_index] - 22) * 32);
                } else if (text[text_index] == 33) {
                    /* Exclamation mark */
                    letter.letter_offset = 1184;
                } else if (text[text_index] == 43) {
                    /* Plus sign */
                    letter.letter_offset = 1216;
                } else if (text[text_index] == 46) {
                    /* Dot */
                    letter.letter_offset = 1248;
                } else if (text[text_index] == 47) {
                    /* Slash */
                    letter.letter_offset = 1312;
                } else if (text[text_index] == 45) {
                    /* Minus sign */
                    letter.letter_offset = 1280;
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

        /* TEMP */
        letters[0].x = 4;

        /* Draw letters */
        /*for (plane = 0; plane < 4; ++plane) {*/
        for (plane = 0; plane < 1; ++plane) {
            /* Select the next plane */
            /*outport(SC_INDEX, 0x0102);*/
            /*outp(SC_INDEX, MAP_MASK);*/
            /*outp(SC_DATA, 1 << plane);*/

            for (ri = 0; ri < NUM_LETTERS; ++ri) {
                letter = letters[ri];

                if (letter.letter_offset == LETTER_SPACE) {
                    continue;
                }

                if (letter.x < 0) {
                    /*
                       screen_offset = (y<<8)+(y<<6);
                       bitmap_offset = letter.letter_offset - letter.x;

                       for(j = 0; j < LETTER_HEIGHT; ++j)
                       {
                       memcpy(&double_buffer[screen_offset], &bmp->data[bitmap_offset], LETTER_WIDTH + letter.x);

                       bitmap_offset += bmp->width;
                       screen_offset += SCREEN_WIDTH;
                       }
                       */
                } else if (letter.x + LETTER_WIDTH < SCREEN_WIDTH) {
                    /* Letter is completely on-screen */

                    /*
                        outp(SC_INDEX, MAP_MASK);
                        outp(SC_DATA, 1 << plane);
                        for (x = 0; x < 80; ++x) {
                            for (y = 0; y < SCREEN_HEIGHT; ++y) {
                                VGA[non_visible_page + (y << 6) + (y << 4) + x] = 4;
                            }
                        }
                        */

                    for (i = 0; i < LETTER_WIDTH; i = i + 1) {
                    /*for (i = 0; i < 4; i = i + 1) {*/
                        rx = letter.x + i;

            outp(SC_INDEX, MAP_MASK);
            outp(SC_DATA, 1 << (rx & 3));

                        y = sintable[sin_index++];
                        bitmap_offset = letter.letter_offset + i;

                        screen_offset = ((y) << 6) + ((y) << 4) + (rx >> 2);
                        for(j = 0; j < LETTER_HEIGHT; ++j) {
                            if (bmp->data[bitmap_offset] != 0) {
                                /*double_buffer[screen_offset] = bmp->data[bitmap_offset];*/
                                /*double_buffer[screen_offset] = (y + j) - 58;*/
                                VGA[non_visible_page + screen_offset] = y - 57;
                            }

                            screen_offset += PLANE_WIDTH;
                            bitmap_offset += bmp->width;
                        }
                    }
                } else if (letter.x + LETTER_WIDTH < SCREEN_WIDTH + LETTER_WIDTH - 1) {
                    /*
                       screen_offset = (y<<8)+(y<<6) + letter.x;
                       bitmap_offset = letter.letter_offset;
                       width = SCREEN_WIDTH - letter.x;

                       for(j = 0; j < LETTER_HEIGHT; ++j)
                       {
                       memcpy(&double_buffer[screen_offset], &bmp->data[bitmap_offset], width);

                       bitmap_offset += bmp->width;
                       screen_offset += SCREEN_WIDTH;
                       }
                       */
                }
            }
        }

        /*wait_for_retrace();*/

        /*flip_pages(&visible_page, &non_visible_page);*/
        temp = visible_page;
        visible_page = non_visible_page;
        non_visible_page = temp;

        high_address = HIGH_ADDRESS | (visible_page & 0xFF00);
        low_address = LOW_ADDRESS | (visible_page << 8);

        /* Wait for end of current vertical trace(?) */
        while ((inp(INPUT_STATUS) & DISPLAY_ENABLE));
        outport(CRTC_INDEX, high_address);
        outport(CRTC_INDEX, low_address);
        /* Wait for start of next vertical trace(?) */
        while (!(inp(INPUT_STATUS) & VRETRACE));

        getch();

        #ifdef USE_TIMER
        ZTimerOff();
        #endif
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

void cracktro() {
    int i;
    BITMAP bmp;
    byte sintable[SINTABLE_SIZE];

    calculate_sintable(sintable, SINTABLE_SIZE);

    bmp.width = 1344;
    bmp.height = 30;
    unrle(letters, &bmp);

    set_mode(VGA_256_COLOR_MODE);
    set_unchained_mode();
    set_palette();

    mainloop(&bmp, sintable);

    free(bmp.data);

    /* Clears the screen, prevents some screen artifacts while switching to text mode */
    memset(VGA, 0, NUM_PIXELS);
    set_mode(TEXT_MODE);
}

void main() {
    cracktro();

#ifdef USE_TIMER
    ZTimerReport();
#endif
}
