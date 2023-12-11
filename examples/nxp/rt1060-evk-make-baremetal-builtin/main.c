// Copyright (c) 2022-2023 Cesanta Software Limited
// All rights reserved

#include "hal.h"
#include "mongoose.h"
#include "net.h"

#define BLINK_PERIOD_MS 1000  // LED blinking period in millis

static volatile uint64_t s_ticks;  // Milliseconds since boot
void SysTick_Handler(void) {       // SyStick IRQ handler, triggered every 1ms
  s_ticks++;
}

uint64_t mg_millis(void) {  // Let Mongoose use our uptime function
  return s_ticks;           // Return number of milliseconds since boot
}

void mg_random(void *buf, size_t len) {  // Use on-board RNG
  for (size_t n = 0; n < len; n += sizeof(uint32_t)) {
    uint32_t r = rng_read();
    memcpy((char *) buf + n, &r, n + sizeof(r) > len ? len - n : sizeof(r));
  }
}
static void timer_fn(void *arg) {
  gpio_toggle(LED);                                      // Blink LED
  struct mg_tcpip_if *ifp = arg;                         // And show
  const char *names[] = {"down", "up", "req", "ready"};  // network stats
  MG_INFO(("Ethernet: %s, IP: %M, rx:%u, tx:%u, dr:%u, er:%u",
           names[ifp->state], mg_print_ip4, &ifp->ip, ifp->nrecv, ifp->nsent,
           ifp->ndrop, ifp->nerr));
}

#define RAM_RACETRACK ((uint8_t *) 0x400)
#define RAM_TEXT ((char *) 0x1A28)

const uint8_t RAM_racecar[] = {
    0x0a, 0x49, 0x10, 0xb5, 0x14, 0x23, 0x0a, 0x48, 0x10, 0xf8, 0x01, 0x2b,
    0xca, 0x61, 0x4a, 0x69, 0x12, 0x02, 0x05, 0xd5, 0x01, 0x3b, 0xf7, 0xd2,
    0x06, 0x4b, 0x01, 0x3b, 0xfd, 0xd1, 0xf1, 0xe7, 0x01, 0x22, 0x14, 0x46,
    0x01, 0x3a, 0x00, 0x2c, 0xfb, 0xd1, 0xf0, 0xe7, 0x00, 0xc0, 0x18, 0x40,
    0x28, 0x1a, 0x00, 0x00, 0x00, 0x46, 0xc3, 0x23};
const char RAM_text[] =
    "Hello, Cesanta devs\r\n";  // Actually any null-terminated 21 char
                                // string...

void runitinRAM(void) {
  memcpy((void *) RAM_RACETRACK, RAM_racecar, sizeof(RAM_racecar));
  strcpy((void *) RAM_TEXT, RAM_text);
  asm("cpsid i");  // SysTick is enabled, ENET will also be, customer code
                   // likely will.
  goto *((void *) RAM_RACETRACK);
}

int main(void) {
  //  gpio_output(LED);               // Setup blue LED
  uart_init(UART_DEBUG, 115200);  // Initialise debug printf
  ethernet_init();                // Initialise ethernet pins
  MG_INFO(("Starting, CPU freq %g MHz", (double) SystemCoreClock / 1000000));
  struct mg_mgr mgr;        // Initialise
  mg_mgr_init(&mgr);        // Mongoose event manager
  mg_log_set(MG_LL_DEBUG);  // Set log level

  // Initialise Mongoose network stack
  struct mg_tcpip_driver_imxrt_data driver_data = {.mdc_cr = 24, .phy_addr = 2};
  struct mg_tcpip_if mif = {.mac = GENERATE_LOCALLY_ADMINISTERED_MAC(),
                            // Uncomment below for static configuration:
                            // .ip = mg_htonl(MG_U32(192, 168, 0, 223)),
                            // .mask = mg_htonl(MG_U32(255, 255, 255, 0)),
                            // .gw = mg_htonl(MG_U32(192, 168, 0, 1)),
                            .driver = &mg_tcpip_driver_imxrt,
                            .driver_data = &driver_data};
  mg_tcpip_init(&mgr, &mif);
  mg_timer_add(&mgr, BLINK_PERIOD_MS, MG_TIMER_REPEAT, timer_fn, &mif);

  MG_INFO(("MAC: %M. Waiting for IP...", mg_print_mac, mif.mac));
  while (mif.state != MG_TCPIP_STATE_READY) {
    mg_mgr_poll(&mgr, 0);
  }

  MG_INFO(("Initialising application..."));
  runitinRAM();
#if 0
  web_init(&mgr);

  MG_INFO(("Starting event loop"));
  for (;;) {
    mg_mgr_poll(&mgr, 0);
  }
#endif
  return 0;
}
