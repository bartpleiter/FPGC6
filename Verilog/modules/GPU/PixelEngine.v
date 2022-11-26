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
    input [7:0]    vram_q
);

localparam HSTART = 159; // Pixel to start rendering
localparam HEND = 800;   // Pixel to end rendering

localparam VSTART = 44; // Line to start rendering
localparam VEND = 524;  // Line to start rendering

reg [7:0] pixel_data = 8'd0;

wire h_active = (h_count > HSTART && h_count <= HEND);
wire v_active = (v_count > VSTART && v_count <= VEND);


wire [9:0] line_active = (v_active) ? v_count-(VSTART+1'b1) : 10'd0;
wire [9:0] pixel_active = (h_active && v_active) ? h_count-HSTART : 10'd0;

wire [16:0] pixel_idx = ( (line_active >> 1) *320) + (pixel_active >> 1);

assign vram_addr = pixel_idx;

assign r = (blank) ? 3'd0 : vram_q[7:5];
assign g = (blank) ? 3'd0 : vram_q[4:2];
assign b = (blank) ? 2'd0 : vram_q[1:0];

endmodule