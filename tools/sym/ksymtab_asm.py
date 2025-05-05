import sys
from elftools.elf.elffile import ELFFile

def find_string_at_address(elffile, address):
    for section in elffile.iter_sections():
        start = section['sh_addr']
        end = start + section['sh_size']
        if start <= address < end:
            offset = address - start
            data = section.data()
            if offset >= len(data):
                continue
            s = []
            while offset < len(data):
                byte = data[offset]
                if byte == 0:
                    break
                s.append(chr(byte))
                offset += 1
            return ''.join(s)
    return None

def process_elf(input_file, output_file):
    with open(input_file, 'rb') as f_in, open(output_file, 'w') as f_out:
        elf = ELFFile(f_in)
        ksymtab = elf.get_section_by_name('__ksymtab')
        if not ksymtab:
            print("Error: __ksymtab section not found")
            sys.exit(1)

        entry_size = 16  # 64位系统下 packed kernel_symbol 的大小
        data = ksymtab.data()
        num_entries = len(data) // entry_size

        # f_out.write(".section .text\n")
        for i in range(num_entries):
            entry = data[i*entry_size:(i+1)*entry_size]
            name_ptr = int.from_bytes(entry[0:8], byteorder='little')
            addr = int.from_bytes(entry[8:16], byteorder='little')

            name = find_string_at_address(elf, name_ptr)
            if not name:
                print(f"Warning: Could not find name for address 0x{name_ptr:x}")
                continue

            f_out.write(f".global {name}\n")
            # f_out.write(f"{name}:\n")
            # f_out.write(f"    unimp\n")
            # f_out.write(f"{name} = 0x{addr:x}\n")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input_kernel.elf> <output.S>")
        sys.exit(1)
    process_elf(sys.argv[1], sys.argv[2])
