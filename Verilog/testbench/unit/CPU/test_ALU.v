`timescale 1ns / 1ps

module test_ALU;

    // Inputs
    reg [31:0] a;
    reg [31:0] b;
    reg [3:0] opcode;

    // Outputs
    wire [31:0] y;

    // Expected output
    reg [31:0] expected_y;

    // Instantiate the ALU
    ALU uut (
        .a(a), 
        .b(b), 
        .opcode(opcode), 
        .y(y)
    );

    initial begin
        // Initialize Inputs
        a = 0;
        b = 0;
        opcode = 0;
        expected_y = 0;

        // Wait 100 ns for global reset to finish
        #100;
        
        // Test OR
        a = 32'hA5A5A5A5;
        b = 32'h5A5A5A5A;
        opcode = 4'b0000;
        expected_y = a | b;
        #10;
        if (y !== expected_y) $display("Test OR failed: y = %h, expected = %h", y, expected_y);
        
        // Test AND
        a = 32'hA5A5A5A5;
        b = 32'h5A5A5A5A;
        opcode = 4'b0001;
        expected_y = a & b;
        #10;
        if (y !== expected_y) $display("Test AND failed: y = %h, expected = %h", y, expected_y);

        // Test XOR
        a = 32'hA5A5A5A5;
        b = 32'h5A5A5A5A;
        opcode = 4'b0010;
        expected_y = a ^ b;
        #10;
        if (y !== expected_y) $display("Test XOR failed: y = %h, expected = %h", y, expected_y);

        // Test ADD
        a = 32'h00000001;
        b = 32'h00000001;
        opcode = 4'b0011;
        expected_y = a + b;
        #10;
        if (y !== expected_y) $display("Test ADD failed: y = %h, expected = %h", y, expected_y);

        // Test SUB
        a = 32'h00000002;
        b = 32'h00000001;
        opcode = 4'b0100;
        expected_y = a - b;
        #10;
        if (y !== expected_y) $display("Test SUB failed: y = %h, expected = %h", y, expected_y);

        // Test SHIFTL
        a = 32'h00000001;
        b = 32'h00000002;
        opcode = 4'b0101;
        expected_y = a << b;
        #10;
        if (y !== expected_y) $display("Test SHIFTL failed: y = %h, expected = %h", y, expected_y);

        // Test SHIFTR
        a = 32'h00000004;
        b = 32'h00000001;
        opcode = 4'b0110;
        expected_y = a >> b;
        #10;
        if (y !== expected_y) $display("Test SHIFTR failed: y = %h, expected = %h", y, expected_y);

        // Test NOTA
        a = 32'hFFFFFFFF;
        b = 32'h00000000;
        opcode = 4'b0111;
        expected_y = ~a;
        #10;
        if (y !== expected_y) $display("Test NOTA failed: y = %h, expected = %h", y, expected_y);

        // Test MULTS
        a = 32'h00000002;
        b = 32'h00000003;
        opcode = 4'b1000;
        expected_y = $signed(a) * $signed(b);
        #10;
        if (y !== expected_y) $display("Test MULTS failed: y = %h, expected = %h", y, expected_y);

        // Test MULTU
        a = 32'h00000002;
        b = 32'h00000003;
        opcode = 4'b1009;
        expected_y = a * b;
        #10;
        if (y !== expected_y) $display("Test MULTU failed: y = %h, expected = %h", y, expected_y);

        // Test SLT
        a = 32'h00000001;
        b = 32'h00000002;
        opcode = 4'b1010;
        expected_y = ($signed(a) < $signed(b)) ? 1 : 0;
        #10;
        if (y !== expected_y) $display("Test SLT failed: y = %h, expected = %h", y, expected_y);

        // Test SLTU
        a = 32'h00000001;
        b = 32'h00000002;
        opcode = 4'b1011;
        expected_y = (a < b) ? 1 : 0;
        #10;
        if (y !== expected_y) $display("Test SLTU failed: y = %h, expected = %h", y, expected_y);

        // Test LOAD
        a = 32'h00000000;
        b = 32'h12345678;
        opcode = 4'b1100;
        expected_y = b;
        #10;
        if (y !== expected_y) $display("Test LOAD failed: y = %h, expected = %h", y, expected_y);

        // Test LOADHI
        a = 32'h00001234;
        b = 32'h56780000;
        opcode = 4'b1101;
        expected_y = a | b;
        #10;
        if (y !== expected_y) $display("Test LOADHI failed: y = %h, expected = %h", y, expected_y);

        // Test SHIFTRS
        a = 32'h80000000;
        b = 32'h00000001;
        opcode = 4'b1110;
        expected_y = $signed(a) >>> b;
        #10;
        if (y !== expected_y) $display("Test SHIFTRS failed: y = %h, expected = %h", y, expected_y);

        // Test FPMULTS
        a = 32'h00010000; // 1.0 in 16.16 fixed point
        b = 32'h00020000; // 2.0 in 16.16 fixed point
        opcode = 4'b1111;
        expected_y = (a * b) >> 16;
        #10;
        if (y !== expected_y) $display("Test FPMULTS failed: y = %h, expected = %h", y, expected_y);

        // Finish simulation
        $finish;
    end
      
endmodule