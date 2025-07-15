/**
 * MIT License
 *
 * Copyright (c) 2025 Arjen Verstoep
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Needs to be compiled with the Large memory model (-ml) */

#include <dos.h>
#include <mem.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "vga.h"
#include "letters.h"
#include "precalc.h"

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned int   dword;

#define PLAY_MUSIC
/*#define USE_TIMER*/
#define VSYNC
/* The "ASM palette swap" method using interrupts to copy the palette is slightly faster but does not work real
 * hardware, or at least not on a C&T 65555 PCI chipset.
 *
 * On that chipset the screen flickers a bit and it doesn't manage to display the full image.
 */
/*#define USE_ASM_PALETTE_SWAP*/
#define DO_FADEOUT

#ifdef PLAY_MUSIC
void extern PreparePlayer(void);
void extern StopPlayer(void);
/*void extern InitPlayer();*/
/*void extern EndPlayer();*/
/*void extern PlayMusic();*/
#endif

#ifdef USE_TIMER
void extern ZTimerOn(void);
void extern ZTimerOff(void);
void extern ZTimerReport(void);
#endif

/* 18.2Hz system clock */
word *my_clock=(word *)0x0000046C;

/* VGA video memory segment */
byte far *VGA=(byte far *)0xA0000000L;
static unsigned int visible_page = 0;
static unsigned int non_visible_page = NUM_PIXELS / 4;
static unsigned int color_offset = 0, alt_color_offset = 0;

typedef struct             /* the structure for a bitmap. */
{
  word width;
  word height;
  byte *data;
} BITMAP;

typedef struct
{
    int x;
    word letter_offset;
    int r_from;
    int r_to;
} SPRITE;

char text[] = "               OH, HELLO! I DID NOT SEE YOU THERE!    WELCOME TO THIS WAVY TEXT CRACKTRO THAT FEATURES SOME NEAT WATER REFLECTION EFFECTS     THIS EFFECT IS ACHIEVED BY COMBINING SOME VGA TRICKS LIKE PAGE FLIPPING, LATCH COPYING, AND PALETTE SWAPPING, ALL IN MODE X!         GREETZ FLY OUT TO: YOU!!!           HACK THE PLANET!!!            MMXXV  ";

static void interrupt (*org_kbd_handler)(void);
static byte esc_pressed = 0;

#define HELPTEXT_NUM_LINES 8
/* Don't forget the NUL terminator */
#define HELPTEXT_LINE_WIDTH 20
char helptext[HELPTEXT_NUM_LINES][HELPTEXT_LINE_WIDTH] = {
    "  GAME +4 TRAINER  ",
    "-------------------",
    "WHILE IN-GAME PRESS",
    "                   ",
    "SHIFT+F1   30K GOLD",
    "SHIFT+F2 ALL POWUPS",
    "SHIFT+F3 EXTRA LIFE",
    "SHIFT+F4  100 LIVES",
};

static int frame_counter = 0;

#define KEYBOARD_INT 0x9

#define SETPIX(x,y,c) *(VGA+(x)+(y<<8)+(y<<6))=c
#define GETPIX(x,y) *(VGA+(x)++(y<<8)+(y<<6))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

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

/* Keyboard interrupt handler (INT 9) */
static void interrupt kbdhandler(void) {
    unsigned char temp;

    switch (inportb(0x60)) {
        case 0x1:
            esc_pressed = 1;

            /* Acknowledge that the keypress was read */
            outportb(0x61, (temp = inportb(0x61)) | 128);
            outportb(0x61, temp);
            break;

        default:
            org_kbd_handler();
            break;
    }

    /* Acknowledge that the interrupt was handled */
    outportb(0x20, 0x20);
}

/**
 * Stores the letters bitmap in VGA memory at half-size (12x12 pixels)
 */
void store_small_font_in_vga_memory(BITMAP bmp, word vga_page, short from_x_position, short to_x_position, byte color_offset) {
    byte plane = 0;
    short j = 0;
    short y = 0;
    short rx = 0;
    word screen_memory_offset, screen_offset, bitmap_offset;
    byte cy = 0;

    from_x_position >>= 1;
    to_x_position >>= 1;

    for (plane = 0; plane < 4; ++plane) {
        outport(SC_INDEX, ((1 << plane) << 8) + MAP_MASK);

        /* rx holds the column to draw on the currently selected plane */
        for (rx = from_x_position + plane; rx < to_x_position; rx = rx + 4) {
            /* Start at row 2 of the letter, step 2 pixels at a time in X direction */
            cy = 1 + (y >> 9) + color_offset;
            screen_offset = y + (rx >> 2) - from_x_position;
            bitmap_offset = BITMAP_WIDTH + (rx << 1);
            screen_memory_offset = vga_page + screen_offset;

            for(j = 0; j < LETTER_HALF_HEIGHT; ++j) {

                if (bmp.data[bitmap_offset] > 0) {
                    VGA[screen_memory_offset] = (cy + j);
                }

                screen_memory_offset += PLANE_WIDTH;
                bitmap_offset += (BITMAP_WIDTH << 1); /* Step times 2, skips 1 row */
            }
        }
    }
}

void wait_for_retrace(void)
{
    /* wait until done with vertical retrace */
    while  ((inp(INPUT_STATUS) & VRETRACE)) {};
    /* wait until done refreshing */
    while (!(inp(INPUT_STATUS) & VRETRACE)) {};
}

void set_palette(void) {
    byte red, green, blue;
    float angle_degrees;
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
    for (angle = 0, angle_degrees = 0.0, i = 0;
            i < TEXT_PALETTE_SIZE - 1;
            i++, angle_degrees += TEXT_PALETTE_ANGLE) {
        angle = (short)angle_degrees;

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
    outp(PALETTE_COLORS, COLOR_WATER); /* B */

    /* Water-tinted rainbow palette (index 65-128) */
    for (angle = 0, angle_degrees = 0.0, i = 0;
            i < TEXT_PALETTE_SIZE - 1;
            i++, angle_degrees += TEXT_PALETTE_ANGLE) {
        angle = (short)angle_degrees;
        if (angle<120) {red = 0, green = ceil((120-angle)*4.25-0.01); blue = 200;} else
        if (angle<180) {red = 0, green = ceil((240-angle)*4.25-0.01); blue = 210;} else
        if (angle<240) {red = 0, green = ceil((240-angle)*4.25-0.01); blue = 255;} else
                        {red = 0, green = 0; blue = ceil((360-angle)*4.25-0.01);}

        outp(PALETTE_COLORS, red >> 2); /* R to 18-bit */
        outp(PALETTE_COLORS, green >> 2); /* G */
        outp(PALETTE_COLORS, blue >> 2); /* B */
    }
}

void read_palette(byte *palette, byte *alt_palette) {
    int ci;

    /* Temporarily disable interrupts */
    disable();

    /* Normal palette: rainbow colors followed by water colors */
    outp(PALETTE_READ_INDEX, 0);
    for (ci = 0; ci < TEXT_PALETTE_COLORS; ++ci) {
        palette[ci] = inp(PALETTE_COLORS);
    }

    /* Alt palette: water colors followed by rainbow colors */
    outp(PALETTE_READ_INDEX, TEXT_PALETTE_SIZE);
    for (ci = 0; ci < 3 * TEXT_PALETTE_SIZE; ++ci) {
        alt_palette[ci] = inp(PALETTE_COLORS);
    }
    outp(PALETTE_READ_INDEX, 0);
    for (ci = 3 * TEXT_PALETTE_SIZE; ci < TEXT_PALETTE_COLORS; ++ci) {
        alt_palette[ci] = inp(PALETTE_COLORS);
    }

    /* Enable interrupts again */
    enable();
}

void flip_pages(dword *visible_page, dword *non_visible_page, dword *high_address, dword *low_address) {
    dword temp = *visible_page;
    *visible_page = *non_visible_page;
    *non_visible_page = temp;
    *high_address = HIGH_ADDRESS | (*visible_page & 0xFF00);
    *low_address = LOW_ADDRESS | (*visible_page << 8);
}

int font_offset_for_letter(char letter) {
    if (letter >= 65) {
        return (short)((letter - 65) << 4) + (short)((letter - 65) << 3);
    } else if (letter >= 48) {
        /* Digits + colon */
        return (short)((letter - 22) << 4) + ((letter - 22) << 3);
    } else if (letter == 33) {
        /* Exclamation mark */
        return 888;
    } else if (letter == 43) {
        /* Plus sign */
        return 912;
    } else if (letter == 46) {
        /* Dot */
        return 936;
    } else if (letter == 45) {
        /* Minus sign */
        return 960;
    } else if (letter == 47) {
        /* Slash */
        return 984;
    } else if (letter == 44) {
        /* Comma */
        return 1008;
    } else if (letter == 35) {
        /* Question mark, but ASCII code of '?' isn't recognized so use a '#' instead */
        return 1032;
    } else {
        /* Space */
        return LETTER_SPACE;
    }
}

/* Takes one step in fading both palettes out to black */
void fadeout_step(byte *palette, byte *alt_palette) {
    int ri = 0, gi = 0, bi = 0;

    for (ri = 3, gi = 4, bi = 5; ri < 3 * TEXT_PALETTE_SIZE; ri += FADE_OUT_STEP, bi += FADE_OUT_STEP, gi += FADE_OUT_STEP) {
        if (palette[ri] >= FADE_OUT_STEP) {
            palette[ri] -= FADE_OUT_STEP;
        }

        if (palette[gi] >= FADE_OUT_STEP) {
            palette[gi] -= FADE_OUT_STEP;
        }

        if (palette[bi] >= FADE_OUT_STEP) {
            palette[bi] -= FADE_OUT_STEP;
        }

        if (alt_palette[ri] >= FADE_OUT_STEP) {
            alt_palette[ri] -= FADE_OUT_STEP;
        }

        if (alt_palette[gi] >= FADE_OUT_STEP) {
            alt_palette[gi] -= FADE_OUT_STEP;
        }

        if (alt_palette[bi] >= COLOR_WATER) {
            alt_palette[bi] -= FADE_OUT_STEP;
        } else {
            alt_palette[bi] = COLOR_WATER;
        }
    }

    for (ri = 3 * (TEXT_PALETTE_SIZE + 1), gi = ri + 1, bi = ri + 2;
            ri < TEXT_PALETTE_COLORS;
            ri += FADE_OUT_STEP, bi += FADE_OUT_STEP, gi += FADE_OUT_STEP) {
        if (palette[ri] >= FADE_OUT_STEP) {
            palette[ri] -= FADE_OUT_STEP;
        }

        if (palette[gi] >= FADE_OUT_STEP) {
            palette[gi] -= FADE_OUT_STEP;
        }

        if (palette[bi] >= COLOR_WATER) {
            palette[bi] -= FADE_OUT_STEP;
        } else {
            palette[bi] = COLOR_WATER;
        }

        if (alt_palette[ri] >= FADE_OUT_STEP) {
            alt_palette[ri] -= FADE_OUT_STEP;
        }

        if (alt_palette[gi] >= FADE_OUT_STEP) {
            alt_palette[gi] -= FADE_OUT_STEP;
        }

        if (alt_palette[bi] >= FADE_OUT_STEP) {
            alt_palette[bi] -= FADE_OUT_STEP;
        }
    }
}

void wavy_text_scene(BITMAP bmp, int *sintable, int *distortion_table) {
    /* The number of steps in the fade out process. Loop will stop when this reaches 0 */
    char fadeout_steps_remaining = 21 * FADE_OUT_STEP;
    int x = 0, y = 0, i = 0;
    int start_x;
    int rx;
    int r_from, r_to;
    int ri = 0;
    int plane = 0;
    /* These are bytes because they are used to modify VGA memory, which is a byte pointer */
    byte cy = 0, j = 0;
    int text_index = 0;
    /* A byte is used because of its automatic wrapping after 255 */
    byte distortion;
    unsigned int distortion_plane, distortion_plane_common;
    SPRITE letters[NUM_LETTERS];
    SPRITE letter;
    unsigned int screen_offset, bitmap_offset;
    unsigned int copy_source, copy_destination;
    byte palette[TEXT_PALETTE_COLORS];
    byte alt_palette[TEXT_PALETTE_COLORS];
    /* VGA pages */
    /* The compiler warns that this variable is written to but never read.
     * It's used to store values read from VGA memory, which is done in
     * preparation of latch copying and needs a variable to avoid the compiler
     * from optimizing the read out. */
    unsigned int temp = 0;
    unsigned int high_address, low_address;
#ifdef USE_ASM_PALETTE_SWAP
    word palette_seg = FP_SEG(palette);
    word palette_off = FP_OFF(palette);
    word alt_palette_seg = FP_SEG(alt_palette);
    word alt_palette_off = FP_OFF(alt_palette);
#else
    int ci;
#endif

    read_palette(palette, alt_palette);

    /* Set the offsets in the font bitmap for the first letters of the wavy text */
    for (ri = 0; ri < NUM_LETTERS; ++ri) {
        letters[ri].x = (short)((LETTER_WIDTH + LETTER_PADDING) * ri);

        letters[ri].letter_offset = font_offset_for_letter(text[text_index]);
        text_index++;
    }

    outport(SC_INDEX, ALL_PLANES);

    while (!(esc_pressed && fadeout_steps_remaining <= 0)) {
        #ifdef USE_TIMER
        ZTimerOn();
        #endif

        /* Interrupts are still disabled at this point */

        /* Fade out the letters when escape is pressed */
        if (esc_pressed) {
            #ifndef DO_FADEOUT
            break;
            #endif

            fadeout_step(palette, alt_palette);
            fadeout_steps_remaining -= FADE_OUT_STEP;
        }

        /* Clear page */
        /* All planes should already be selected because of the latch copy at the end of the loop */
        /*outport(SC_INDEX, ALL_PLANES);*/
        memset(&VGA[non_visible_page], color_offset, UPPER_AREA_PLANE_PIXELS);
        /* Ensures that the reflection background is fully filled with the water color */
        /* Most of the reflection is overwritten during the drawnig phase but
         * everything below the reflection rows need to filled with the right
         * color */
        memset(&VGA[non_visible_page+UPPER_AREA_PLANE_PIXELS+REFLECTION_AREA_PLANE_PIXELS], alt_color_offset, REFLECTION_AREA_BOTTOM_EDGE_PLANE_PIXELS);

        /* First update the position of the letters */
        for (ri = 0; ri < NUM_LETTERS; ++ri) {
            letter = letters[ri];

            letter.x -= LETTER_SCROLL_SPEED;

            /* Update render start and end positions (not taking planes into the account) */
            if (letter.x >= 0 && letter.x < SCREEN_WIDTH - LETTER_WIDTH) {
                /* Letter is completely on screen */
                letter.r_from = (letter.x - (letter.x & 3)) - letter.x;
                letter.r_to = LETTER_WIDTH;
            } else if (letter.x < 0) {
                /* Left side of the letter is offscreen */
                letter.r_from = -letter.x;
                letter.r_to = LETTER_WIDTH;
            } else {
                /* Right side of the letter is offscreen */
                letter.r_from = (letter.x - (letter.x & 3)) - letter.x;
                letter.r_to = -(letter.x - SCREEN_WIDTH);
            }

            if (letter.x <= -LETTER_WIDTH) {
                /* Letter is completely offscreen so load the next one to appear on the right */
                letter.x = LETTER_PADDING + (LETTER_WIDTH + LETTER_PADDING) * (NUM_LETTERS - 1);

                letter.letter_offset = font_offset_for_letter(text[text_index]);
                text_index++;

                /* Repeat the text from the start when we get to the end */
                if (text_index == sizeof(text) - 1) {
                    text_index = 0;
                }
            }

            letters[ri] = letter;
        }

        /* Draw letters */
        for (plane = 0; plane < 4; ++plane) {
            /* Select the next plane */
            outport(SC_INDEX, ((1 << plane) << 8) + MAP_MASK);

            for (ri = 0; ri < NUM_LETTERS; ++ri) {
                letter = letters[ri];

                r_from = letter.r_from + plane;
                if (r_from < 0) {
                    r_from += 4;
                }

                for (i = r_from; i < letter.r_to; i += 4) {
                    /* This holds the column to draw on the currently selected plane */
                    rx = letter.x + i;

                    y = sintable[(byte)rx];
                    /* Bring the Y-coordinate back to a smaller value so we can use it directly as a color code */
                    cy = 1 + (y >> 9) + color_offset;

                    bitmap_offset = letter.letter_offset + i;
                    screen_offset = non_visible_page + y + (rx >> 2);

                    /* Handle spaces differently: because they only consist of
                     * empty pixels, the frame rate goes up when there are
                     * several spaces on screen because we normally don't write
                     * empty pixels to VGA memory. Do put the VGA to work for
                     * spaces to slow it down a little. */
                    if (letter.letter_offset == LETTER_SPACE) {
                        for (j = 0; j < 6; ++j) {
                            VGA[screen_offset] = color_offset;
                        }
                    } else {
                        for (j = 0; j < LETTER_HEIGHT; ++j) {
                            if (bmp.data[bitmap_offset] > 0) {

                                VGA[screen_offset] = cy;
                            }

                            cy++;
                            screen_offset += PLANE_WIDTH;
                            bitmap_offset += BITMAP_WIDTH;
                        }
                    }
                }
            }
        }

        /* Copy pixels from the other page using latches */
        /* Tell the VGA that all writes are to be done with bits from the latches, and none from the CPU */
        outp(GC_INDEX, 0x08);
        outp(GC_DATA, 0x00);

        /* Select all planes */
        outport(SC_INDEX, ALL_PLANES);

        copy_source = visible_page + REFLECTION_SOURCE_START;
        copy_destination = non_visible_page + REFLECTION_DESTINATION_START;

        for (y = 0; y < REFLECTION_ROWS; ++y) {
            distortion = y + frame_counter;
            distortion_plane = distortion_table[distortion++];
            copy_source += distortion_plane;

            for (x = 0; x < PLANE_WIDTH; ++x) {
                /* This code results in a nicer looking water reflection but it's also way slower */
                /*distortion_plane = distortion_table[distortion++];*/

                /* This if() is true in most of the cases, avoiding an
                 * expensive jump instruction. Only at the last 4 iterations
                 * can distortion_plane_common become too big
                 *
                 * At least, that's the idea. Benchmarking seems to refute this
                 * but that would mean that calculating a second
                 * `distortion_plane` and copying `distortion_plane_common` to
                 * `distortion_plane` is more expensive than a jump?
                 */
                /*
                if (distortion_plane >= PLANE_WIDTH) {
                    distortion_plane = PLANE_WIDTH - 1;
                }

                if ((x & 1) == 1) {
                    distortion_plane = distortion_table[distortion++];
                }

                if (distortion_plane > PLANE_WIDTH) {
                    distortion_plane = distortion_plane_common;
                    distortion_plane = PLANE_WIDTH - x - 1;
                }
                temp = VGA[copy_source];
                VGA[copy_destination] = 0;
                */

                /* Simpler distortian effect, simply relying on the pre-calculated reflection table */
                temp = VGA[copy_source];
                VGA[copy_destination] = 0;

                distortion_plane++;
                if (distortion_plane < PLANE_WIDTH - 1) {
                    copy_source++;
                }
                copy_destination++;
            }

            /* Skip 8 rows, compresses the reflected image into a smaller area,
             * as if looking at it from an angle */
            copy_source = visible_page + REFLECTION_SOURCE_START - (y * REFLECTION_ROW_STEP);
            /*copy_destination += PLANE_WIDTH;*/
        }

        /* Restore write behaviour: copy data from normal memory/CPU registers */
        outp(GC_DATA, 0xFF);
        /*wait_for_retrace();*/

        flip_pages(&visible_page, &non_visible_page, &high_address, &low_address);

#ifdef VSYNC
        /* Wait for end of current vertical trace(?) */
        while ((inp(INPUT_STATUS) & DISPLAY_ENABLE));
#endif
        disable();
        outport(CRTC_INDEX, high_address);
        outport(CRTC_INDEX, low_address);
#ifdef VSYNC
        /* Wait for start of next vertical trace(?) */
        while (!(inp(INPUT_STATUS) & VRETRACE));
#endif

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
            for (ci = 0; ci < TEXT_PALETTE_COLORS;) {
                outportb(PALETTE_COLORS, palette[ci++]);
                outportb(PALETTE_COLORS, palette[ci++]);
                outportb(PALETTE_COLORS, palette[ci++]);
            }
        } else {
            color_offset = 0;
            alt_color_offset = TEXT_PALETTE_SIZE;

            outp(PALETTE_WRITE_INDEX, 0);
            for (ci = 0; ci < TEXT_PALETTE_COLORS;) {
                outportb(PALETTE_COLORS, alt_palette[ci++]);
                outportb(PALETTE_COLORS, alt_palette[ci++]);
                outportb(PALETTE_COLORS, alt_palette[ci++]);
            }
        }
#endif
        enable();

        ++frame_counter;

        #ifdef USE_TIMER
        ZTimerOff();
        #endif
    }
}

static void cycle_palette(byte *palette, int last_index, int first_index) {
    /*int ci;*/
    byte r, g, b;

    r = palette[last_index - 2];
    g = palette[last_index - 1];
    b = palette[last_index]; /* 3 + 3 * (TEXT_PALETTE_SIZE - 1) */

    memmove(palette + first_index + 3,
            palette + first_index,
            (last_index - first_index) - 2);
    /*movmem(palette + first_index, palette + first_index + 3, 186);*/
    /*for (ci = last_index; ci > first_index; ci -= 3) {
        palette[ci - 2] = palette[ci - 5]; [> R <]
        palette[ci - 1] = palette[ci - 4]; [> G <]
        palette[ci] = palette[ci - 3];     [> B <]
    }*/

    palette[first_index] = r;
    palette[first_index + 1] = g;
    palette[first_index + 2] = b;
}

void trainer_help_scene(int *distortion_table, word vga_storage_page, byte *palette, byte *alt_palette) {
    int x = 0;
    int y = 0;
    int start_y = 200;
    char fadeout_steps_remaining = 21 * FADE_OUT_STEP;
    int i;
    int ri, gi, bi;
    byte j = 0;
    byte helptext_line = 0;
    unsigned int color_storage_offset = 0;
    unsigned int letter_storage_offset = 0;
    int text_index = 0;
    byte distortion;
    unsigned int distortion_plane;
    SPRITE letters[HELPTEXT_NUM_LINES][HELPTEXT_LINE_WIDTH];
    SPRITE letter;
    unsigned int screen_memory_offset, screen_offset, storage_memory_offset;
    unsigned int copy_source, copy_destination;
    /* VGA pages */
    unsigned int temp = 0;
    unsigned int high_address, low_address;
#ifdef USE_ASM_PALETTE_SWAP
    word palette_seg = FP_SEG(palette);
    word palette_off = FP_OFF(palette);
    word alt_palette_seg = FP_SEG(alt_palette);
    word alt_palette_off = FP_OFF(alt_palette);
#else
    int ci;
#endif

    /* Offsets in the *small* font bitmap for each letter in the help text */
    for (helptext_line = 0; helptext_line < HELPTEXT_NUM_LINES; ++helptext_line) {
        text_index = 0;

        for (ri = 0; ri < HELPTEXT_LINE_WIDTH; ++ri) {
            letters[helptext_line][ri].x = (short)((LETTER_HALF_WIDTH + LETTER_HALF_PADDING) * ri) + 12;

            if (helptext[helptext_line][text_index] >= 65) {
                letters[helptext_line][ri].letter_offset = (short)((helptext[helptext_line][text_index] - 65) << 4) + (short)((helptext[helptext_line][text_index] - 65) << 3);
            } else if (helptext[helptext_line][text_index] >= 48) {
                /* Digits + colon */
                letters[helptext_line][ri].letter_offset = (short)((helptext[helptext_line][text_index] - 48) << 4) + ((helptext[helptext_line][text_index] - 48) << 3) + 8320;
            } else if (helptext[helptext_line][text_index] == 33) {
                /* Exclamation mark */
                letters[helptext_line][ri].letter_offset = 888;
            } else if (helptext[helptext_line][text_index] == 43) {
                /* Plus sign */
                letters[helptext_line][ri].letter_offset = 7968;
            } else if (helptext[helptext_line][text_index] == 45) {
                /* Minus sign */
                letters[helptext_line][ri].letter_offset = 8016;
            } else {
                /* Space */
                letters[helptext_line][ri].letter_offset = 8112;
            }

            text_index++;
        }
    }

    /* All planes should already be selected at this point */

    while (!(esc_pressed && fadeout_steps_remaining <= 0)) {
        #ifdef USE_TIMER
        ZTimerOn();
        #endif

        cycle_palette(palette, 191, 3);
        cycle_palette(alt_palette, TEXT_PALETTE_COLORS - 1, 195);

        /* Enable this if the water colors should cycle as well. Doesn't look great */
        /*cycle_palette(alt_palette, 191, 3);*/
        /*cycle_palette(palette, TEXT_PALETTE_COLORS - 1, 195);*/

        /* Interrupts are still disabled at this point */

        /* Fade out the letters when escape is pressed */
        if (esc_pressed) {
            #ifndef DO_FADEOUT
            break;
            #endif

            fadeout_step(palette, alt_palette);
            fadeout_steps_remaining -= FADE_OUT_STEP;
        }

        /* Clear page */
        /* All planes should already be selected because of the latch copy at the end of the loop */
        memset(&VGA[non_visible_page], color_offset, UPPER_AREA_PLANE_PIXELS);
        /* The reflection area doesn't need to be filled: it will be completely
         * overwritten with data from the previos frame */

        /* Copy pixels from the other page using latches */
        /* Tell the VGA that all writes are to be done with bits from the latches, and none from the CPU */
        outp(GC_INDEX, 0x08);
        outp(GC_DATA, 0x00);

        if (start_y >= 26) {
            start_y -= 1;
        }

        y = (start_y << 6) + (start_y << 4);

        for (helptext_line = 0; helptext_line < HELPTEXT_NUM_LINES; ++helptext_line) {
            if (y >= REFLECTION_DESTINATION_START) {
                break;
            }

            for (ri = 0; ri < HELPTEXT_LINE_WIDTH; ++ri) {
                letter = letters[helptext_line][ri];

                /* Bitmap -> VGA storage offset is divided by a total of 8: divided by 2 because of half-sized letters, by 4 because of planar system */
                letter_storage_offset = color_storage_offset + (letter.letter_offset >> 3);
                screen_offset = y + (letter.x >> 2);
                /* Each letter needs 3 iterations to be drawn: 12 (letter half width) divided by 4 because we're copying 4 pixels at a time */
                for (i = 0; i < 3; ++i) {
                    screen_memory_offset = non_visible_page + screen_offset;
                    screen_offset++;

                    storage_memory_offset = letter_storage_offset + i;

                    for(j = 0; j < LETTER_HALF_HEIGHT; ++j) {
                        /* Prime VGA memory for latch copying */
                        temp = VGA[storage_memory_offset];
                        VGA[screen_memory_offset] = 0;

                        screen_memory_offset += PLANE_WIDTH;
                        storage_memory_offset += PLANE_WIDTH;
                    }
                }
            }

            /* Start the next lines 16 rows down: 12 for the letters that have been drawn, plus a padding of 4 pixels */
            /*y += PLANE_WIDTH << 4;*/
            y += 1280;
        }

        copy_source = visible_page + REFLECTION_SOURCE_START;
        /* Plus 1 so the distortion shift on the left side doesn't cause pixels to end up on the right side */
        copy_destination = non_visible_page + REFLECTION_DESTINATION_START + 1;

        /* `+ 10` to let the reflection reach the bottom edge of the screen */
        for (y = 0; y < REFLECTION_ROWS + 10; ++y) {
            distortion = y + frame_counter;

            for (x = 0; x < PLANE_WIDTH; x += 2) {
                distortion_plane = distortion_table[distortion++] + copy_source + x;

                temp = VGA[distortion_plane];
                VGA[copy_destination++] = 0;

                temp = VGA[distortion_plane + 1];
                VGA[copy_destination++] = 0;
            }

            /* Skip 8 rows, compresses the reflected image into a smaller area,
             * as if looking at it from an angle */
            /*copy_source -= REFLECTION_ROW_STEP;*/
            copy_source -= 160;
        }

        /* Restore write behaviour: copy data from normal memory/CPU registers */
        outp(GC_DATA, 0xFF);
        /*wait_for_retrace();*/

        flip_pages(&visible_page, &non_visible_page, &high_address, &low_address);

#ifdef VSYNC
        /* Wait for end of current vertical trace(?) */
        while ((inp(INPUT_STATUS) & DISPLAY_ENABLE));
#endif
        disable();
        outport(CRTC_INDEX, high_address);
        outport(CRTC_INDEX, low_address);
#ifdef VSYNC
        /* Wait for start of next vertical trace(?) */
        while (!(inp(INPUT_STATUS) & VRETRACE));
#endif

#ifdef USE_ASM_PALETTE_SWAP
        if ((frame_counter & 1) == 0) {
            color_offset = TEXT_PALETTE_SIZE;
            alt_color_offset = 0;
            color_storage_offset = 2480 + vga_storage_page;

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
            color_storage_offset = vga_storage_page;

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
            /* Even frames */
            color_offset = TEXT_PALETTE_SIZE;
            alt_color_offset = 0;
            color_storage_offset = 2480 + vga_storage_page;

            /* Cycle the rainbow colors of the text */
            outp(PALETTE_WRITE_INDEX, 0);
            for (ci = 0; ci < TEXT_PALETTE_COLORS;) {
                outportb(PALETTE_COLORS, palette[ci++]);
                outportb(PALETTE_COLORS, palette[ci++]);
                outportb(PALETTE_COLORS, palette[ci++]);
            }
        } else {
            /* Odd frames */
            color_offset = 0;
            alt_color_offset = TEXT_PALETTE_SIZE;
            color_storage_offset = vga_storage_page;

            /* Cycle the rainbow colors of the text */
            outp(PALETTE_WRITE_INDEX, 0);
            for (ci = 0; ci < TEXT_PALETTE_COLORS;) {
                outportb(PALETTE_COLORS, alt_palette[ci++]);
                outportb(PALETTE_COLORS, alt_palette[ci++]);
                outportb(PALETTE_COLORS, alt_palette[ci++]);
            }
        }
#endif

        enable();

        ++frame_counter;

        #ifdef USE_TIMER
        ZTimerOff();
        #endif
    }
}

void cracktro(void) {
    BITMAP bmp;
    int sintable[SINTABLE_SIZE];
    int distortion_table[SINTABLE_SIZE];
    word vga_storage_page = 2 * (NUM_PIXELS / 4);
    byte palette_copy[TEXT_PALETTE_COLORS];
    byte alt_palette_copy[TEXT_PALETTE_COLORS];

    /* Pre-calculate two tables: one sin() table that's used for the wavy
     * text, the other for the offset of the water reflections */
    calculate_sintable(sintable, SINTABLE_SIZE);
    calculate_distortion_table(distortion_table, SINTABLE_SIZE);

    bmp.width = BITMAP_WIDTH;
    bmp.height = BITMAP_HEIGHT;
    /* The font is stored in `letters.h` in with Run-length Encoded (RLE)
     * compression, to not overload the compiler with an extremely long header
     * file (and also to keep the size of the .exe. file down a bit) */
    unrle(letters, &bmp);

    disable(); /* Disable interrupts while switching display mode */
    set_mode(VGA_256_COLOR_MODE);
    unchain_vga(VGA);
    set_mode_x();
    enable();
    set_palette();

#ifdef PLAY_MUSIC
    PreparePlayer();
#endif

    /* Store palette for the color cycling in the helptext scene, as the
     * palette will be modified by the fadeout in wavy_text_scene() */
    read_palette(palette_copy, alt_palette_copy);

    /* Copy a smaller version of the font for use in `trainer_help_scene()` so
     * that the whole help text fits on the screen */
    /* Letters for Palette 1 */
    store_small_font_in_vga_memory(bmp, vga_storage_page, 0, 624, 0);
    /* No idea why it needs to be "1274" */
    store_small_font_in_vga_memory(bmp, vga_storage_page + 1274, 624, BITMAP_WIDTH, 0);
    /* Letters for Palette 2 */

    /* Draw the background fill color first as it will be latch-copied as well.
     * "+ 1" to add a single line of margin because the foot of the "4" had a
     * black (i.e. not blue) line
     */
    outport(SC_INDEX, ALL_PLANES);
    memset(&VGA[vga_storage_page + 2480], TEXT_PALETTE_SIZE, (SCREEN_WIDTH * (LETTER_HALF_HEIGHT + 1)) >> 1); /* 320 * 24 * 2 (rows) / 4 planes */
    store_small_font_in_vga_memory(bmp, vga_storage_page + 2480, 0, 624, TEXT_PALETTE_SIZE);
    store_small_font_in_vga_memory(bmp, vga_storage_page + 3754, 624, BITMAP_WIDTH, TEXT_PALETTE_SIZE);

    wavy_text_scene(bmp, sintable, distortion_table);
    esc_pressed = 0;

    /* All planes should already be selected at this point (unless wavy_text_scene() is commented out) */
    /* because wavy_text_scene() while condition is checked at a point where all planes are selected */
    memset(&VGA[visible_page], alt_color_offset, UPPER_AREA_PLANE_PIXELS);
    /* Ensures that the reflection background is fully filled with the water color */
    memset(&VGA[visible_page+UPPER_AREA_PLANE_PIXELS], color_offset, REFLECTION_AREA_PLANE_PIXELS);

    trainer_help_scene(distortion_table, vga_storage_page, palette_copy, alt_palette_copy);

#ifdef PLAY_MUSIC
    StopPlayer();
#endif

    free(bmp.data);

    set_mode(TEXT_MODE);
}

void main() {
    /* Replace the keyboard interrupt handler so we can watch for Esc presses */
    org_kbd_handler = getvect(0x9);
    setvect(KEYBOARD_INT, kbdhandler);

    cracktro();

#ifdef USE_TIMER
    ZTimerReport();
#endif

    /* Restore the original keyboard interrupt handler */
    setvect(KEYBOARD_INT, org_kbd_handler);
}
