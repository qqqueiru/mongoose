#include "hal.h"

// Unless main flash is going to stay untouched:
// - Can't call any system function, including memcpy 
// - Call with interrupts disabled
void runmeinRAM(void){
  for(;;){
    uart_write_buf(UART_DEBUG, "Hello, Cesanta devs\r\n", 21);
    for(int i=0; i<600000000; i++) (void) 0;
  }
}

int main(void) {
  asm("cpsid i");   // SysTick is enabled, ENET will also be, customer code likely will.
  uart_init(UART_DEBUG, 115200);  // Initialise debug printf
  runmeinRAM();
  return 0;
}
