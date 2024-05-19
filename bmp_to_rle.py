import struct


def load_bmp(filename: str) -> list[int]:
    with open(filename, "rb") as bmp:
        bmp.seek(2 + 16)
        width, height, num_colors = struct.unpack("<h2xh22xh", bmp.read(30))

        # Assume 8-bit BMP
        if num_colors == 0:
            num_colors = 256

        bmp.seek(bmp.tell() + (num_colors * 4) + 6)

        data = [0] * (width * height)
        y = 1
        for index in range((height - 1) * width, -1, -width):
            for x in range(width):
                # data[index + x] = ord(bmp.read(1).decode("ascii"))
                byte = ord(bmp.read(1).decode("ascii"))
                if byte > 0:
                    # data[index + x] = y
                    data[index + x] = 1
                else:
                    data[index + x] = 0

            y += 1

        return data


def rle(uncompressed: list[int]):
    compressed = []

    duplicate_byte = None
    duplicate_count = 0
    for byte in uncompressed:
        if byte != duplicate_byte:
            if duplicate_count > 0:
                compressed.append(duplicate_count)
                compressed.append(duplicate_byte)

            duplicate_byte = byte
            duplicate_count = 1
        else:
            duplicate_count += 1

    if duplicate_count > 0:
        compressed.append(duplicate_count)
        compressed.append(duplicate_byte)

    # Insert a hint for the uncompressed and compressed length
    compressed.insert(0, len(uncompressed))
    compressed.insert(1, len(compressed))

    return compressed


def unrle(compressed: list[int]):
    uncompressed = []

    for i in range(2, len(compressed), 2):
        duplicate_count = compressed[i]
        duplicate_byte = compressed[i + 1]

        uncompressed.extend([duplicate_byte] * duplicate_count)

    return uncompressed


# uncompressed = [0, 0, 42, 69, 69, 69, 69, 0, 18]
# print(rle(uncompressed))

bmp = load_bmp("letters.bmp")
print("Uncompressed length:", len(bmp))
bmp_rle = rle(bmp)
print("Compressed length:", len(bmp_rle))

print(unrle(bmp_rle) == bmp)

with open("letters.h", "w") as f:
    f.write(f"#define LETTERS_UNCOMPRESSED {bmp_rle[0]}\r\n")
    f.write(f"#define LETTERS_COMPRESSED {bmp_rle[1]}\r\n")
    f.write("unsigned char letters[] = {")
    f.write(",".join([str(n) for n in bmp_rle[2:]]))
    f.write("};\r\n")
