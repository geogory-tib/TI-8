import os
from textwrap import wrap

# Folder containing your .ch8 ROM files
ROM_FOLDER = "roms"
OUTPUT_HEADER = "src/include/roms.h"

# Helper: convert bytes to C hex literals
def bytes_to_c_array(data, line_width=12):
    hex_bytes = [f"0x{b:02X}" for b in data]
    # wrap into lines for readability
    lines = wrap(", ".join(hex_bytes), line_width * 5)  # ~5 chars per byte ("0xXX,")
    return "\n".join(lines)

def generate_header(rom_files):
    lines = []
    lines.append("// Auto-generated CHIP-8 ROM header\n")
    lines.append("#ifndef CHIP8_ROMS_H")
    lines.append("#define CHIP8_ROMS_H\n")
    lines.append('#include "typedefs.h"')
    lines.append("struct rom_t\n{\n const char *name;\n byte *data;\n u16 length;\n};\n")

    rom_struct_names = []

    # Generate each ROM array
    rom_names = []
    for i, rom_file in enumerate(rom_files):
        rom_name = f"rom{i+1}"
        rom_struct_names.append(rom_name)
        with open(rom_file, "rb") as f:
            rom_data = f.read()
        
        lines.append(f"// ROM: {os.path.basename(rom_file)}")
        rom_names.append(os.path.basename(rom_file))
        lines.append(f"byte {rom_name}[] = {{\n    {bytes_to_c_array(rom_data)}\n}};")
        lines.append(f"u16 {rom_name}_len = sizeof({rom_name});\n")

    # Generate array of structs
    lines.append("rom_t roms[] = {")
    i = 0
    for name in rom_struct_names:
        lines.append(f'    {{ "{rom_names[i]}",{name}, sizeof({name}) }},')
        i += 1
    lines.append("};\n")
    lines.append("// To avoid using sprintf rom prompt is made at compile time\n")
    lines.append(f'u16 no_of_rom = {len(rom_names)};\n')
    lines.append("#endif // CHIP8_ROMS_H\n")
    return "\n".join(lines)

def main():
    # Find all .ch8 files in ROM_FOLDER
    rom_files = [os.path.join(ROM_FOLDER, f) for f in os.listdir(ROM_FOLDER) if f.endswith(".ch8")]
    header_content = generate_header(rom_files)

    with open(OUTPUT_HEADER, "w") as f:
        f.write(header_content)
    print(f"Generated header file: {OUTPUT_HEADER}")

if __name__ == "__main__":
    main()
