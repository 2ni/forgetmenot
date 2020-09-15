#ifndef __RFM95_h__
#define __RFM95_h__

uint8_t rfm95_init();
void    rfm95_select();
void    rfm95_unselect();
uint8_t rfm95_read_reg(uint8_t addr);
void    rfm95_write_reg(uint8_t addr, uint8_t data);
void    rfm95_send_package(uint8_t *package, uint8_t package_length);
void    rfm95_set_mode(uint8_t mode);
void    rfm95_setpower(int8_t power);
void    rfm95_set_datarate(uint8_t rate);
void    rfm95_set_channel(uint8_t channel);

#endif
