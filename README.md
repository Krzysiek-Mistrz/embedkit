# embedkit

C lib 4 embedded systems practice, tested using GoogleTest
and subsequently used—without modifications—on an nRF52840 (via Zephyr / nRF Connect SDK).
Can be used also on other embedded systems like esp32 or stm32 platforms...

```
lib/    bits, ringbuf, fsm, debounce, frame_parser   <- #0 hardware deps
tests/  GoogleTest-based tests
```

## Build and testing on PC

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

U can also use sanitizers 2 detect undefined behavior, memory leaks, and race conditions...

```bash
cmake -S . -B build-asan -G Ninja \
-DCMAKE_C_FLAGS="-fsanitize=address,undefined -g" \
-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g" \
-DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
# use -fsanitize=thread for testing the SPSC ring buffer
```

## Sample usage on nRF52840

Principle: The `lib/` directory contains no includes 4m Zephyr or any HAL. Hardware-specific components -> UART, GPIO, timers reside within the application layer and merely *invoke* the library. Structure:

```
workspace/
├── embedded-practice/lib/...
└── my_app/ 
├── CMakeLists.txt 
├── prj.conf 
└── src/main.c
```

`my_app/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_app)

set(LIB ${CMAKE_CURRENT_SOURCE_DIR}/../embedded-practice/lib)
target_sources(app PRIVATE 
src/main.c 
${LIB}/bits.c ${LIB}/ringbuf.c ${LIB}/fsm.c ${LIB}/debounce.c ${LIB}/frame_parser.c
)
target_include_directories(app PRIVATE ${LIB})
```

`prj.conf` (to start): `CONFIG_GPIO=y`, `CONFIG_SERIAL=y`, `CONFIG_UART_INTERRUPT_DRIVEN=y`, `CONFIG_LOG=y`.

Build and flash:

```bash
west build -b nrf52840dk/nrf52840 my_app
west flash
```

### Example: UART RX (ISR) -> ringbuf -> parser - main loop

```c
static uint8_t rx_storage[256];
static ringbuf_t rx_rb;
static uint8_t frame_buf[64];
static frame_parser_t parser;

static void uart_isr(const struct device *dev, void *user)
{ 
  uint8_tb; 
  while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) { 
    if (uart_fifo_read(dev, &b, 1) == 1) { 
      ringbuf_push(&rx_rb, b);
    }
  }
}

int main(void)
{
  ringbuf_init(&rx_rb, rx_storage, sizeof rx_storage); 
  frame_parser_init(&parser, frame_buf, sizeof frame_buf, on_frame, NULL); 
  for (;;) {
    uint8_t b; 
    while (ringbuf_pop(&rx_rb, &b)) {
      frame_parser_feed(&parser, b); 
    }
    k_msleep(1); 
  }
}
```

## Notes

- **SPSC Ringbuf**: Single-Producer, Single-Consumer. Works in an ISR -> main context on Cortex-M
(due to atomic 32-bit load/store operations). Having two producers (e.g., two threads pushing data)
would require a lock (`k_mutex` / `irq_lock`). Zephyr provides its own `ring_buf` implementation—it is
worth comparing how it addresses these same challenges.
- **Debounce**: Call `debounce_update()` at a fixed interval (e.g., every 5 ms using a `k_timer`).
- **Frame parser**: The CRC is calculated over the `len` and `payload` fields. There is no byte-stuffing; 
if an error occurs, the parser reverts to searching for the `0x7E` start byte.
- The compiler used for nRF is `arm-none-eabi-gcc` from the Zephyr SDK; the library utilizes
`__atomic_*` built-ins (GCC/Clang). If using a different compiler, replace these with `<stdatomic.h>`.

> Sidenote:
> This prj is hella old, I decided 2 post it now cause better then never type shit...  Like I always want the things I create 2 be perfect which in reality is never and can never be a case unortunately

Built with ❤️ using C/Cpp
Modified MIT License © 2023