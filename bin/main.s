; Pushes 1..30 onto the stack
        load b8 30
loop:   addi a8 a8 1
        push a8
        bgt b8 a8 loop      ; keep going while b8 > a8
        halt
