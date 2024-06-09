#include <math.h>

#include "defines.h"

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
        /*table[i] = (1 + (sin((PI / 256) * i)) / 1);*/
    }
}


