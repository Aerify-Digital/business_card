from PIL import Image
import sys

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
    print(f'const unsigned char {varname}[] = {{')
    for i, b in enumerate(byte_array):
        print(f'0x{b:02X},', end='')
        if (i + 1) % 16 == 0:
            print()
    print('};')

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: python3 bmp2c.py <image.bmp> <varname>')
    else:
        bmp_to_c_array(sys.argv[1], sys.argv[2])