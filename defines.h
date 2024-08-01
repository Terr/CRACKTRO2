#define NUM_COLORS 256
#define TEXT_PALETTE_SIZE 64
#define TEXT_PALETTE_ANGLE 5.71428
/* 2 * TEXT_PALETTE_SIZE * 3 */
#define TEXT_PALETTE_COLORS 384
#define COLOR_WATER 115 >> 2

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define NUM_PIXELS 76800
#define SCREEN_DEPTH 64
#define PLANE_WIDTH 80

#define PI 3.14159
#define SINTABLE_SIZE 256
#define ZTABLE_FIXED_FRAC 6

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

#define LETTER_HALF_WIDTH 12
#define LETTER_HALF_HEIGHT 12
/* A padding of 2 would look nicer but latch copying needs to start at multiples of 4 */
#define LETTER_HALF_PADDING 4

#define TEXT_Y_OFFSET 20
/*#define WIGGLE 10*/
#define WIGGLE 60

#define REFLECTION_ROWS 42
/*#define REFLECTION_ROW_STEP 6 * PLANE_WIDTH*/
#define REFLECTION_ROW_STEP 320
/*#define REFLECTION_SOURCE_START (SCREEN_HEIGHT - REFLECTION_ROWS - 20) * PLANE_WIDTH*/
#define REFLECTION_SOURCE_START 14400
/*#define REFLECTION_DESTINATION_START (SCREEN_HEIGHT - REFLECTION_ROWS - 12) * PLANE_WIDTH*/
#define REFLECTION_DESTINATION_START 15040

/* (Screen height - reflection rows - margin) * pixels per plane */
/*#define UPPER_AREA_PLANE_PIXELS (SCREEN_HEIGHT - REFLECTION_ROWS - 20) * PLANE_WIDTH*/
#define UPPER_AREA_PLANE_PIXELS 15040
/* (Screen height - (Screen height - reflection rows - margin)) * pixels per plane */
#define REFLECTION_AREA_PLANE_PIXELS 4160

#define FADE_OUT_STEP 3
