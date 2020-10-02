/*
 * calculate airtime of lorawan packets
 * based on
 * https://docs.google.com/spreadsheets/d/19SEkw9gAO1NuBZgpJsPshfnaFuJ7NUc4utmLDjeL3eo/edit#gid=585312605
 *
 * the airtime from a packet sent by the gw needs to be added when waiting for an answer on the node
 * to get a proper rx window
 *
 * gcc airtime.c -o bin/airtime && bin/airtime
 *
 */
#include <stdio.h>
#include <stdint.h>


uint16_t calculate_airtime(uint8_t len);

int main() {
  uint8_t len;
  while (1) {
    printf("length of total package:");
    scanf("%hhu", &len); // hhu = uint8_t, hu=uint16_t
    // uint8_t len = 33; // header: 13, payload join accept: 20
    calculate_airtime(len);
  }
}

/*
 * (unsigned) char:   8 bytes (uint8_t)
 * (unsigned) short: 16 bytes (uint16_t)
 * (unsigned) int:   32 bytes (uint32_t)
 */
uint16_t calculate_airtime(uint8_t len) {
  uint8_t preamble = 8;
  uint16_t bandwith = 125;
  uint8_t explicit_header = 1;
  uint8_t coding_rate = 5; // 5=4/5 (lorawan), 6=4/6, 7=4/7, 8=4/8
  uint8_t de = 0; // low data rate optimization

  for (uint8_t datarate=7; datarate<=12; datarate++) {
    de = datarate > 10 ? 1:0;
    uint32_t tsym = (128<<datarate)/bandwith;                                                // *128 for better precision
    uint32_t tpreamble = (preamble*128+4.25*128)*tsym/128;                                   // *128 for better precision
    int32_t temp = (16*8*len-4*datarate+28-16-20*(1-explicit_header)) / (4*(datarate-2*de)); // *16 for better precision
    if (temp<0) temp = 0;
    uint8_t symbnb = 8+(temp+16)/16*coding_rate;                                             // +16/16 for ceiling()

    uint32_t tpayload = symbnb*tsym;
    uint16_t tpacket = (tpreamble+tpayload)/128;                                             // /128 for better precision

    printf("SF%u: %ums\n", datarate, tpacket);
  }

  printf("done.\n");
  return 0;
}
