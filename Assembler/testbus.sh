#!/bin/bash

iverilog -o /home/bart/Documents/FPGA/FPGC6/Verilog/output/output /home/bart/Documents/FPGA/FPGC6/Verilog/testbench/busProtocol_tb.v
vvp /home/bart/Documents/FPGA/FPGC6/Verilog/output/output
