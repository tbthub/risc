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

        f_out.write(".section .text\n")
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


# import sys
# from elftools.elf.elffile import ELFFile

# def process_elf(input_file, output_s, output_so):
#     with open(input_file, 'rb') as f_in, open(output_s, 'w') as f_asm:
#         elf = ELFFile(f_in)
        
#         # 定位 __ksymtab 段
#         ksymtab = elf.get_section_by_name('__ksymtab')
#         if not ksymtab:
#             print("Error: __ksymtab section not found")
#             sys.exit(1)

#         # 获取字符串表（假设符号名存储在 .strtab）
#         strtab = elf.get_section_by_name('.strtab')
#         if not strtab:
#             print("Error: .strtab section not found")
#             sys.exit(1)

#         # 遍历 __ksymtab 中的每个 kernel_symbol 结构体
#         entry_size = 16  # packed kernel_symbol 结构体大小 (8 + 8)
#         data = ksymtab.data()
#         num_entries = len(data) // entry_size

#         for i in range(num_entries):
#             entry_offset = i * entry_size
#             entry = data[entry_offset : entry_offset + entry_size]

#             # 提取 name 指针（在 ELF 中实际是字符串表偏移量）
#             name_offset = int.from_bytes(entry[0:8], byteorder='little')
#             # 提取 addr（编译时不需要，但可以保留信息）
#             symbol_addr = int.from_bytes(entry[8:16], byteorder='little')

#             # 从字符串表获取符号名
#             name = strtab.get_string(name_offset)
#             if not name:
#                 print(f"Warning: Invalid name offset 0x{name_offset:x}")
#                 continue

#             # 生成汇编声明（不绑定地址）
#             f_asm.write(f".global {name}\n")
#             f_asm.write(f".type {name}, @function\n")  # 声明为函数类型

#     # # 编译为动态库（允许未定义符号）
#     # import subprocess
#     # cmd = [
#     #     "gcc", "-shared", "-fPIC", "-nostdlib",
#     #     "-Wl,--allow-shlib-undefined",  # 允许动态库未定义符号
#     #     "-x", "assembler", "-o", output_so, output_s
#     # ]
#     # subprocess.run(cmd, check=True)

# if __name__ == '__main__':
#     if len(sys.argv) != 4:
#         print(f"Usage: {sys.argv[0]} <input_kernel.elf> <output.S> <output.so>")
#         sys.exit(1)
#     process_elf(sys.argv[1], sys.argv[2], sys.argv[3])
