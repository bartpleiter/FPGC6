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
    .dw 0b00011100000000000000010100010001 ;Set r1 to 5                                    //27
    .dw 0b00011100000000000000001000100010 ;Set r2 to 2                                    //28 [Done, -> int]
    .dw 0b00000011000000000000000100100011 ;Compute r1 + r2 and write result to r3         //29 
    .dw 0b00000011000000000000001100100011 ;Compute r3 + r2 and write result to r3         //2A
    .dw 0b11111111111111111111111111111111 ; Halt
    .dw 6 7 8 9 10 11 12 13 14 15 16


Int:
    push r1
    push r2
    push r3
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    readintid r1
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    pop r3
    pop r2
    pop r1
    reti
