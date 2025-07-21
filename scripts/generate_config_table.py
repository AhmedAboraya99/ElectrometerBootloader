import struct
import os
import binascii

def parse_map_file(map_file):
    symbols = {}
    if not os.path.exists(map_file):
        print(f"Error: {map_file} not found")
        return symbols
    with open(map_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            if 'process_data' in line or 'read_sensor' in line:
                parts = line.split()
                if len(parts) >= 2:
                    try:
                        symbols[parts[0]] = int(parts[1], 16)
                    except ValueError:
                        continue
    return symbols

def calculate_crc(data):
    return binascii.crc32(data) & 0xFFFFFFFF

def generate_config_table(module1_map, module2_map, module1_bin, module2_bin):
    config_table = []
    
    # Module 1 (UART, GPIO)
    if not os.path.exists(module1_map) or not os.path.exists(module1_bin):
        print(f"Error: {module1_map} or {module1_bin} not found")
        return
    symbols1 = parse_map_file(module1_map)
    with open(module1_bin, 'rb') as f:
        module1_data = f.read()
    
    module1_info = {
        'module_id': 1,
        'address': 0x10000000,  # Virtual address
        'func_count': 2,
        'functions': [
            {'name': 'process_data', 'offset': symbols1.get('process_data', 0x10000000), 'size': 256, 'crc': calculate_crc(module1_data[:256]), 'peripheral': 0x3},  # UART, GPIO
            {'name': 'read_sensor', 'offset': symbols1.get('read_sensor', 0x10000200), 'size': 64, 'crc': calculate_crc(module1_data[256:320]), 'peripheral': 0x3}   # UART, GPIO
        ]
    }
    
    # Module 2 (Timer, ADC)
    if not os.path.exists(module2_map) or not os.path.exists(module2_bin):
        print(f"Error: {module2_map} or {module2_bin} not found")
        return
    symbols2 = parse_map_file(module2_map)
    with open(module2_bin, 'rb') as f:
        module2_data = f.read()
    
    module2_info = {
        'module_id': 2,
        'address': 0x10004000,  # Virtual address
        'func_count': 2,
        'functions': [
            {'name': 'process_data', 'offset': symbols2.get('process_data', 0x10004000), 'size': 256, 'crc': calculate_crc(module2_data[:256]), 'peripheral': 0xC},  # Timer, ADC
            {'name': 'read_sensor', 'offset': symbols2.get('read_sensor', 0x10004200), 'size': 64, 'crc': calculate_crc(module2_data[256:320]), 'peripheral': 0xC}   # Timer, ADC
        ]
    }
    
    config_table.append(module1_info)
    config_table.append(module2_info)
    
    if not os.path.exists('build'):
        os.makedirs('build')
    with open('build/config_table.bin', 'wb') as f:
        for mod in config_table:
            f.write(struct.pack('<I', mod['module_id']))
            f.write(struct.pack('<I', mod['address']))
            f.write(struct.pack('<I', mod['func_count']))
            for func in mod['functions']:
                name = func['name'].encode().ljust(16, b'\0')
                f.write(name)
                f.write(struct.pack('<I', func['offset']))
                f.write(struct.pack('<I', func['size']))
                f.write(struct.pack('<I', func['crc']))
                f.write(struct.pack('<I', func['peripheral']))

if __name__ == '__main__':
    generate_config_table('build/module1.map', 'build/module2.map', 'build/module1.bin', 'build/module2.bin')