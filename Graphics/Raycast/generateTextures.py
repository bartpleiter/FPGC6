# Script to precalculate dir and plane values, as this keeps them in sync
# However, this means no dynamic adjustments of the FOV,
#  as this is determined by the ratio between dir and plane vectors
#  currently resulting in an FOV of 2*atan(0.66/1.0)=66°

from math import cos, sin, pi, pow
import numpy as np
from PIL import Image

TEXTURE_NAME = "colorstone"

rmap = [0,36,72,109,145,182,218,255]
gmap = [0,36,72,109,145,182,218,255]
bmap = [0,85,170,255]

# print(str(doubleToFP16(planeY)) + ", ", end='')

im = Image.open("Textures/" + TEXTURE_NAME + ".png")
tex_array = np.array(im)

print("{")
for x in range(tex_array.shape[0]):
    for y in range(tex_array.shape[1]):
        r = tex_array[x][y][0]
        g = tex_array[x][y][1]
        b = tex_array[x][y][2]
        rval = min(range(len(rmap)), key=lambda i: abs(rmap[i]-r))
        gval = min(range(len(gmap)), key=lambda i: abs(gmap[i]-g))
        bval = min(range(len(bmap)), key=lambda i: abs(bmap[i]-b))
        print(str(rval*32+gval*4+bval) + ", ", end='')
    print()

print("},")
