#include <xc.inc>

#define IO_PIN_OUT  LATA,2
#define IO_PIN_IN  PORTA,2
#define IO_PIN_TRIS  TRISA,2

GLOBAL _swu_dat,_swu_tmp,_swu_cnt,_swu_flg, _swu_tx, _swu_rx
SIGNAT _swu_tx,4217
SIGNAT _swu_rx,4217

PSECT	mytext,local,class=CODE,delta=2

_swu_tx:
    nop
    movlb   0
    movwf   BANKMASK (_swu_dat)
    movlb   1
    bcf     IO_PIN_TRIS
    movlb   0
    movlw   0x08
    movwf   BANKMASK (_swu_cnt)
    bcf	    IO_PIN_OUT
    call    _swu_tx_sbt
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
   
_swu_tx1:
    btfss   BANKMASK (_swu_dat),0
    bcf	    IO_PIN_OUT
    btfsc   BANKMASK (_swu_dat),0
    bsf	    IO_PIN_OUT
    rrf     BANKMASK (_swu_dat)
    call    _swu_tx_sbt
    decfsz  BANKMASK (_swu_cnt)
    goto   _swu_tx1
    
    bsf	    IO_PIN_OUT
    call    _swu_tx_sbt   
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    banksel TRISA
    bsf     IO_PIN_TRIS
    banksel PORTA
   RETURN

_swu_tx_sbt:
    movlw   0x51
    movwf  BANKMASK (_swu_tmp)
_swu_tx_lp1:
    decfsz  BANKMASK (_swu_tmp)
    goto    _swu_tx_lp1
    return
    
    
    
    
 _swu_rx:
    nop
    nop
    banksel TRISA
    bsf     IO_PIN_TRIS
    banksel PORTA
    nop
    nop
    nop
    nop
    banksel PORTA
    clrf    BANKMASK (_swu_dat)
    movlw   0x08
    movwf   BANKMASK (_swu_cnt)
 _swu_rx1:
    btfsc   IO_PIN_IN
    goto    _swu_rx1
    ;comf    LATB
    nop
    call    _swu_rx_hbt
    nop
    nop
    nop

_swu_rx2:
    call    _swu_rx_hbt
    call    _swu_rx_hbt
    ;comf    LATB
    nop
    nop
    nop
    nop
    rrf     BANKMASK (_swu_dat)
    btfsc   IO_PIN_IN
    bsf     BANKMASK (_swu_dat),7
    btfss   IO_PIN_IN
    nop
    decfsz  BANKMASK (_swu_cnt)
    goto    _swu_rx2
    
    bcf     _swu_flg,0
    btfss   IO_PIN_IN
    bsf     _swu_flg,0
    call    _swu_rx_hbt
    call    _swu_rx_hbt
    movf    BANKMASK (_swu_dat),W
    return
    
_swu_rx_hbt:
    movlw   0x27
    movwf  BANKMASK (_swu_tmp)
_swu_rx_lp2:
    decfsz  BANKMASK (_swu_tmp)
    goto    _swu_rx_lp2
    return
