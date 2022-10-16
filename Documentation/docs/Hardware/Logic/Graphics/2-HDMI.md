# HDMI
To output a HDMI signal, the output has to be TMDS encoded. However, the Cyclone IV and V do not support this IO standard. Adding an HDMI encoder IC is expensive, difficult to solder, increases PCB complexity and costs more I/O pins. Luckily, 3.3v LVDS (by selecting 2.5v LVDS with a 3.3v IO bank voltage) can also be used to output something close enough that most devices accept it as a valid HDMI signal. The video signal still has to be TMDS encoded before sending it over LVDS, which can be done using some logic.

## HDMI video timings
The timing of the HDMI video signal is as follows:
``` text
H_RES   = 640    // horizontal resolution (pixels)
V_RES   = 480    // vertical resolution (lines)
H_FP    = 16     // horizontal front porch
H_SYNC  = 96     // horizontal sync
H_BP    = 48     // horizontal back porch
V_FP    = 10     // vertical front porch
V_SYNC  = 2      // vertical sync
V_BP    = 33     // vertical back porch
H_POL   = 0      // horizontal sync polarity (0:neg, 1:pos)
V_POL   = 0      // vertical sync polarity
```

## TMDS encoding logic
Since I do not feel the need to exaclty understand the way TMDS encoding works, other than that it minimizes the number of transitions between 1 and 0, I used existing code for this from [this blog post on msjeemjdo.com](https://mjseemjdo.com/2021/04/02/tutorial-6-hdmi-display-output/)

In addition, I use DDR (dubble data rate) in combination with half of the TMDS clock to reduce the clock speed and increase signal stability. Then the output of this is sent to the LVDS encoder.