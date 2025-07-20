import zlib
import struct
import os
import sys

# Constants
MAX_MODULE_SIZE = 16384
CONFIG_TABLE_ADDRESS = 0x00000000
START_ADDRESS = 0x00001000
ADDRESS_ALIGNMENT = 0x1000

# Calculate CRC32
def calculate_function_crc(bin_file, offset, size):
    with open(bin_file, 'rb') as f:
        f.seek(offset)
        data = f.read(size)
    return zlib.crc32(data)

# Generate config table
def generate_config_table(module_files, output_file, max_size=MAX_MODULE_SIZE):
    config = []
    current_address = START_ADDRESS

    for module_id, bin_file, out_file in module_files:
        if not os.path.exists(bin_file):
            print(f"Error: {bin_file} not found")
            sys.exit(1)
        
        size = os.path.getsize(bin_file)
        if size > max_size:
            print(f"Error: Module {bin_file} size ({size} bytes) exceeds max ({max_size} bytes)")
            sys.exit(1)
        
        with open(bin_file, 'rb') as f:
            data = f.read()
        crc = zlib.crc32(data)
        
        func_info = [
            (0, "process_data", 0x10, 256, calculate_function_crc(bin_file, 0x10, 256)),
            (1, "toggle_led", 0x110, 128, calculate_function_crc(bin_file, 0x110, 128))
        ] if module_id == 1 else [
            (0, "other_func", 0x10, 200, calculate_function_crc(bin_file, 0x10, 200))
        ]
        func_count = len(func_info)
        
        print(f"Module {module_id}: Size={size} bytes, Address=0x{current_address:08X}, CRC=0x{crc:08X}, Funcs={func_count}")
        for func_index, func_name, offset, func_size, func_crc in func_info:
            print(f"  Function {func_index}: {func_name}, Offset=0x{offset:04X}, Size={func_size}, CRC=0x{func_crc:08X}")
        
        config.append((module_id, current_address, size, crc, func_count, func_info))
        
        current_address += ((size + ADDRESS_ALIGNMENT - 1) // ADDRESS_ALIGNMENT) * ADDRESS_ALIGNMENT

    with open(output_file, 'wb') as f:
        for module_id, addr, size, crc, func_count, func_info in config:
            f.write(struct.pack('<IIIII', module_id, addr, size, crc, func_count))
            for func_index, func_name, offset, func_size, func_crc in func_info:
                f.write(struct.pack('<I15sIII', func_index, func_name.encode('utf-8'), offset, func_size, func_crc))
        f.write(struct.pack('<IIIII', 0, 0, 0, 0, 0))

    with open('build/module_addresses.mk', 'w') as f:
        for module_id, addr, _, _, _, _ in config:
            f.write(f'MODULE{module_id}_ADDRESS = 0x{addr:08X}\n')

    return config

if __name__ == '__main__':
    module_files = [
        (1, 'build/module1.bin', 'build/module1.out'),
        (2, 'build/module2.bin', 'build/module2.out')
    ]
    generate_config_table(module_files, 'build/config_table.bin')