/*
* Contains three tables for PS/2 scan codes
* Each table is 256 words long to account for all possible scan code
* One for shifted, one for non-shifted
* and one for extended
*/

void DATA_PS2SCANCODE_NORMAL(){
asm(
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 9 96 0\n"
".dw 0 0 0 0 0 113 49 0 0 0 122 115 97 119 50 0\n"
".dw 0 99 120 100 101 52 51 0 0 32 118 102 116 114 53 0\n"
".dw 0 110 98 104 103 121 54 0 0 44 109 106 117 55 56 0\n"
".dw 0 44 107 105 111 48 57 0 0 46 47 108 59 112 45 0\n"
".dw 0 0 39 0 91 61 0 0 0 0 10 93 0 92 0 0\n"
".dw 0 60 0 0 0 0 8 0 0 49 0 52 55 0 0 0\n"
".dw 48 46 50 53 54 56 27 0 0 43 51 45 42 57 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
);
}

void DATA_PS2SCANCODE_SHIFTED(){
asm(
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 9 126 0\n"
".dw 0 0 0 0 0 81 33 0 0 0 90 83 65 87 64 0\n"
".dw 0 67 88 68 69 36 35 0 0 32 86 70 84 82 37 0\n"
".dw 0 78 66 72 71 89 94 0 0 59 77 74 85 38 42 0\n"
".dw 0 60 75 73 79 41 40 0 0 62 63 76 58 80 95 0\n"
".dw 0 0 34 0 123 43 0 0 0 0 10 125 0 124 0 0\n"
".dw 0 62 0 0 0 0 8 0 0 49 0 52 55 0 0 0\n"
".dw 48 46 50 53 54 56 27 0 0 43 51 45 42 57 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
);
}

/* vritual codes:
- left      256
- right     257
- up        258
- down      259
- insert    260
- home      261
- pageup    262
- end       263
- pagedown  264
*/
void DATA_PS2SCANCODE_EXTENDED(){
asm(
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 47 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 10 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 263 0 256 261 0 0 0\n"
".dw 260 127 259 0 257 258 0 0 0 0 264 0 0 262 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
".dw 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
);
}