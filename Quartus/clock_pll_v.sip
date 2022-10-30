set_global_assignment -entity "clock_pll_v" -library "lib_clock_pll_v" -name IP_TOOL_NAME "altera_pll"
set_global_assignment -entity "clock_pll_v" -library "lib_clock_pll_v" -name IP_TOOL_VERSION "21.1"
set_global_assignment -entity "clock_pll_v" -library "lib_clock_pll_v" -name IP_TOOL_ENV "mwpim"
set_global_assignment -library "lib_clock_pll_v" -name SPD_FILE [file join $::quartus(sip_path) "clock_pll_v.spd"]

set_global_assignment -library "lib_clock_pll_v" -name MISC_FILE [file join $::quartus(sip_path) "clock_pll_v_sim/clock_pll_v.vo"]
