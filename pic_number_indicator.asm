; Поочередное зажигание цифры то в левом, то в правом индикаторе.
; Одна кнопка (с кликом) прибавляет 1 к активному индикатору, вторая – вычитает.
LIST        P=16F877A
__CONFIG    0x3FFA	    
PORTC       equ        0x07
TRISC       equ        0x07
PORTB       equ        0x06
TRISB       equ        0x06
STATUS      equ        0x03
PCL         equ        0x02
INTCON      equ        0x0B
TMR0	    equ	       0x01
OPTION_REG  equ	       0x01
FSR	    equ	       0x04
INDF	    equ	       0x00	    
ITR_1	    equ        0x30
ITR_2	    equ        0x31	    
TMP	    equ        0x20  
COUNT       equ        0x21
Reg_T	    equ        0x22	 	    
Reg_1       equ        0x23
Reg_2	    equ        0x24
FLAG	    equ        0x25
org	0
call RS_DET
goto MAIN    
org	4
call S_DET
retfie    
MAIN:	    
	    bcf STATUS, 7    
	    bsf STATUS, 5
	    movlw b'00000111'
	    movwf OPTION_REG 
	    clrf TRISC
	    movlw .255
	    movwf TRISB   
	    bcf STATUS, 5   
	    bsf	PORTB, 4
	    bsf	PORTB, 5
	    clrf PORTC   
	    clrf ITR_1
	    clrf ITR_2    
	    movlw b'10101000'
	    movwf INTCON    
	    movlw .78
	    movwf COUNT    
	    clrf FLAG
	    clrf Reg_T    
	    movlw ITR_1
	    movwf FSR   
START: 
	    movfw INDF
            call TABLE_NUM
	    iorwf FLAG, 0
            movwf PORTC
	    goto START
DELAY_7537800N:
            movlw .241
            movwf Reg_1  
            movlw .49
            movwf Reg_2  
            decfsz Reg_1, 1
            goto $-1    
            decfsz Reg_2, 1
            goto $-3	    
            nop
	    
	    return	    
RS_DET:	
	    return
S_DET:	    
	    movwf TMP
	    btfsc INTCON, 0
	    call EVENT_PORTB
	    btfsc INTCON, 2
	    call EVENT_TMR0
	    movfw TMP
	    return
EVENT_PORTB:	
	    btfss PORTB, 4
	    call EVENT_PORTB_4
	    btfss PORTB, 5
	    call EVENT_PORTB_5
	    movfw PORTB
	    bcf	INTCON, 0
	    return    
EVENT_PORTB_4:
	    incf INDF, 1
            movlw .10
	    bcf STATUS, 2
            subwf INDF, 0
	    btfsc STATUS, 2
            clrf INDF    
	    bsf	PORTB, 4
	    return
EVENT_PORTB_5:
	    decf INDF, 1
	    movlw .255
	    bcf	STATUS, 2
            subwf INDF, 0
	    movlw .9
	    btfsc STATUS, 2
	    movwf INDF   
	    bsf	PORTB, 5
	    return
EVENT_TMR0:
	    incf Reg_T, 1
	    bcf	STATUS, 2
	    movfw COUNT
	    subwf Reg_T, 0
	    btfss STATUS, 2
	    goto END_TMR0
	    movlw .128
	    bcf	STATUS, 2
            subwf FLAG, 0
	    btfsc STATUS, 2
	    goto RIGHT_IND   
	    movlw ITR_2
	    movwf FSR
	    movlw .128
	    movwf FLAG
	    goto END_SWAP	    
RIGHT_IND:
	    movlw ITR_1
	    movwf FSR
	    clrf FLAG
END_SWAP:	    
	    clrf Reg_T    
END_TMR0:   
	    bcf	INTCON, 2
	    return
	    
TABLE_NUM: 			;значения чисел на индикаторе
	    addwf  PCL, 1
            retlw  b'01000000';0
            retlw  b'01111001';1
            retlw  b'00100100';2
            retlw  b'00110000';3
            retlw  b'00011001';4
            retlw  b'00010010';5
            retlw  b'00000010';6
            retlw  b'01111000';7
            retlw  b'00000000';8
            retlw  b'00010000';9
	    
	    end
