# Script to precalculate cameraX values

screenWidth = 320

def doubleToFP16(x):
    return round(x * pow(2,16))

print("fixed_point_t cameraX[" + str(screenWidth) + "] = {")
for i in range(screenWidth):
    print(str(doubleToFP16(2*i / (screenWidth-1) - 1)) + ", ", end='')
print("\n};\n")