    .model large
    .stack 100h
    .286
    jumps
    locals

;EXTRN C precalc:far
;EXTRN C intro:far

IPS		=	50
    
.DATA

.CODE
Start:
PUBLIC _mainx
_mainx PROC FAR
        ;call precalc

        mov	ax,cs
		mov	ds,ax

        ; Set 320x200@256
        mov ah, 0
        mov al, 13h
        int 10h

		mov	ax,Music
		mov	es,ax
		call	_InitPlayer
		call	_SetInt

        ;call intro

	; wait for keypress
		;xor	ah,ah
		;int	16h

	; stop tune
		call	_ResetInt
		call	_EndPlayer
		mov	ax,4c00h
		int	21h
_mainx ENDP

PUBLIC _PreparePlayer
_PreparePlayer PROC FAR
		mov	ax,Music
		mov	es,ax
		call	_InitPlayer
		call	_SetInt
        ret
_PreparePlayer ENDP

PUBLIC _StopPlayer
_StopPlayer PROC FAR
		call	_ResetInt
		call	_EndPlayer
        ret
_StopPlayer ENDP

;²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²
; Enables and starts the player interrupt.
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
PUBLIC _SetInt
_SetInt:		push	ax es

		cli
		xor	ax,ax
		mov	es,ax
		mov	ax,es:[8*4]
		mov	word ptr OldInt,ax
		mov	ax,es:2[8*4]
		mov	word ptr OldInt+2,ax
		mov	word ptr es:[8*4], offset PlayerInt
		mov	es:2[8*4],cs

		mov	ax,IPS
		call	SetTimer

		sti
		pop	es ax
		ret






;²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²
; Disables the player interrupt.
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
PUBLIC _ResetInt
_ResetInt:	push	ax es

		cli
		xor	ax,ax
		mov	es,ax
		mov	ax,word ptr OldInt
		mov	es:[8*4],ax
		mov	ax,word ptr OldInt+2
		mov	es:2[8*4],ax

		call	ResetTimer

		sti
		pop	es ax
		ret






;²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²
; The player interrupt.  Called 50 times a second.
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
PlayerInt:	push	ax

		call	_PlayMusic

	; see if we have passed 18.2/s mark
	@@lx:	mov	ax,TimerSteps		; this no. of steps per int.
		add	TimerCnt,ax
		jnc	@@ly			; don't call timer interrupt
		pop	ax
		jmp	cs:OldInt		; call old interrupt handlers

	; standard exit
	@@ly:	mov	al,20h
		out	20h,al
		pop	ax
		iret






;²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²
; Sets the interrupt timer duty cycle.
; IN:
;	AX	- number of times per second for INT08.
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
SetTimer:	push	ax bx dx

		mov	bx,ax
		mov	ax,13532	; 1193180 mod 65536 (TASM craps out)
		mov	dx,18		; 1193180/65536 (TASM can't calculate this)
		div	bx
		mov	bx,ax

		mov	al,36h
		out	43h,al
		mov	al,bl
		out	40h,al
		mov	al,bh
		out	40h,al

		mov	TimerSteps,bx	; for keeping 18.2 timer correct
		mov	TimerCnt,0	; counter

		pop	dx bx ax
		ret






;²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²
; Resets the interrupt timer back to 18.2/sec duty cycle.
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ResetTimer:	push	ax

		mov	al,36h
		out	43h,al
		xor	al,al
		out	40h,al
		out	40h,al

		pop	ax
		ret




		include		Player.Asm




OldInt		dd	?
TimerCnt	dw	?
TimerSteps	dw	?
		ends

Music		segment para public use16
                ;include Raster.Inc
                ;include Popcorn.Inc
                include ALLOYRUN.Inc
		ends

Stack16		segment para public use16 stack
		dw	100h dup (?)
		ends

END Start
