from PIL import Image
import sys
import os

def bmp_to_c_array(filename, varname):
    img = Image.open(filename).convert('1')
    width, height = img.size
    pixels = img.load()
    byte_array = []
    for y in range(height):
        for x in range(0, width, 8):
            byte = 0
            for bit in range(8):
                if x + bit < width:
                    byte <<= 1
                    if pixels[x + bit, y] == 0:  # 0 = black
                        byte |= 1
                else:
                    byte <<= 1
            byte_array.append(byte)

    # Prepare array data as a string
    array_data = ''
    for i, b in enumerate(byte_array):
        array_data += f'0x{b:02X},'
        if (i + 1) % 16 == 0:
            array_data += '\n'

    # File base name (without extension)
    base = os.path.splitext(os.path.basename(filename))[0]
    header_file = f"{base}.h"
    source_file = f"{base}.cpp"


    # Header content
    header_content = f"""#pragma once
#include <stdint.h>
extern const unsigned char {varname}[];
"""

    # Source content
    source_content = f"""#include "{header_file}"
#include <avr/pgmspace.h>
const unsigned char {varname}[] PROGMEM = {{
    {array_data}
}};
"""

    # Write header file
    with open(header_file, "w") as h:
        h.write(header_content)

    # Write source file
    with open(source_file, "w") as c:
        c.write(source_content)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: python3 bmp2c.py <image.bmp> <varname>')
    else:
        bmp_to_c_array(sys.argv[1], sys.argv[2])