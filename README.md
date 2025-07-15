# CRACKTRO2

A "cracktro" that uses some semi-advanced stuff that you can do with VGA (Mode X so planar drawing, page flipping, latch copies) which are all used to create real-time water reflections.

Drawing in Mode X is complicated because of the 4 "planes" the VGA memory is divided into. And because switching planes is slow, all kinds of extra loops are needed with skips and offsets so the number of plane switches can be kept to the absolute minimum of 4 times per frame. This makes the code very complicated.

This isn't the best code by any means anyway but hopefully it helps someone who's working with DOS VGA graphics. I know from experience that it isn't easy to find the source (in C) of complete, commented programs that use these VGA features. I've refactored the code a little bit to hopefully make it easier to understand.

The two scene functions (`wavy_text_scene()` and (`trainer_help_scene()`) are pretty long. My goal was to make this run well on a low-end 486 so I wanted to remove any overhead for function calls that the Turbo C compiler isn't able to optimize out.

This does make the functions start with a mountain of variable declarations, because in C89 you can only declare variables at the start of a function. This is especially egregious for counter variables because you cannot declare them at the start of a `for` statement.

For this public repository I also modified a few of the commits to remove music files and some timer code that wasn't made by me, as I'm not sure under which license they are distributed. For the music, you can find instructions on where to find compatible files, and how to prepare them for use, below.

This demo works on real hardware. That took some effort as everything seemed fine in DOSBox but it turned out it emulates some things differently.

## Usage

To compile the code you need these things:

* Turbo C 2.01 (can also be compiled with Turbo C++'s compiler, BCC. See `makefile.bcc`)
* Turbo Assembler 5.2
* A music track and a modified `radlib.asm` (see below)

Edit `makefile` and set `LIBPATH` to where your Turbo C library files are. Then, with both tools in your `PATH`, just run `make` to compile everything. 

The output EXE is called `VGA9.EXE`.

## Adding music

The demo uses the Reality Adlib Tracker library v1.1 (RAD) for music, which uses its own tracker format. The music track is bundled into the EXE via an Assembly include. The RAD library includes a tool to convert a RAD file to an Assembly include.

1. The library and tracks can be found over at Reality's website: https://www.3eality.com/Public/.
2. Convert the music file to an include file by running: `DATA.COM track.RAD track.INC`, this program is included in the RAD library
3. Place the INC file in the root of this repository
4. Edit `radlib.asm` around line 199 to have it include the INC file
5. Set `IPS` at around line 9 to either 19 or 50, depending on the speed of the track (you can see this when loading the file in the RAD Music Tracker, `RAD.EXE`)

(Ideally you would set it to 18 for 18.2 Hz music, but that causes a divide by zero error that I haven't been able to fix)

The track I used for the demo is `NEOINTRO.RAD`, which is a 18.2 Hz track.

## Explanation of the water reflection effect

The water reflection is achieved by latch-copying (for speed) the color data of the previous frame, in reverse row-order so that the image is flipped. 

To get the copied pixels to show up in a blue tint in the water, palette swapping is used as changing their colors pixel-by-pixel would be too slow.

Two sets of palettes are kept, each 128 contain colors, dividing the 256 color palette that VGA offers. In each palette, 64 colors are used for the rainbow colors of the text, 64 of the blue-tinted water reflection.

Even frames (frame 2, 4, 6, etc.) use palette 1, odd frames (frame 1, 3, 5, etc.) use palette 2. 

* When an even frame draws, for example, a red pixel for the text, it will use color code 1. On odd frames, the color code 1 is some blue-tinted color.
* When an odd frame places a red pixel, it uses color code 129. Color 129 on even frames is a blue-tinted color.

So because the different frames use different palettes, when the previous frame's color data is copied, what was red then now shows up in a blue-tinted color.

Even with the palette swapping, latch copying is faster than rendering the wavy text twice (once in rainbow colors, once as a reflection), especially when adding the distortion (ripple) effect.

The distortion effect of the water is achieved by applying a small column offset to each row before latch-copying it.

## Note when modifying the code

When modifying the code, remember to save the files with DOS line endings!
