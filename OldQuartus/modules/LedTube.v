/* LedTube
*  Shows the digits specified by d1...d8 on the led tubes
*/

module LedTube (
   clk,
   dataout,
   en, 
   d1, 
   d2, 
   d3, 
   d4, 
   d5, 
   d6, 
   d7, 
   d8
);

input wire        clk;     //50MHz clk
output reg [7:0]  dataout; //data to ledtubes
output reg [7:0]  en;      //the ledtubes to enable

input wire [3:0]  d1;      //1st digit
input wire [3:0]  d2;      //2nd digit
input wire [3:0]  d3;      //3rd digit
input wire [3:0]  d4;      //4th digit
input wire [3:0]  d5;      //5th digit
input wire [3:0]  d6;      //6th digit
input wire [3:0]  d7;      //7th digit
input wire [3:0]  d8;      //8th digit

//LedTube vars
reg [15:0]  cnt_scan;      //counter to time updating the ledtubes
reg [4:0]   dataout_buf;   //index for output values


/*Table for creating custom characters
dataout -> "8bit pixel data":
0 -> on, 1 -> off.
   
bits from left to right | position
-----------------------------------
0 -> Dot
1 -> center
2 -> left_top
3 -> left_bottom
-
4 -> bottom
5 -> right_bottom
6 -> right_top
7 -> top


character | byte
------------------
0 -> 8'b1100_0000
1 -> 8'b1111_1001
2 -> 8'b0010_0100
3 -> 8'b1011_0000
4 -> 8'b1001_1001
5 -> 8'b1001_0010
6 -> 8'b1000_0010
7 -> 8'b1111_1000
8 -> 8'b1000_0000
9 -> 8'b1001_1000
*/

always@(posedge clk)
begin
   cnt_scan <= cnt_scan + 1'b1;
end

//select the ledtubes from right to left
always @(cnt_scan)
begin
   case(cnt_scan[15:13])
       3'b000 :
          en = 8'b1111_1110; 
       3'b001 :
          en = 8'b1111_1101;
       3'b010 :
          en = 8'b1111_1011;
       3'b011 :
          en = 8'b1111_0111;
       3'b100 :
          en = 8'b1110_1111;
       3'b101 :
          en = 8'b1101_1111;
       3'b110 :
          en = 8'b1011_1111;
       3'b111 :
          en = 8'b0111_1111;
       default :
          en = 8'b1111_1110;  
    endcase
end

//set data for corresponding ledtube
always@(en or d1 or d2 or d3 or d4 or d5 or d6 or d7 or d8) 
begin
   case(en)
      8'b1111_1110:  
         dataout_buf = d1;
      8'b1111_1101:
         dataout_buf = d2;
      8'b1111_1011:
         dataout_buf = d3;
      8'b1111_0111:
         dataout_buf = d4;
      8'b1110_1111:
         dataout_buf = d5;
      8'b1101_1111:
         dataout_buf = d6;
      8'b1011_1111:
         dataout_buf = d7;
      8'b0111_1111:
         dataout_buf = d8;
      default: 
         dataout_buf = 4'b1111;
    endcase
end

//pixel data
always@(dataout_buf)
begin
   case(dataout_buf)
      4'b0000:
         dataout = 8'b1100_0000; //0
      4'b0001:
         dataout = 8'b1111_1001; //1
      4'b0010:
         dataout = 8'b1010_0100; //2
      4'b0011:
         dataout = 8'b1011_0000; //3
      4'b0100:
         dataout = 8'b1001_1001; //4
      4'b0101:
         dataout = 8'b1001_0010; //5
      4'b0110:
         dataout = 8'b1000_0010; //6
      4'b0111:
         dataout = 8'b1111_1000; //7
      4'b1000:
         dataout = 8'b1000_0000; //8
      4'b1001:
         dataout = 8'b1001_1000; //9
      default: 
         dataout = 8'b1100_0000; //-
    endcase
end

endmodule 