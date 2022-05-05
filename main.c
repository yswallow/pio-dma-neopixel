#include <stdio.h>
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/sync.h"
#include "asm.pio.h"

#define NEOPIXEL_PIN 0
#define PIO_NEOPIXEL_SM 0
#define PIO_ID pio0

static void init_pio_neopixel(void);
static void init_dma(void);
bool alarm_callback(struct repeating_timer *t);

uint32_t neopixel_colors[9];
uint8_t neopixel_colors_head=0;
int dma_channel;
static uint8_t h=1;

int main()
{
    stdio_init_all();
    
    init_pio_neopixel();
    init_dma();
    
    struct repeating_timer timer;
    add_repeating_timer_ms(10, alarm_callback, NULL, &timer);
    // NeoPixel Power ---------------------
    //gpio_init(11);
    //gpio_set_dir(11, GPIO_OUT);
    //gpio_put(11, 1);
    // ------------------------
    while(true) {
        __wfe();
    }
}

static void pio_put_color_dma(uint32_t red, uint32_t green, uint32_t blue) {
    uint32_t pio_color = green<<24 | red<<16 | blue<<8;
    neopixel_colors[neopixel_colors_head++] = pio_color;
}

static void set_neopixel_colors(void) {
    neopixel_colors_head = 0;
    for(uint8_t i=0;i<3;i++) {
        pio_put_color_dma(h,0,0);
        pio_put_color_dma(0,h,0);
        pio_put_color_dma(0,0,h);
    }
    h+=1;
}

static void init_pio_neopixel(void) {
    // send program
    pio_add_program(PIO_ID, &write_neopixel_program);
    
    // initialize
    write_neopixel_program_init(PIO_ID, PIO_NEOPIXEL_SM, NEOPIXEL_PIN);
    
    set_neopixel_colors();
}

bool alarm_callback(struct repeating_timer *t) {
    set_neopixel_colors();
    dma_channel_set_read_addr(dma_channel, neopixel_colors, true);
    return true;
}

/*
static void on_dma_end(void) {
  //called every txf pushes (every one DMA item)  
}
*/

static void init_dma(void) {
    dma_channel = dma_claim_unused_channel(true);
    
    dma_channel_config c = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, 0); // DREQ_PIO0_TX0
    
    dma_channel_configure(
        dma_channel,          // Channel to be configured
        &c,            // The configuration we just created
        PIO_ID->txf,           // The initial write address
        neopixel_colors,           // The initial read address
        9, // Number of transfers; in this case each is 1 byte.
        false           // Start immediately.
    );
    dma_channel_set_irq0_enabled(dma_channel, true);
    
    //irq_set_exclusive_handler(DMA_IRQ_0, on_dma_end);
    //irq_set_enabled(DMA_IRQ_0, true);
    
    dma_channel_start(dma_channel);
}
