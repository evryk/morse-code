## Contents

1. **Setting Up the Timer**: The `set_alarm` function configures a timer to trigger an interrupt after a specified delay. It calculates the time at which the timer should expire and sets the corresponding register value to trigger the interrupt.

2. **Installing Timer Interrupt Service Routine (ISR)**: The `install_alrm_isr` function installs the ISR for the timer interrupt. It sets up the address of the ISR in the interrupt vector table and enables the timer interrupt in the NVIC (Nested Vectored Interrupt Controller).

3. **Handling Timer Interrupts**: When the timer expires, the processor executes the ISR associated with the timer interrupt (`alrm_isr`). The ISR performs tasks such as processing user input, measuring time intervals, or updating the system state based on the timer expiration.

4. **Using Timers in Application Logic**: Throughout the assembly code, timer-related functions can be called to set up timers, handle timer interrupts, and perform actions based on timer events. For example, if there is a function for `level_in`, the `set_alarm` function is called to set up a timer for detecting these button presses. This ensures that the program waits for a certain duration before checking for user input again.

5. ** example code:@ Get level input : something like this,
      level_in:
    push    {lr}                           @ Store the link register to the stack as we will call the init_btns subroutine
    movs    r4, #3                         @ r4 shows whether a dot (0) or dash (1) has been pressed
    movs    r5, r0                         @ Store the start time in r7
     loop:
    bl      set_alarm
    wfi                                    @ wait for alarm/gpio interrupt
    bl      user_in                        @ insert the user input into the input array

