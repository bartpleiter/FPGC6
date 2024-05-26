set_global_assignment -entity "mainpll" -library "lib_mainpll" -name IP_TOOL_NAME "altera_pll"
set_global_assignment -entity "mainpll" -library "lib_mainpll" -name IP_TOOL_VERSION "23.1"
set_global_assignment -entity "mainpll" -library "lib_mainpll" -name IP_TOOL_ENV "mwpim"
set_global_assignment -library "lib_mainpll" -name SPD_FILE [file join $::quartus(sip_path) "mainpll.spd"]

set_global_assignment -library "lib_mainpll" -name MISC_FILE [file join $::quartus(sip_path) "mainpll_sim/mainpll.vo"]
