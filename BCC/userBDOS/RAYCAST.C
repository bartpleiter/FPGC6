/*
* Raycaster, based on lodev engine tutorial
*/

#define word char

#include "LIB/MATH.C"
#include "LIB/STDLIB.C"
#include "LIB/SYS.C"
#include "LIB/GFX.C"
#include "LIB/FP.C"

#define screenWidth 320
#define screenHeight 240
#define mapWidth 24
#define mapHeight 24

#define BTN_LEFT 256
#define BTN_RIGHT 257
#define BTN_UP 258
#define BTN_DOWN 259

// Colors
#define COLOR_RED         224
#define COLOR_DARK_RED    96
#define COLOR_GREEN       28
#define COLOR_DARK_GREEN  12
#define COLOR_BLUE        3
#define COLOR_DARK_BLUE   2
#define COLOR_WHITE       0xFF
#define COLOR_GREY        0xB6
#define COLOR_YELLOW      0xFC
#define COLOR_DARK_YELLOW 0x90

#define FB_ADDR 0xD00000

// Framebuffer. fb[Y][X] (bottom right is [239][319])
char (*fb)[screenWidth] = (char (*)[screenWidth]) FB_ADDR;

word quitGame = 0;

word LUTdirX[360] = {
-65526, -65496, -65446, -65376, -65287, -65177, -65048, -64898, -64729, -64540, -64332, -64104, 
-63856, -63589, -63303, -62997, -62672, -62328, -61966, -61584, -61183, -60764, -60326, -59870, 
-59396, -58903, -58393, -57865, -57319, -56756, -56175, -55578, -54963, -54332, -53684, -53020, 
-52339, -51643, -50931, -50203, -49461, -48703, -47930, -47143, -46341, -45525, -44695, -43852, 
-42995, -42126, -41243, -40348, -39441, -38521, -37590, -36647, -35693, -34729, -33754, -32768, 
-31772, -30767, -29753, -28729, -27697, -26656, -25607, -24550, -23486, -22415, -21336, -20252, 
-19161, -18064, -16962, -15855, -14742, -13626, -12505, -11380, -10252, -9121, -7987, -6850, 
-5712, -4572, -3430, -2287, -1144, 0, 1144, 2287, 3430, 4572, 5712, 6850, 
7987, 9121, 10252, 11380, 12505, 13626, 14742, 15855, 16962, 18064, 19161, 20252, 
21336, 22415, 23486, 24550, 25607, 26656, 27697, 28729, 29753, 30767, 31772, 32768, 
33754, 34729, 35693, 36647, 37590, 38521, 39441, 40348, 41243, 42126, 42995, 43852, 
44695, 45525, 46341, 47143, 47930, 48703, 49461, 50203, 50931, 51643, 52339, 53020, 
53684, 54332, 54963, 55578, 56175, 56756, 57319, 57865, 58393, 58903, 59396, 59870, 
60326, 60764, 61183, 61584, 61966, 62328, 62672, 62997, 63303, 63589, 63856, 64104, 
64332, 64540, 64729, 64898, 65048, 65177, 65287, 65376, 65446, 65496, 65526, 65536, 
65526, 65496, 65446, 65376, 65287, 65177, 65048, 64898, 64729, 64540, 64332, 64104, 
63856, 63589, 63303, 62997, 62672, 62328, 61966, 61584, 61183, 60764, 60326, 59870, 
59396, 58903, 58393, 57865, 57319, 56756, 56175, 55578, 54963, 54332, 53684, 53020, 
52339, 51643, 50931, 50203, 49461, 48703, 47930, 47143, 46341, 45525, 44695, 43852, 
42995, 42126, 41243, 40348, 39441, 38521, 37590, 36647, 35693, 34729, 33754, 32768, 
31772, 30767, 29753, 28729, 27697, 26656, 25607, 24550, 23486, 22415, 21336, 20252, 
19161, 18064, 16962, 15855, 14742, 13626, 12505, 11380, 10252, 9121, 7987, 6850, 
5712, 4572, 3430, 2287, 1144, 0, -1144, -2287, -3430, -4572, -5712, -6850, 
-7987, -9121, -10252, -11380, -12505, -13626, -14742, -15855, -16962, -18064, -19161, -20252, 
-21336, -22415, -23486, -24550, -25607, -26656, -27697, -28729, -29753, -30767, -31772, -32768, 
-33754, -34729, -35693, -36647, -37590, -38521, -39441, -40348, -41243, -42126, -42995, -43852, 
-44695, -45525, -46341, -47143, -47930, -48703, -49461, -50203, -50931, -51643, -52339, -53020, 
-53684, -54332, -54963, -55578, -56175, -56756, -57319, -57865, -58393, -58903, -59396, -59870, 
-60326, -60764, -61183, -61584, -61966, -62328, -62672, -62997, -63303, -63589, -63856, -64104, 
-64332, -64540, -64729, -64898, -65048, -65177, -65287, -65376, -65446, -65496, -65526, -65536
};



word LUTdirY[360] = {
1144, 2287, 3430, 4572, 5712, 6850, 7987, 9121, 10252, 11380, 12505, 13626, 
14742, 15855, 16962, 18064, 19161, 20252, 21336, 22415, 23486, 24550, 25607, 26656, 
27697, 28729, 29753, 30767, 31772, 32768, 33754, 34729, 35693, 36647, 37590, 38521, 
39441, 40348, 41243, 42126, 42995, 43852, 44695, 45525, 46341, 47143, 47930, 48703, 
49461, 50203, 50931, 51643, 52339, 53020, 53684, 54332, 54963, 55578, 56175, 56756, 
57319, 57865, 58393, 58903, 59396, 59870, 60326, 60764, 61183, 61584, 61966, 62328, 
62672, 62997, 63303, 63589, 63856, 64104, 64332, 64540, 64729, 64898, 65048, 65177, 
65287, 65376, 65446, 65496, 65526, 65536, 65526, 65496, 65446, 65376, 65287, 65177, 
65048, 64898, 64729, 64540, 64332, 64104, 63856, 63589, 63303, 62997, 62672, 62328, 
61966, 61584, 61183, 60764, 60326, 59870, 59396, 58903, 58393, 57865, 57319, 56756, 
56175, 55578, 54963, 54332, 53684, 53020, 52339, 51643, 50931, 50203, 49461, 48703, 
47930, 47143, 46341, 45525, 44695, 43852, 42995, 42126, 41243, 40348, 39441, 38521, 
37590, 36647, 35693, 34729, 33754, 32768, 31772, 30767, 29753, 28729, 27697, 26656, 
25607, 24550, 23486, 22415, 21336, 20252, 19161, 18064, 16962, 15855, 14742, 13626, 
12505, 11380, 10252, 9121, 7987, 6850, 5712, 4572, 3430, 2287, 1144, 0, 
-1144, -2287, -3430, -4572, -5712, -6850, -7987, -9121, -10252, -11380, -12505, -13626, 
-14742, -15855, -16962, -18064, -19161, -20252, -21336, -22415, -23486, -24550, -25607, -26656, 
-27697, -28729, -29753, -30767, -31772, -32768, -33754, -34729, -35693, -36647, -37590, -38521, 
-39441, -40348, -41243, -42126, -42995, -43852, -44695, -45525, -46341, -47143, -47930, -48703, 
-49461, -50203, -50931, -51643, -52339, -53020, -53684, -54332, -54963, -55578, -56175, -56756, 
-57319, -57865, -58393, -58903, -59396, -59870, -60326, -60764, -61183, -61584, -61966, -62328, 
-62672, -62997, -63303, -63589, -63856, -64104, -64332, -64540, -64729, -64898, -65048, -65177, 
-65287, -65376, -65446, -65496, -65526, -65536, -65526, -65496, -65446, -65376, -65287, -65177, 
-65048, -64898, -64729, -64540, -64332, -64104, -63856, -63589, -63303, -62997, -62672, -62328, 
-61966, -61584, -61183, -60764, -60326, -59870, -59396, -58903, -58393, -57865, -57319, -56756, 
-56175, -55578, -54963, -54332, -53684, -53020, -52339, -51643, -50931, -50203, -49461, -48703, 
-47930, -47143, -46341, -45525, -44695, -43852, -42995, -42126, -41243, -40348, -39441, -38521, 
-37590, -36647, -35693, -34729, -33754, -32768, -31772, -30767, -29753, -28729, -27697, -26656, 
-25607, -24550, -23486, -22415, -21336, -20252, -19161, -18064, -16962, -15855, -14742, -13626, 
-12505, -11380, -10252, -9121, -7987, -6850, -5712, -4572, -3430, -2287, -1144, 0
};



word LUTplaneX[360] = {
755, 1510, 2264, 3017, 3770, 4521, 5271, 6020, 6766, 7511, 8253, 8993, 
9730, 10464, 11195, 11922, 12646, 13366, 14082, 14794, 15501, 16203, 16901, 17593, 
18280, 18961, 19637, 20306, 20970, 21627, 22277, 22921, 23558, 24187, 24809, 25424, 
26031, 26630, 27220, 27803, 28377, 28942, 29499, 30047, 30585, 31114, 31634, 32144, 
32644, 33134, 33614, 34084, 34544, 34993, 35431, 35859, 36276, 36681, 37076, 37459, 
37831, 38191, 38539, 38876, 39201, 39514, 39815, 40104, 40381, 40645, 40897, 41137, 
41364, 41578, 41780, 41969, 42145, 42309, 42459, 42597, 42721, 42833, 42931, 43017, 
43089, 43148, 43194, 43227, 43247, 43254, 43247, 43227, 43194, 43148, 43089, 43017, 
42931, 42833, 42721, 42597, 42459, 42309, 42145, 41969, 41780, 41578, 41364, 41137, 
40897, 40645, 40381, 40104, 39815, 39514, 39201, 38876, 38539, 38191, 37831, 37459, 
37076, 36681, 36276, 35859, 35431, 34993, 34544, 34084, 33614, 33134, 32644, 32144, 
31634, 31114, 30585, 30047, 29499, 28942, 28377, 27803, 27220, 26630, 26031, 25424, 
24809, 24187, 23558, 22921, 22277, 21627, 20970, 20306, 19637, 18961, 18280, 17593, 
16901, 16203, 15501, 14794, 14082, 13366, 12646, 11922, 11195, 10464, 9730, 8993, 
8253, 7511, 6766, 6020, 5271, 4521, 3770, 3017, 2264, 1510, 755, 0, 
-755, -1510, -2264, -3017, -3770, -4521, -5271, -6020, -6766, -7511, -8253, -8993, 
-9730, -10464, -11195, -11922, -12646, -13366, -14082, -14794, -15501, -16203, -16901, -17593, 
-18280, -18961, -19637, -20306, -20970, -21627, -22277, -22921, -23558, -24187, -24809, -25424, 
-26031, -26630, -27220, -27803, -28377, -28942, -29499, -30047, -30585, -31114, -31634, -32144, 
-32644, -33134, -33614, -34084, -34544, -34993, -35431, -35859, -36276, -36681, -37076, -37459, 
-37831, -38191, -38539, -38876, -39201, -39514, -39815, -40104, -40381, -40645, -40897, -41137, 
-41364, -41578, -41780, -41969, -42145, -42309, -42459, -42597, -42721, -42833, -42931, -43017, 
-43089, -43148, -43194, -43227, -43247, -43254, -43247, -43227, -43194, -43148, -43089, -43017, 
-42931, -42833, -42721, -42597, -42459, -42309, -42145, -41969, -41780, -41578, -41364, -41137, 
-40897, -40645, -40381, -40104, -39815, -39514, -39201, -38876, -38539, -38191, -37831, -37459, 
-37076, -36681, -36276, -35859, -35431, -34993, -34544, -34084, -33614, -33134, -32644, -32144, 
-31634, -31114, -30585, -30047, -29499, -28942, -28377, -27803, -27220, -26630, -26031, -25424, 
-24809, -24187, -23558, -22921, -22277, -21627, -20970, -20306, -19637, -18961, -18280, -17593, 
-16901, -16203, -15501, -14794, -14082, -13366, -12646, -11922, -11195, -10464, -9730, -8993, 
-8253, -7511, -6766, -6020, -5271, -4521, -3770, -3017, -2264, -1510, -755, 0
};



word LUTplaneY[360] = {
43247, 43227, 43194, 43148, 43089, 43017, 42931, 42833, 42721, 42597, 42459, 42309, 
42145, 41969, 41780, 41578, 41364, 41137, 40897, 40645, 40381, 40104, 39815, 39514, 
39201, 38876, 38539, 38191, 37831, 37459, 37076, 36681, 36276, 35859, 35431, 34993, 
34544, 34084, 33614, 33134, 32644, 32144, 31634, 31114, 30585, 30047, 29499, 28942, 
28377, 27803, 27220, 26630, 26031, 25424, 24809, 24187, 23558, 22921, 22277, 21627, 
20970, 20306, 19637, 18961, 18280, 17593, 16901, 16203, 15501, 14794, 14082, 13366, 
12646, 11922, 11195, 10464, 9730, 8993, 8253, 7511, 6766, 6020, 5271, 4521, 
3770, 3017, 2264, 1510, 755, 0, -755, -1510, -2264, -3017, -3770, -4521, 
-5271, -6020, -6766, -7511, -8253, -8993, -9730, -10464, -11195, -11922, -12646, -13366, 
-14082, -14794, -15501, -16203, -16901, -17593, -18280, -18961, -19637, -20306, -20970, -21627, 
-22277, -22921, -23558, -24187, -24809, -25424, -26031, -26630, -27220, -27803, -28377, -28942, 
-29499, -30047, -30585, -31114, -31634, -32144, -32644, -33134, -33614, -34084, -34544, -34993, 
-35431, -35859, -36276, -36681, -37076, -37459, -37831, -38191, -38539, -38876, -39201, -39514, 
-39815, -40104, -40381, -40645, -40897, -41137, -41364, -41578, -41780, -41969, -42145, -42309, 
-42459, -42597, -42721, -42833, -42931, -43017, -43089, -43148, -43194, -43227, -43247, -43254, 
-43247, -43227, -43194, -43148, -43089, -43017, -42931, -42833, -42721, -42597, -42459, -42309, 
-42145, -41969, -41780, -41578, -41364, -41137, -40897, -40645, -40381, -40104, -39815, -39514, 
-39201, -38876, -38539, -38191, -37831, -37459, -37076, -36681, -36276, -35859, -35431, -34993, 
-34544, -34084, -33614, -33134, -32644, -32144, -31634, -31114, -30585, -30047, -29499, -28942, 
-28377, -27803, -27220, -26630, -26031, -25424, -24809, -24187, -23558, -22921, -22277, -21627, 
-20970, -20306, -19637, -18961, -18280, -17593, -16901, -16203, -15501, -14794, -14082, -13366, 
-12646, -11922, -11195, -10464, -9730, -8993, -8253, -7511, -6766, -6020, -5271, -4521, 
-3770, -3017, -2264, -1510, -755, 0, 755, 1510, 2264, 3017, 3770, 4521, 
5271, 6020, 6766, 7511, 8253, 8993, 9730, 10464, 11195, 11922, 12646, 13366, 
14082, 14794, 15501, 16203, 16901, 17593, 18280, 18961, 19637, 20306, 20970, 21627, 
22277, 22921, 23558, 24187, 24809, 25424, 26031, 26630, 27220, 27803, 28377, 28942, 
29499, 30047, 30585, 31114, 31634, 32144, 32644, 33134, 33614, 34084, 34544, 34993, 
35431, 35859, 36276, 36681, 37076, 37459, 37831, 38191, 38539, 38876, 39201, 39514, 
39815, 40104, 40381, 40645, 40897, 41137, 41364, 41578, 41780, 41969, 42145, 42309, 
42459, 42597, 42721, 42833, 42931, 43017, 43089, 43148, 43194, 43227, 43247, 43254
};

word worldMap[mapWidth][mapHeight]=
{
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,2,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,3,0,0,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,0,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,5,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,5,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Render vertical line in pixel plane
// INPUT:
//   r4 = x (which vertical line)
//   r5 = y when to start drawing line
//   r6 = y when to stop drawing line
//   r7 = color of line
void RAYFX_drawVertLine(word x, word start, word end, char color)
{
  // reg 4 5 6 7 and 2 (retval) are safe
  asm(
  "; backup registers\n"
  "push r9\n"
  );

  asm(

  "load32 0xD00000 r9   ; r9 = framebuffer addr\n"
  "add r9 r4 r4         ; r4 = first pixel in line\n"

  "multu r5 320 r9      ; r9 = start with line offset\n"
  "add r9 r4 r5         ; r5 = fb addr of start\n"
  
  "multu r6 320 r9      ; r9 = end with line offset\n"
  "add r9 r4 r6         ; r6 = fb addr of start\n"

  "load 239 r2          ; r2 = y endloop\n"
  "multu r2 320 r9      ; r9 = start line offset\n"
  "add r9 r4 r2         ; r2 = fb addr of final pixel\n"

  "; draw until start\n"
  "RAYFX_drawVlineLoopCeiling:\n"
  "  write 0 r4 r0     ; write black pixel\n"
  "  add r4 320 r4     ; go to next line pixel\n"

  "  bge r4 r5 2       ; keep looping until reached start\n"
  "  jump RAYFX_drawVlineLoopCeiling\n"

  "; draw until end\n"
  "RAYFX_drawVlineLoopWall:\n"
  "  write 0 r4 r7     ; write color pixel\n"
  "  add r4 320 r4     ; go to next line pixel\n"

  "  bge r4 r6 2       ; keep looping until reached end\n"
  "  jump RAYFX_drawVlineLoopWall\n"


  "; draw until final pixel\n"
  "RAYFX_drawVlineLoopFloor:\n"
  "  write 0 r4 r0     ; write black pixel\n"
  "  add r4 320 r4     ; go to next line pixel\n"

  "  bge r4 r2 2       ; keep looping until reached end of screen\n"
  "  jump RAYFX_drawVlineLoopFloor\n"
  );

  asm(
  "; restore registers\n"
  "pop r9\n"
  );

}

int main() 
{
  // clear screen from text
  GFX_clearWindowtileTable();
  GFX_clearWindowpaletteTable();
  GFX_clearBGtileTable();
  GFX_clearBGpaletteTable();

  //x and y start position
  fixed_point_t posX = FP_intToFP(12);
  fixed_point_t posY = FP_intToFP(12);

  //initial direction vector
  fixed_point_t dirX = LUTdirX[0];
  fixed_point_t dirY = LUTdirY[0];

  //the 2d raycaster version of camera plane
  fixed_point_t planeX = LUTplaneX[0];
  fixed_point_t planeY = LUTplaneY[0];

  // rotation angle (loops at 360)
  word rotationAngle = 0;
  word rotationSpeed = 3; // degrees per frame

  fixed_point_t moveSpeed = FP_StringToFP("0.15");

  while(!quitGame)
  {
    word x;
    for(x = 0; x < screenWidth; x++)
    {
      //calculate ray position and direction
      fixed_point_t cameraX = FP_Div(FP_intToFP(x<<1), FP_intToFP(screenWidth)) - FP_intToFP(1); //x-coordinate in camera space

      fixed_point_t rayDirX = dirX + FP_Mult(planeX, cameraX);
      fixed_point_t rayDirY = dirY + FP_Mult(planeY, cameraX);

      //which box of the map we are in
      word mapX = FP_FPtoInt(posX);
      word mapY = FP_FPtoInt(posY);

      //length of ray from current position to next x or y-side
      fixed_point_t sideDistX;
      fixed_point_t sideDistY;

      //length of ray from one x or y-side to next x or y-side
      //these are derived as:
      //deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX))
      //deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY))
      //which can be simplified to abs(|rayDir| / rayDirX) and abs(|rayDir| / rayDirY)
      //where |rayDir| is the length of the vector (rayDirX, rayDirY). Its length,
      //unlike (dirX, dirY) is not 1, however this does not matter, only the
      //ratio between deltaDistX and deltaDistY matters, due to the way the DDA
      //stepping further below works. So the values can be computed as below.
      // Division through zero is prevented by setting the result to a very high value
      fixed_point_t deltaDistX = (rayDirX == 0) ? 1<<30 : MATH_abs( FP_Div(FP_intToFP(1), rayDirX));
      fixed_point_t deltaDistY = (rayDirY == 0) ? 1<<30 : MATH_abs( FP_Div(FP_intToFP(1), rayDirY));

      fixed_point_t perpWallDist;

      //what direction to step in x or y-direction (either +1 or -1)
      word stepX;
      word stepY;

      word hit = 0; //was there a wall hit?
      word side; //was a NS or a EW wall hit?

      //calculate step and initial sideDist
      if(rayDirX < 0)
      {
        stepX = -1;
        sideDistX = FP_Mult((posX - FP_intToFP(mapX)), deltaDistX);
      }
      else
      {
        stepX = 1;
        sideDistX = FP_Mult((FP_intToFP(mapX + 1) - posX), deltaDistX);
      }
      if(rayDirY < 0)
      {
        stepY = -1;
        sideDistY = FP_Mult((posY - FP_intToFP(mapY)), deltaDistY);
      }
      else
      {
        stepY = 1;
        sideDistY = FP_Mult((FP_intToFP(mapY + 1) - posY), deltaDistY);
      }

      //perform DDA
      while(hit == 0)
      {
        //jump to next map square, either in x-direction, or in y-direction
        if(sideDistX < sideDistY)
        {
          sideDistX += deltaDistX;
          mapX += stepX;
          side = 0;
        }
        else
        {
          sideDistY += deltaDistY;
          mapY += stepY;
          side = 1;
        }
        //Check if ray has hit a wall
        if(worldMap[mapX][mapY] > 0) hit = 1;
      }

      //Calculate distance projected on camera direction. This is the shortest distance from the point where the wall is
      //hit to the camera plane. Euclidean to center camera point would give fisheye effect!
      //This can be computed as (mapX - posX + (1 - stepX) / 2) / rayDirX for side == 0, or same formula with Y
      //for size == 1, but can be simplified to the code below thanks to how sideDist and deltaDist are computed:
      //because they were left scaled to |rayDir|. sideDist is the entire length of the ray above after the multiple
      //steps, but we subtract deltaDist once because one step more into the wall was taken above.
      if(side == 0) perpWallDist = (sideDistX - deltaDistX);
      else          perpWallDist = (sideDistY - deltaDistY);

      //Calculate height of line to draw on screen
      word lineHeight = FP_FPtoInt(FP_Div(FP_intToFP(screenHeight), perpWallDist));

      //calculate lowest and highest pixel to fill in current stripe
      word drawStart = - (lineHeight >> 1) + (screenHeight >> 1);
      if(drawStart < 0) drawStart = 0;
      word drawEnd = (lineHeight >> 1) + (screenHeight >> 1);
      if(drawEnd >= screenHeight) drawEnd = screenHeight - 1;

      //choose wall color
      //give x and y sides different brightness
      char color;
      switch(worldMap[mapX][mapY])
      {
        case 1:  color = (side == 1) ? COLOR_DARK_RED : COLOR_RED;    break;
        case 2:  color = (side == 1) ? COLOR_DARK_GREEN : COLOR_GREEN;    break;
        case 3:  color = (side == 1) ? COLOR_DARK_BLUE : COLOR_BLUE;   break;
        case 4:  color = (side == 1) ? COLOR_GREY : COLOR_WHITE;  break;
        default: color = (side == 1) ? COLOR_DARK_YELLOW : COLOR_YELLOW; break;
      }

      //draw the pixels of the stripe as a vertical line
      // Safity checks beforehand
      // Currently skips first line of frame as it does not render properly
      if (x != 0 && drawStart >= 0 && drawEnd >= drawStart && drawEnd < screenHeight)
      {
        RAYFX_drawVertLine(x, drawStart, drawEnd, color);
      }
    }


    // check which button is held
    if (BDOS_USBkeyHeld(BTN_LEFT))
    {
      //both camera direction and camera plane must be rotated
      rotationAngle -= rotationSpeed;
      if (rotationAngle < 0)
      {
        rotationAngle += 360;
      }
      dirX = LUTdirX[rotationAngle];
      dirY = LUTdirY[rotationAngle];
      planeX = LUTplaneX[rotationAngle];
      planeY = LUTplaneY[rotationAngle];
    }
    else if (BDOS_USBkeyHeld(BTN_RIGHT))
    {
      //both camera direction and camera plane must be rotated
      rotationAngle += rotationSpeed;
      if (rotationAngle >= 360)
      {
        rotationAngle -= 360;
      }
      dirX = LUTdirX[rotationAngle];
      dirY = LUTdirY[rotationAngle];
      planeX = LUTplaneX[rotationAngle];
      planeY = LUTplaneY[rotationAngle];
    }

    if (BDOS_USBkeyHeld(BTN_UP))
    {
      word worldMapX = FP_FPtoInt(posX + FP_Mult(dirX, moveSpeed));
      word worldMapY = FP_FPtoInt(posY);

      if(worldMap[worldMapX][worldMapY] == 0)
      {
        posX += FP_Mult(dirX, moveSpeed);
      }

      worldMapX = FP_FPtoInt(posX);
      worldMapY = FP_FPtoInt(posY + FP_Mult(dirY, moveSpeed));
      if(worldMap[worldMapX][worldMapY] == 0)
      {
        posY += FP_Mult(dirY, moveSpeed);
      }
    }
    else if (BDOS_USBkeyHeld(BTN_DOWN))
    {
      word worldMapX = FP_FPtoInt(posX - FP_Mult(dirX, moveSpeed));
      word worldMapY = FP_FPtoInt(posY);

      if(worldMap[worldMapX][worldMapY] == 0)
      {
        posX -= FP_Mult(dirX, moveSpeed);
      }

      worldMapX = FP_FPtoInt(posX);
      worldMapY = FP_FPtoInt(posY - FP_Mult(dirY, moveSpeed));
      if(worldMap[worldMapX][worldMapY] == 0)
      {
        posY -= FP_Mult(dirY, moveSpeed);
      }
    }

    if (HID_FifoAvailable())
    {
      word c = HID_FifoRead();

      if (c == 27) // escape
      {
        GFX_clearPXframebuffer();
        return 'q';
      }
    }
    
  }

  return 'q';
}

void interrupt()
{
  // handle all interrupts
  word i = getIntID();
  switch(i)
  {
    case INTID_TIMER1:
      timer1Value = 1; // notify ending of timer1
      break;

    case INTID_TIMER2:
      break;

    case INTID_UART0:
      break;

    case INTID_GPU:
      break;

    case INTID_TIMER3:
      break;

    case INTID_PS2:
      break;

    case INTID_UART1:
      break;

    case INTID_UART2:
      break;
  }
}