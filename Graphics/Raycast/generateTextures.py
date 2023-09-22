# Script to precalculate dir and plane values, as this keeps them in sync
# However, this means no dynamic adjustments of the FOV,
#  as this is determined by the ratio between dir and plane vectors
#  currently resulting in an FOV of 2*atan(0.66/1.0)=66°

from math import cos, sin, pi, pow
import numpy as np
from PIL import Image

TEXTURE_NAME = "colorstone"

# print(str(doubleToFP16(planeY)) + ", ", end='')

im = Image.open("Textures/" + TEXTURE_NAME + ".png")
tex_array = np.array(im)

print("{")
for x in range(tex_array.shape[0]):
    for y in range(tex_array.shape[1]):
        r = tex_array[x][y][0]
        g = tex_array[x][y][1]
        b = tex_array[x][y][2]
        print(str(r*2**16+g*2**8+b) + ", ", end='')
    print()

print("},")
