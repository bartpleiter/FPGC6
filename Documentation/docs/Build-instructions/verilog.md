# Verilog build/simulate instructions

To simulate the design, iverilog is used. You might need to compile the latest version yourself.

To run the simulation (in this case a testbench of the entire FPGC), run 

```bash
iverilog -o /home/bart/Documents/FPGA/FPGC6/Verilog/output/output \
  /home/bart/Documents/FPGA/FPGC6/Verilog/testbench/FPGC_tb.v \
  && vvp /home/bart/Documents/FPGA/FPGC6/Verilog/output/output
```

Assuming the testbench generates a `wave.vcd`, run `GTKWave` with this file as argument (or open the file within the program).

Tip: use `ctrl+shft+b` to reload the waveform when overwritten by a new simulation.