#include "hardware/regs/addressmap.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/regs/timer.h"
#include "hardware/regs/m0plus.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/regs/sio.h"
 
.syntax unified 
.cpu cortex-m0plus 
.thumb 
.global main_asm 
.align  4 


.equ    DFLT_ALRM0_TIME, 1000000      @ Specify the default ALARM0 timeout (space)
.equ    DFLT_ALRM1_TIME, 2000000      @ Specify the default ALARM1 timeout (sequence)

.equ    DOT_TIME, 250000              @ Specify threshold for DOT time (0.25s)
 
.equ    GPIO_FALL_MSK, 0x00400000     @ Bit-22 for falling-edge event on GP21
.equ    GPIO_RISE_MSK, 0x00800000     @ Bit-23 for rising-edge event on GP21

.equ    GPIO_BTN_PIN,  21             @ Specify pin for the button interrupt
.equ    GPIO_DIR_IN,   0              @ Specify input direction for a GPIO pin 
.equ    GPIO_DIR_OUT,  1              @ Specify output direction for a GPIO pin 

.equ    GPIO_ISR_OFFSET, 0x74         @ GPIO is int #13 (vector table entry 29) 
.equ    ALRM0_ISR_OFFSET, 0x40        @ ALARM0 is int #0 (vector table entry 16)
.equ    ALRM1_ISR_OFFSET, 0x44        @ ALARM1 is int #1

main_asm:       
    bl      init_pin                    @ Initialise the GPIO21 Button
    bl      install_alrm_isr            @ Branch to subroutine to initialise Alarm ISR (0 and 1)
    bl      install_gpio_isr            @ Branch to subroutine to initialise GPIO ISR
    bl      print_welcome               @ C function
    
@ Enter main loop
main_loop:
    wfi                                 @ Waits here until any interrupt fires
    b       main_loop                   @ Always branches back to the loop


@ Initialising Button Interrupt
init_pin:
    push    {lr}                        @ Save the link register on the stack
    movs    r0, #GPIO_BTN_PIN           @ This value is the GPIO21 BUTTON pin on the PI PICO board
    bl      asm_gpio_init               @ Call the subroutine to initialise the GPIO pin specified by r0
    movs    r0, #GPIO_BTN_PIN           @ This value is the GPIO21 BUTTON pin on the PI PICO board
    movs    r1, #GPIO_DIR_IN            @ We want this GPIO pin to be setup as an input pin
    bl      asm_gpio_set_dir            @ Call the subroutine to set the GPIO pin specified by r0 to state specified by r1
    movs    r0, #GPIO_BTN_PIN           @ This value is the GPIO21 BUTTON pin on the PI PICO board
    bl      asm_gpio_set_irq            @ Call the subroutine to set up falling-edge and rising-edge interrupts
    pop     {pc}                        @ Pop the program counter from the stack to branch back to main_asm

@ Subroutine used to install the Alarm interrupt service handler
install_alrm_isr:
    @ ALARM0 ISR (for a space)
    ldr       r2, =(PPB_BASE + M0PLUS_VTOR_OFFSET)      @ Get the address of the RAM vector table using the (PPB_BASE + M0PLUS_VTOR_OFFSET) register
    ldr       r1, [r2]                                  @ Load its value into r1
    movs      r2, #ALRM0_ISR_OFFSET                     @ Save the offset of the Alarm0 Interrupt to r2
    add       r2, r1                                    @ Get memory location of the Alarm handler entry in vector table
    ldr       r0, =alrm0_isr                            @ Load the address of the alrm0_isr function into r0
    str       r0, [r2]                                  @ Store the address of the alrm0_isr handler to the correct offset for ALARM0 in the vector table
    @ ALARM1 ISR (for a sequence)
    ldr       r2, =(PPB_BASE + M0PLUS_VTOR_OFFSET)      @ Get the address of the RAM vector table using the (PPB_BASE + M0PLUS_VTOR_OFFSET) register
    ldr       r1, [r2]                                  @ Load its value into r1
    movs      r2, #ALRM1_ISR_OFFSET                     @ Save the offset of the Alarm1 Interrupt to r2
    add       r2, r1                                    @ Get memory location of the Alarm handler entry in vector table
    ldr       r0, =alrm1_isr                            @ Load the address of the alrm1_isr function into r0
    str       r0, [r2]                                  @ Store the address of the alrm1_isr handler to the correct offset for ALARM1 in the vector table
    @ Clear and set interrupts
    ldr       r0, =0x3                                  @ Bit 0 for TIMER_IRQ_0, Bit 1 for TIMER_IRQ_1
    ldr       r2, =(PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET) @ Get (PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET) address
    str       r0, [r2]                                  @ Disable the ALARM0 and ALARM1 IRQ using ^^ address
    ldr       r2, =(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET) @ Get (PPB_BASE + M0PLUS_NVIC_ISER_OFFSET) address
    str       r0, [r2]                                  @ Enable the ALARM0 and ALARM1 IRQ using ^^ address
    bx        lr                                        @ Exit subroutine

@ Subroutine used to install the GPIO interrupt service handler
install_gpio_isr:
    ldr       r2, =(PPB_BASE + M0PLUS_VTOR_OFFSET)      @ Get RAM vector table address using (PPB_BASE + M0PLUS_VTOR_OFFSET) register
    ldr       r1, [r2]                                  @ Load its value into r1
    movs      r2, #GPIO_ISR_OFFSET                      @ Save the offset of the GPIO Interrupt to r2
    add       r2, r1                                    @ Get memory location of the GPIO handler entry in vector table
    ldr       r0, =gpio_isr                             @ Load the address of the gpio_isr function into r0
    str       r0, [r2]                                  @ Stores the address of the gpio_isr handler to the correct offset for GPIO in the vector table
    @ Clear and set interrupt
    ldr       r0, =0x2000                               @ Load 2^13 since GPIO is IRQ13 (bit 13 so int #13)
    ldr       r2, =(PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET) @ Get (PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET) address
    str       r0, [r2]                                  @ Disable the GPIO IRQ using ^^ address
    ldr       r2, =(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET) @ Get (PPB_BASE + M0PLUS_NVIC_ISER_OFFSET) address
    str       r0, [r2]                                  @ Enable the GPIO IRQ using ^^ address
    bx        lr                                        @ Exit subroutine

@ Enable alarm timer interrupts and set an alarm
set_alarm:
    push    {lr}                                        @ Save the link register on the stack
    @ ALARM0 (for a space = end of morse character)
    ldr     r2, =(TIMER_BASE + TIMER_TIMELR_OFFSET)     @ Get current timer count address
    ldr     r0, =DFLT_ALRM0_TIME                        @ Load space character delay (1s)
    ldr     r1, [r2]                                    @ Load current time into r1
    add     r1, r0                                      @ Add time delay we want to wait for to current timer count
    
    ldr     r2, =(TIMER_BASE + TIMER_ALARM0_OFFSET)     @ Load Alarm0 offset
    str     r1, [r2]                                    @ Store updated value to offset

    @ ALARM1 (for a sequence = end of full word)
    ldr     r2, =(TIMER_BASE + TIMER_TIMELR_OFFSET)     @ Get current timer count address
    ldr     r0, =DFLT_ALRM1_TIME                        @ Load end of sequence delay (2s)
    ldr     r1, [r2]                                    @ Load current time into r1
    add     r1, r0                                      @ Add time delay we want to wait for to current timer count
    
    ldr     r2, =(TIMER_BASE + TIMER_ALARM1_OFFSET)     @ Load Alarm1 offset
    str     r1, [r2]                                    @ Store updated value to offset

    @ Enable both ALARM0 and ALARM1
    movs    r1, #0x3                                    @ Value used for Alarm0 and Alarm1
    ldr     r2, =(TIMER_BASE + TIMER_INTE_OFFSET)       @ Timer Interrupt enable address
    str     r1, [r2]                                    @ Enable both Alarms
    
    pop     {pc}                                        @ Pop the link register from the stack to the program counter


@ Alarm interrupt service handler routine -> char input done
.thumb_func
alrm0_isr:
    push    {lr}                                        @ Store the link register to the stack as we will call nested subroutines
    ldr     r1, =(TIMER_BASE + TIMER_INTR_OFFSET)       @ Load TIMER raw interrupts address
    movs    r0, #1                                      @ Save value to clear Alarm0 interrupt
    str     r0, [r1]                                    @ Clear pending interrupt
    bl      end_char                                    @ Finished inputting character -> call C function
    pop     {pc}                                        @ Pop the link register from the stack to the program counter

@ Alarm interrupt service handler routine -> word/sequence input done
.thumb_func
alrm1_isr:
    push    {lr}                                        @ Store the link register to the stack as we will call nested subroutines
    ldr     r1, =(TIMER_BASE + TIMER_INTR_OFFSET)       @ Load TIMER raw interrupts address
    movs    r0, #2                                      @ Save value to clear Alarm1 interrupt
    str     r0, [r1]                                    @ Clear pending interrupt
    bl      end_word                                    @ Finished inputting full word -> call C function
    pop     {pc}                                        @ Pop the link register from the stack to the program counter

@ GPIO21 interrupt service handler routine -> input dot or dash
.thumb_func
gpio_isr:
    push    {lr}                                                @ Store the link register to the stack as we will call nested loops
    ldr     r2, =(IO_BANK0_BASE + IO_BANK0_PROC0_INTS2_OFFSET)  @ Get the address of the register which informs whether an interrupt has happened
    ldr     r1, [r2]                                            @ Load the value from that register into r1
    ldr     r0, =GPIO_FALL_MSK                                  @ Mask for falling edge
    cmp     r0, r1                                              @ Checks for falling edge in the registers
    beq     btn_pressed                                         @ If equal, go to btn_pressed subroutine
    ldr     r0, =GPIO_RISE_MSK                                  @ Mask for rising edge
    cmp     r0, r1                                              @ Checks for rising edge in the registers
    beq     btn_released                                        @ If equal, go to btn_released subroutine

gpio_exit:
    pop     {pc}                                                @ Pop the link register from the stack to the program counter       

btn_pressed:
    ldr     r2, =(IO_BANK0_BASE + IO_BANK0_INTR2_OFFSET)        @ Get the address of the INTR2 register
    str     r0, [r2]                                            @ Reset the GPIO ISR for GP21 Falling edge using Mask
    @ Disable both Alarm0 and Alarm1
    ldr		r2, =(TIMER_BASE + TIMER_INTE_OFFSET)				@ Load Timer Interrupt enable address
	movs	r1, #0x0											@ Get 0 to disable
	str		r1, [r2]                                            @ Disable both alarms
    @ Get current time at initial press down
    ldr     r2, =(TIMER_BASE + TIMER_TIMELR_OFFSET)             @ Load TimeLR address
    ldr     r1, [r2]                                            @ Load current time for when the button is first pressed
    ldr     r2, =curr_time                                      @ Load memory address into r2
    str     r1, [r2]                                            @ Store initial press time into memory 
    b       gpio_exit                                           @ Finish handling the interrupt

btn_released:
    ldr     r2, =(IO_BANK0_BASE + IO_BANK0_INTR2_OFFSET)        @ Get the address of the INTR2 register
    str     r0, [r2]                                            @ Reset the GPIO ISR for GP21 Rising edge using Mask
    @ Get current time when button is released
    ldr     r2, =(TIMER_BASE + TIMER_TIMELR_OFFSET)             @ Load TimeLR address
    ldr     r1, [r2]                                            @ Load time when button is no longer pressed
    ldr     r2, =curr_time                                      @ Load curr_time address
    ldr     r0, [r2]                                            @ Load time for initial button press
    subs    r1, r1, r0                                          @ Get press down duration by subtracting: (release - press)
    @ Determine whether dot or dash
    ldr     r0, =DOT_TIME                                       @ Load specified DOT TIME 0.25s
    cmp     r1, r0                                              @ Compare press down duration to DOT TIME
    bgt     dash                                                @ If time pressed is more than 0.25s -> it's a dash
    b       dot                                                 @ Else it's a dot

dot:
    movs    r0, #0                                              @ Value specified for a dot
    bl      add_input                                           @ Call C function to input a dot
    b       released_exit                                       @ Finish handling the interrupt

dash:
    movs    r0, #1                                              @ Value specified for a dash
    bl      add_input                                           @ Call C function to input a dash
    b       released_exit                                       @ Finish handling the interrupt

released_exit:
    bl      set_alarm                                           @ Reset alarm for next trigger
    b       gpio_exit                                           @ Finish


@ Set data alignment
.data
    .align 4
    curr_time:  .word 0x0
