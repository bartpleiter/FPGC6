# Script to precalculate dir and plane values, as this keeps them in sync
# However, this means no dynamic adjustments of the FOV,
#  as this is determined by the ratio between dir and plane vectors
#  currently resulting in an FOV of 2*atan(0.66/1.0)=66°

from math import cos, sin, pi, pow

functionsToCreateTableOf = ["dirX", "dirY", "planeX", "planeY"]


dirX = -1.0
dirY = 0.0
planeX = 0.0
planeY = 0.66

def doubleToFP16(x):
    return round(x * pow(2,16))

# Original code quickly converted to python
#rotate to the right
def moveRight(rotSpeed, function):
    global dirX
    global dirY
    global planeX
    global planeY

    #both camera direction and camera plane must be rotated
    oldDirX = dirX
    dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed)
    dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed)

    oldPlaneX = planeX
    planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed)
    planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed)

    #print("dx:{:.5f} dy:{:.5f} px:{:.5f} py:{:.5f}".format(dirX, dirY, planeX, planeY) )

    if function == "dirX":
        print(str(doubleToFP16(dirX)) + ", ", end='')

    elif function == "dirY":
        print(str(doubleToFP16(dirY)) + ", ", end='')

    elif function == "planeX":
        print(str(doubleToFP16(planeX)) + ", ", end='')

    elif function == "planeY":
        print(str(doubleToFP16(planeY)) + ", ", end='')

# pi/180 means 360 even steps -> one per degree (so use 360 as looplength)
rotSpeed = pi/720
loopLength = 1440

# Create lookup table for each function
for function in functionsToCreateTableOf:
    # reset to make sure
    dirX = -1.0
    dirY = 0.0
    planeX = 0.0
    planeY = 0.66

    print("word LUT"+function + "[" + str(loopLength) + "] = {")
    for i in range(loopLength):
        moveRight(rotSpeed, function)
        if (i+1) % 12 == 0:
            print()
    print("};")
    print("\n\n")