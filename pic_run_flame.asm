; К порту D подключено 8 светодиодов, а к порту B - кнопка
; При нажатой кнопке по линейке светодиодов циклически бежит один огонёк, при нажатии второй раз - два рядом идущих огонька

#INCLUDE "P16F877A.INC"  ;Подключаем файл с символьными обозначениями специальных регистров и их битов
LIST p=PIC16F877A
__CONFIG 0x3FFA
STATUS 	equ	03h
PORTB	equ	06h
TRISB	equ 06h
PORTD	equ	08h
TRISD	equ	08h	
RPO		equ 05h
Reg_1   equ 20h
Reg_2   equ 21h
Reg_3   equ 22h
Reg_O_K	equ 23h
Reg_T_K	equ 24h

org	0

START:	bsf STATUS, RPO ;Делаем активной 1 страницу, чтобы получить доступ к TRISB
	clrf TRISB      ;обнуляем TRISB
	clrf TRISD      ;обнуляем TRISD
	bcf STATUS, RPO ;делаем активной 0 страницу
	clrf PORTD      ;обнуляем PORTD
	movlw .255      ;помещаем в аккумулятор число 255
	movwf PORTB     ;заносим число в регистр
	clrf Reg_O_K
	incf Reg_O_K, 1
	movlw .3
	movwf Reg_T_K

S:
	btfsc PORTB, 7
	goto $+4
	movfw Reg_O_K
	call fire	;вызываем функцию огонька
	movwf Reg_O_K

	btfsc PORTB, 6 ;проверяем, нажата ли кнопка
	goto S
	movfw Reg_T_K
	call fire
	movwf Reg_T_K

	goto S

fire:	
	movwf PORTD
	rlf PORTD, 1
	btfss STATUS, 0
	goto $+3
	bcf STATUS, 0
	incf PORTD, 1
	call DELAY
	movfw PORTD
	clrf PORTD
	return
		

DELAY:	movlw .11 ;подпрограмма задержки
        movwf Reg_1

        movlw .160
       	movwf Reg_2

        movlw .200
        movwf Reg_3
		
LOOP    decfsz Reg_1, F
       	goto LOOP

        decfsz Reg_2, F
        goto LOOP

        decfsz Reg_3, F
       	goto LOOP
            
        return
end
