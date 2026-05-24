make: riscv riscv-dissasm

#-mno-relax 
riscv: sphisect_kernel.cpp
	riscv64-unknown-elf-g++ -mno-relax -nostartfiles -emain -Wstack-usage=512 -mabi=lp64f -Ofast -I ../../include main.cpp -o kernel-gcc

riscv-dissasm: riscv
	riscv64-unknown-elf-objdump -d -x kernel-gcc >kernel-gcc.dump

clean:
	rm -f riscv/*