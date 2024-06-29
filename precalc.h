#ifndef PRECALC_H
#define PRECALC_H

void calculate_sintable(int *table, int table_size);
void calculate_ztable(short *table, int table_size);
void calculate_distortion_table(int *table, int table_size);
void calculate_xoffset_sintable(char *table, int table_size);

#endif
