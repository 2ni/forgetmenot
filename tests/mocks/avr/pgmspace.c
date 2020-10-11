#include <stdint.h>
#include "pgmspace.h"

uint8_t pgm_read_byte(const uint8_t *elm) {
  return *elm;
}
