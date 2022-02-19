/* BCD16
*  Converts 16-bit binary to decimal numbers (in binary)
*/

module BCD16(
input clk,
input [15:0] bin,
output reg [3:0] tk,
output reg [3:0] k,
output reg [3:0] h,
output reg [3:0] t,
output reg [3:0] s
);

/*
output reg [3:0]  tk;   //10.000 digits
output reg [3:0]  k;    //1.000 digits
output reg [3:0]  h;    //100   digits
output reg [3:0]  t;    //10    digits
output reg [3:0]  s;    //1   digits
*/

//BCD vars
reg [19:0]  bcd;        //unsplitted bcd
reg [4:0]   i;          //iteration var

//Double Dabble algorithm
always @(posedge clk)
  begin
    bcd   = 20'd0;
    s     = 4'd0;
    t     = 4'd0;
    h     = 4'd0;
    k     = 4'd0;
    tk      = 4'd0;
    
    for (i = 5'd0; i < 5'd16; i = i+1'b1) //run for 16 iterations
    begin
       bcd = {bcd[18:0],bin[15-i]}; //concatenation
          
       //if a hex digit of 'bcd' is more than 4, add 3 to it.  
       if(i < 15 && bcd[3:0] > 4) 
          bcd[3:0] = bcd[3:0] + 2'd3;
       if(i < 15 && bcd[7:4] > 4)
          bcd[7:4] = bcd[7:4] + 2'd3;
       if(i < 15 && bcd[11:8] > 4)
          bcd[11:8] = bcd[11:8] + 2'd3;  
       if(i < 15 && bcd[15:12] > 4)
          bcd[15:12] = bcd[15:12] + 2'd3;  
       if(i < 15 && bcd[19:15] > 4)
          bcd[19:15] = bcd[19:15] + 2'd3;  
    end
    
    //split the bcd into digits
    s = bcd [3:0];
    t = bcd [7:4];
    h = bcd [11:8];
    k = bcd [15:12];
    tk = bcd [18:15];
  end     

endmodule