; Test program to verify the copy to SDRAM and jump functionality of the ROM bootloader

Main:

    CopyCode:

        addr2reg CodeToCopy r1              ; r1 = (src) address of the code to copy, stored in ROM
        load 0 r2                           ; r2 = (dst) address 0 of SDRAM: 0x00, and loop var
        load 16 r4                          ; r4 = number of words to copy

        CopyStartLoop:
            ; copy ROM to SDRAM      
            read 0 r1 r15
            write 0 r2 r15

            add r1 1 r1             ; incr ROM address 
            add r2 1 r2             ; incr SDRAM address

            beq r2 r4 2             ; copy is done when SDRAM address == number of words to copy at the start
                jump CopyStartLoop  ; copy is not done yet, copy next address

        jump 0 ; jump to sdram

    CodeToCopy:
    .dw 0b00011100000001000000000000010001 ; Set r1 to 1024
    .dw 0b00011100000000000001000001010101 ; Set r5 to 16
    .dw 0b11010000000000000000010100010000 ; Write value in r1 to address in r5 with offset 0
    .dw 0b11100000000000000000010100000010 ; Read from address in r5 to r2
    .dw 0b11111111111111111111111111111111 ; Halt
    .dw 6 7 8 9 10 11 12 13 14 15 16


Int:
    readintid r1
    reti