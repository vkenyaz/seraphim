;NASM X86 bootloader
BITS 16
org 0x7c00
mov ax, 0
mov es, ax
mov ah, 0x02   ; read Sectors From Drive
mov al, 8      ; sector count
mov ch, 0      ; cylinder
mov cl, 2      ; sector
mov dh, 0      ; head
mov bx, 0x500  ; destination address
int 0x13
jmp 0:0x500
times 510 -( $ - $$ ) db 0
db 0x55, 0xaa
