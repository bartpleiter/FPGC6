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

localparam HSTART_HDMI = 159; // Pixel to start rendering
localparam VSTART_HDMI = 44; // Line to start rendering

localparam HSTART_NTSC = 195; // Pixel to start rendering
localparam VSTART_NTSC = 19; // Line to start rendering

wire [9:0] h_start = (scale2x) ? HSTART_HDMI : HSTART_NTSC;
wire [9:0] v_start = (scale2x) ? VSTART_HDMI : VSTART_NTSC;

reg [7:0] pixel_data = 8'd0;

wire h_active = (h_count > h_start);
wire v_active = (v_count > v_start);

wire [9:0] line_active = (v_active) ? v_count-(v_start+1'b1) : 10'd0;
wire [9:0] pixel_active = (h_active && v_active) ? h_count-h_start : 10'd0;

wire [16:0] pixel_idx = ( (line_active >> scale2x) *320) + (pixel_active >> 1);

assign vram_addr = pixel_idx;

assign r = (blank) ? 3'd0 : vram_q[7:5];
assign g = (blank) ? 3'd0 : vram_q[4:2];
assign b = (blank) ? 2'd0 : vram_q[1:0];

endmodule