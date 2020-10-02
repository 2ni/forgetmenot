/*
 * calculate channel registers
 *
 * gcc channel.c -o bin/channel && bin/channel
 *
 * register value = frq*2^19/32MHz
 *
 */
#include <stdio.h>
#include <stdint.h>


uint64_t calculate_channel(uint32_t frq);

int main() {
  uint32_t frq;
  uint64_t r;
  uint32_t channels[9] = { 868100, 868300, 868500, 867100, 867300, 867500, 867700, 867900, 868525 };

  for (uint8_t i=0; i<9; i++) {
    r = calculate_channel(channels[i]);
    printf("channel %u %uMHz: (%llu) 0x%02x 0x%02x 0x%02x\n", i, channels[i], r, (uint8_t)(r>>16), (uint8_t)(r>>8), (uint8_t)(r));
  }

  /*
  while (1) {
    printf("frq in kHz (eg 868100 or 868525):");
    scanf("%u", &frq); // hhu = uint8_t, hu=uint16_t, u=uint32_t
    calculate_channel(frq);
  }
  */
}

/*
 * frq * 2^19 / 32'000'000
 * channel 0 868100 kHz: (14222950) 0xd9 0x06 0x66
 * channel 1 868300 kHz: (14226227) 0xd9 0x13 0x33
 * channel 2 868500 kHz: (14229504) 0xd9 0x20 0x00
 * channel 3 867100 kHz: (14206566) 0xd8 0xc6 0x66
 * channel 4 867300 kHz: (14209843) 0xd8 0xd3 0x33
 * channel 5 867500 kHz: (14213120) 0xd8 0xe0 0x00
 * channel 6 867700 kHz: (14216396) 0xd8 0xec 0xcc
 * channel 7 867900 kHz: (14219673) 0xd8 0xf9 0x99
 * channel 8 868525 kHz: (14229913) 0xd9 0x21 0x99
 *
 * frq: frequency in kHz, eg 868100, 868525
 */
uint64_t calculate_channel(uint32_t frq) {
  uint64_t r = (uint64_t)frq*524288/32000;
  // printf("r: %llu 0x%08llx 0x%02x 0x%02x 0x%02x\n", r, r, (uint8_t)(r>>16), (uint8_t)(r>>8), (uint8_t)(r));
  return r;
}
