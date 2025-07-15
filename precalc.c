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
        /* Row position * 320 pixels */
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

void calculate_distortion_table(int *table, int table_size) {
    int i;
    table[0] = 1;
    for (i = 1; i < table_size; ++i) {
        /*table[i] = (i % 12) >> 2;*/
        /*table[i] = (i % 10) >> 2;*/
        /*table[i] = 4 * (1 + (sin(((PI / 128) * i) * 4)) / 1);*/
        /*table[i] = (int)(2 * (1 + (sin(((PI / 64) * i) * 4)) / 1));*/
        table[i] = 1.5 * (1 + (sin(((PI / 64) * i) * 4)) / 1);
    }
}

void calculate_xoffset_sintable(char *table, int table_size) {
    int i;
    for (i = 0; i < table_size; ++i) {
        table[i] = 5 * (1 + (sin(((PI / 64) * i) * 4)) / 1);
    }
}
