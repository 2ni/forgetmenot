/*
 * https://github.com/nayem-cosmic/RFM69-Library-AVR
 */
#include <avr/io.h>

#ifndef RFM69_h
#define RFM69_h

#define RF69_MODE_SLEEP         0   // XTAL OFF
#define RF69_MODE_STANDBY       1   // XTAL ON
#define RF69_MODE_SYNTH         2   // PLL ON
#define RF69_MODE_RX            3   // RX MODE
#define RF69_MODE_TX            4   // TX MODE

#define COURSE_TEMP_COEF        -90
#define CSMA_LIMIT              -90 // upper RX signal sensitivity threshold in dBm for carrier sense access
#define RF69_BROADCAST_ADDR     0
#define RF69_MAX_DATA_LEN       61
#define RFM69_CTL_SENDACK       0x80
#define RFM69_CTL_REQACK        0x40

uint8_t  rfm69_init(uint16_t freq, uint8_t node_id, uint8_t network_id);
void     select();
void     unselect();
uint8_t  read_reg(uint8_t addr);
void     write_reg(uint8_t addr, uint8_t value);
void     encrypt(const char* key);
void     set_mode(uint8_t new_mode);
void     set_high_power_regs(uint8_t on_off);
void     set_high_power();
void     set_power_level(uint8_t level);
void     sleep();
uint8_t  read_temperature();
int16_t  read_rssi(uint8_t force_trigger);
uint8_t  can_send();
uint8_t  receive_done();
void     receive_begin();
void     send(uint8_t to, const void* buffer, uint8_t size, uint8_t request_ack);
void     send_frame(uint8_t to, const void* buffer, uint8_t size, uint8_t request_ack, uint8_t send_ack);
void     promiscuous(uint8_t on_off);
void     set_network(uint8_t id);
void     send_ack(const void *buffer, uint8_t size);
uint8_t  ack_requested();
void     get_data(char *data, uint8_t len);

#endif
