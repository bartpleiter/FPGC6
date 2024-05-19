# Build script for assembly files for simulation in verilog (run from SRAM)
if (python3 Assembler.py bdos 0x000000 > ../Verilog/memory/sram.list) # compile and write to verilog memory folder
    then
        iverilog -o /home/bart/Documents/FPGA/FPGC6/Verilog/output/output /home/bart/Documents/FPGA/FPGC6/Verilog/testbench/FPGC_100MHz_tb.v
        vvp /home/bart/Documents/FPGA/FPGC6/Verilog/output/output
    else
        # print the error, which is in code.list
        (cat ../Verilog/memory/sram.list)
fi