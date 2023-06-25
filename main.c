#include <stdio.h>
#include "pico/stdlib.h"

#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "uart_rx.pio.h"

#define WITH_OUTPUT 1

#define FIFO_SIZE 64

#define PIO_RX_PIN 5
#define PIO_SIDESET 7
#define OUT_TRIGGER 26

static PIO pio;
static uint sm;
static uint offset;
// For pushing data back from core1 to core0 for printing
static queue_t fifo;

#define PATTERN_LEN_MAX 1024
static uint baudrate;
static char pattern[PATTERN_LEN_MAX];
static uint pattern_len = PATTERN_LEN_MAX + 1;

static void __time_critical_func(core1_main)() {
    // Wait for a uart char to appear on the FIFO
    io_rw_8 *rxfifo_shift = (io_rw_8*)&pio->rxf[sm] + 3;
    char c;
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    int match_counter = 0;

    while (1) {
        // Wait for a character
        while (pio_sm_is_rx_fifo_empty(pio, sm))
            tight_loop_contents();
        
        c = (char)*rxfifo_shift;

        if (pattern[match_counter] == c) {
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
            ++match_counter;
        } else {
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            match_counter = 0;
        }

        if (pattern[match_counter] == c) {
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
            ++match_counter;
        }
        
        if (match_counter == 4) {
            gpio_put(OUT_TRIGGER, 1);
            asm volatile("nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop");
            gpio_put(OUT_TRIGGER, 0);
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            match_counter = 0;
        }

#ifdef WITH_OUTPUT
        // Push it to core0 for debugging
        // This adds some overhead between characters, but for 115200 UART it's fine.
        if (!queue_try_add(&fifo, &c)) {
            panic("fifo full");
        }
#endif
    }
}

int main() {
    char c;
    int i;

    // set_sys_clock_khz(300000, true);

    stdio_init_all();
    
    getchar();

    queue_init(&fifo, 1, FIFO_SIZE);

    pio = pio0;
    offset = pio_add_program(pio, &uart_rx_program);
    sm = (int8_t)pio_claim_unused_sm(pio, false);
    assert(sm >= 0);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    gpio_init(OUT_TRIGGER);
    gpio_set_dir(OUT_TRIGGER, GPIO_OUT);
    gpio_put(OUT_TRIGGER, 0);

    printf("baudrate?\n>");
    scanf("%d", &baudrate);
    printf(" %d\n", baudrate);
    
    while (pattern_len > 1024) {
        printf("pattern_len?\n>");
        scanf("%d", &pattern_len);
        printf(" %d\n", pattern_len);
    }

    printf("pattern?\n>");
    for (i = 0; i < pattern_len; ++i) {
        pattern[i] = getchar();
    }
    for (i = 0; i < pattern_len; ++i) {
        printf("%02x", pattern[i]);
    }
    printf("\n");

    uart_rx_program_init(pio, sm, offset, PIO_RX_PIN, baudrate);
    multicore_launch_core1(core1_main);

#ifdef WITH_OUTPUT
    while (1) {
        // Wait for characters to come from core1 to print
        while(queue_is_empty(&fifo))
            tight_loop_contents();

        if (!queue_try_remove(&fifo, &c))
            panic("fifo empty");

        putchar(c);
    }
#endif

    // getchar();

    // while (true) {
    //     printf("Hello, world!\n");
    //     sleep_ms(1000);
    // }
}

