#ifndef LIBMESSAGING_H_
#define LIBMESSAGING_H_

#include <stddef.h>
#include <stdint.h>

// mmap() allocates a whole page at a time, I recommend setting this to a
// multiple of one of your system's supported page sizes.

// C23
#if (__STDC_VERSION__ >= 202311L)
static constexpr uint_least16_t MESSAGE_SIZE = 4096;
#endif

#if (__STDC_VERSION__ < 202311L)
#define MESSAGE_SIZE 4096
#endif

// Can be called from any process to write at filename.
// Once sent, the process will block until the file is read.
// Once read, the data is deleted.
// Behaviour if the file is already filled is undefined, or rather it is defined as 'will not work'.
// Use sockets or something if you actually want secure communication
void send_data(const char *contents, const char filename[restrict static 1]);

// Thread-blocking, will await for data to be written somewhere before reading
// it.
void receive_data(char contents[restrict static MESSAGE_SIZE], const char filename[restrict static 1]);

#endif // LIBMESSAGING_H_
