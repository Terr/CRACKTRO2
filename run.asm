; vim: ft=masm

.model small
.386

	cr	equ 0Dh
	lf	equ 0Ah

print macro msg
	mov ah, 9
	lea dx, msg
	int 21h
endm

_TEXT segment word public 'CODE'

	org 100h
	assume cs:_TEXT,ds:_TEXT,es:_TEXT,ss:_TEXT

main proc near
	mov ax, 4a00h	; Release excess memory
	mov bx, (program_len + 10Fh) / 16
	int 21h

	mov [dsseg], ds
	mov stackseg, ss		; Store stack addresses
	mov stackoff, sp

	; Execute subprogram 1
	mov ax, ds
	mov es, ax
	mov ax, 4B00h
	mov dx, offset intro_exe
	mov bx, offset params
	int 21h

	mov ax, cs
	mov ds, ax
	mov es, ax

	cli		; Prevents a 8088 bug according to
	mov ss, stackseg	; Advanced MS-DOS Programming book
	mov sp, stackoff
	sti

	; Execute subprogram 2
	mov ax, [dsseg]
	mov ds, ax
	mov es, ax
	mov ax, 4B00h
	; Set the segment addresses of the parameter block
	; It might be possible to have the assembler do this but I haven't
	; found a method that works for .com files (because of
	; "segment-relocatable items present" errors)
	mov [params+2], offset cmdline
	mov [params+4], ds
	mov [params+6], offset fcb1
	mov [params+8], ds
	mov [params+10], offset fcb2
	mov [params+12], ds
	mov dx, offset train_com
	mov bx, offset params
	int 21h

terminate:
	; Terminate the program
	mov ax, 4c00h
	int 21h
main endp

intro_exe db 'INTRO.EXE',0
train_com db 'CIVTRAIN.COM',0

; params db 30 dup(0)

; params 	dw 4 dup(0)
; 	dd cmdline
; 	dd fcb1
; 	dd fcb2
; 
; cmdline db	4,' Hi!',cr	; Length (cr not counted) plus non-empty argument
; 				; so trainer will skip the helptext
; 
; fcb1 	db	0		; File Control Block #1
; 	db	11 dup (?)
; 	db	25 dup (0)
; 
; fcb2 	db	0		; File Control Block #2
; 	db	11 dup (?)
; 	db	25 dup (0)


params	dw 0		; Environment address: 0 = copy from parent
	dw cmdline
	dw 0
	dw fcb1
	dw 0
	dw fcb2
	dw 0

cmdline db 10,' Hi there!',cr

fcb1 	db 0			; FCB #1
	db 11 dup (?)
	db 25 dup (?)

fcb2 	db 0			; FCB #2
	db 11 dup (?)
	db 25 dup (?)

stackseg	dw 0
stackoff	dw 0
dsseg		dw 0

; environment 	segment para 'ENVIR'
; 	db	'PATH=',0		; Empty search path
; 	db	'COMSPEC=C:\COMMAND.COM',0	; Location of command.com, simply
; 								; assumed to be in the default
; 								; place on a PC with a hard drive
; 	db	0						; End of environment
; environment 	ends

program_len equ $ - main	; program length

_TEXT ends

end main
