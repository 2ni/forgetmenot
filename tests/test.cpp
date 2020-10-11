/*
 * gcc -I mocks -I ../common ../common/pins.cpp ../common/cmac.cpp ../common/aes.cpp test.cpp -o test && ./test
 *
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "struct.h"
#include "pins.h"
#include "cmac.h"
#include "aes.h"

#define NOK(str)  "\033[31;1m" str "\033[0m"  // output in red
#define OK(str)   "\033[32;1m" str "\033[0m"  // output in green
#define WARN(str) "\033[33;1m" str "\033[0m"  // output in yellow
#define BOLD(str) "\033[1m" str "\033[0m"     // output bold

typedef enum {
  FAIL,
  PASS
} Test_Result;

void printarray(uint8_t *array, uint8_t len) {
  for (uint8_t i=0; i<len; i++) {
    printf(" %02x", array[i]);
  }
  printf("\n");
}

Test_Result is_same(Packet *one, Packet *two) {
  if (one->len != two->len) return FAIL;

  for (uint8_t i=0; i<one->len; i++) {
    if (one->data[i] != two->data[i]) return FAIL;
  }

  return PASS;
}

Test_Result assert(uint8_t same) {
  return (same ? PASS : FAIL);
}

void print_test(const char *title, Test_Result result) {
  printf("%s: %s\n", title, result == PASS ? OK("\xE2\x9C\x93") : NOK("\xE2\x9C\x97"));
}

int main() {
  uint16_t number_of_tests = 0;
  uint16_t number_of_failed = 0;
  Test_Result result;

  // test pgmspace mock
  result = assert(pgm_read_byte(&AES_BOX[0][0]) == 0x63);
  number_of_tests++;
  number_of_failed += result;
  print_test("pgmspace", result);

  // test B0 appending
  const uint8_t len = 4;
  uint8_t data[len] = { 0x01, 0x02, 0x03, 0x04 };
  Packet packet = { .data=data, .len=len };
  uint16_t counter = 0x1234;
  uint8_t direction = 1;
  uint8_t dev_addr[4] = { 0x11, 0x12, 0x13, 0x14 };

  const uint8_t blocklen = len + 16;
  uint8_t blockdata[blocklen] = {0};
  Packet block = { .data=blockdata, .len=blocklen };

  aes128_b0(&packet, counter, direction, dev_addr, &block);
  const uint8_t elen = 20;
  uint8_t edata[elen] = { 0x49, 0x00, 0x00, 0x00, 0x00, 0x01, 0x14, 0x13, 0x12, 0x11, 0x34, 0x12, 0x00, 0x00, 0x00, 0x04, 0x01, 0x02, 0x03, 0x04 };
  Packet expected = { .data=edata, .len=elen };
  result = assert(is_same(&expected, &block));
  number_of_tests++;
  number_of_failed += result;
  print_test("prepend B0", result);


  // final
  number_of_failed = number_of_tests - number_of_failed;
  printf("Total tests failed: %u/%u.\n", number_of_failed ? number_of_failed : number_of_failed, number_of_tests);
  return number_of_failed;
}

PORT_t FakePort;
ADC_t FakeADC;
SIGROW_t FakeSIGROW;

/*
 * override pgmspace functions
 */
uint8_t pgm_read_byte(const uint8_t *elm) {
  return *elm;
}

