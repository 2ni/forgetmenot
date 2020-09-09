#ifndef __RFM95_h__
#define __RFM95_h__

uint8_t rfm95_init();
void    rfm95_select();
void    rfm95_unselect();
uint8_t rfm95_read_reg(uint8_t addr);
void    rfm95_write_reg(uint8_t addr, uint8_t data);

void    lora_send_data(uint8_t *data, uint8_t data_length, uint16_t frame_counter_tx);
void    rfm95_send_package(uint8_t *package, uint8_t package_length);

void    aes_encrypt_payload(uint8_t *data, uint8_t data_length, uint16_t frame_counter, uint8_t direction);
void    aes_calculate_mic(uint8_t *data, uint8_t *final_mic, uint8_t data_length, uint16_t frame_counter, uint8_t direction);
void    aes_encrypt(uint8_t *data, uint8_t *key);
void    aes_generate_keys(uint8_t *k1, uint8_t *k2);
void    aes_calculate_round_key(uint8_t round, uint8_t *round_key);
void    aes_add_round_key(uint8_t *round_key, uint8_t (*state)[4]);
void    aes_shift_rows(uint8_t (*state)[4]);
void    aes_mix_columns(uint8_t (*state)[4]);
void    aes_shift_left(uint8_t *data);
void    aes_xor(uint8_t *new_data, uint8_t *old_data);
uint8_t aes_sub_byte(uint8_t byte);

#endif
