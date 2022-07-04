set_global_assignment -entity "clkMux" -library "lib_clkMux" -name IP_TOOL_NAME "Qsys"
set_global_assignment -entity "clkMux" -library "lib_clkMux" -name IP_TOOL_VERSION "21.1"
set_global_assignment -entity "clkMux" -library "lib_clkMux" -name IP_TOOL_ENV "Qsys"
set_global_assignment -library "lib_clkMux" -name SPD_FILE [file join $::quartus(sip_path) "../clkMux.spd"]
set_global_assignment -library "lib_clkMux" -name MISC_FILE [file join $::quartus(sip_path) "../../clkMux.qsys"]

set_global_assignment -library "lib_clkMux" -name MISC_FILE [file join $::quartus(sip_path) "clkMux.v"]
set_global_assignment -library "lib_clkMux" -name MISC_FILE [file join $::quartus(sip_path) "submodules/clkMux_altclkctrl_0.v"]
