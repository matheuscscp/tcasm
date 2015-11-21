; File: The3n+1Problem.s
; Author: Lucas Nunes

; OBSERVACAO: modificado p/ retirar a divisao por constante inicializada com zero, pois isso eh ilegal.

; Take any natural number n.
; If n is even, divide it by 2 to get n / 2.
; If n is odd, multiply it by 3 and add 1 to obtain 3n + 1.
; Repeat the process indefinitely. No matter what number you start with, you shall always eventually reach 1.
; Example: 22 -> 11 34 17 52 26 13 40 20 10 5 16 8 4 2 1
; The program displays every number it passes through following the cycle.
; When it reaches 1, it outputs the original input and the number of cycles passed through.
; Then it crashes on a division by zero :D yay
; If the input is negative, the program will just print the input twice, zero, and crash.

Section Data
    one:   Const  1  ; Constants everywhere
    two:   Const  2
    three: Const  3
    num:   Space     ; Number the user entered
    cNum:  Space  2  ; The current number in the cycle, with enough space for a copy
    cycs:  Space     ; Counts the cycles

Section Text
; Entry Point
Main:  Copy   one, cycs         ; Initialize the cycle counter as one (input counts as a cycle)
       Input  num               ; Get the input
       Copy   num, cNum[0]      ; Make a copy
       Output num               ; Show it
       
       ; Check if num is negative
       Load   num               
       JmpP   Outer
       Jmp    End               ; If so, abort
       
; Outer Loop: Calculates cycles
Outer: Copy   cNum[0], cNum[1]  ; Make a copy-copy

    ; Inner loop: Calculates divisibility by 2
    Inner: Load   cNum[1]
           Sub    two
           Store  cNum[1]
           Sub    one
           JmpN   Even          ; N - 1 = -1 => remainder zero
           JmpZ   Odd           ; N - 1 = 0  => remainder one
           Jmp    Inner         ; N - 1 > 0  => still >= 2
           
    ; It's even! Divide by 2
    Even:  Load   cNum[0]
           Div    two
           Jmp    Show
    
    ; It's odd! Multiply by three, add one
    Odd:   Load   cNum[0]
           Mult   three
           Add    one
           Jmp    Show
           
    ; Show the current number, increment the counter, maybe break the loop
    Show:  Store  cNum[0]
           Output cNum[0]
           
           Load   cycs
           Add    one
           Store  cycs
           
           Load   cNum[0]
           Sub    one
           JmpZ   End
           Jmp    Outer
       
End:   Output num
       Output cycs
       Stop
       
       
