/*
* Renders Pixel Plane
*/
module PixelEngine(
    // Video I/O
    input               clk,
    input               hs,
    input               vs,
    input               blank,
    input               scale2x,    // render vertically in 2x scaling (e.g. for HDMI to get full screen 320x240 on a 640x480 signal)
                                    // note: horizontally this scaling is always applied

    // Output pixels
    output wire [2:0]   r,
    output wire [2:0]   g,
    output wire [1:0]   b,

    input [11:0]        h_count,  // line position in pixels including blanking 
    input [11:0]        v_count,  // frame position in lines including blanking 

    // VRAMpixel
    output wire [16:0] vram_addr,
    input  [7:0]       vram_q
);

localparam VSTART = 45; // Line to start rendering
localparam HSTART = 159; // Pixel to start rendering, -1 because pixel_data buffer

reg [7:0] pixel_data = 8'd0;

assign vram_addr = (blank) ? 32'd0: ( ( (v_count - VSTART) >> scale2x ) * 320) + ( (h_count - HSTART)  >> scale2x);

// Reading from VRAM
always @(posedge clk)
begin
    if (blank)
    begin
        pixel_data <= 8'd0;
    end
    else
    begin
        pixel_data <= vram_q;
    end
end

assign r = pixel_data[7:5];
assign g = pixel_data[4:2];
assign b = pixel_data[1:0];

endmodule