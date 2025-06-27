#ifndef LIBMESSAGING_H_
#define LIBMESSAGING_H_

#include <stddef.h>
#include <stdint.h>

void send_data(const char *restrict contents, const char filename[restrict static 1]);

void receive_data(char *restrict contents, const char filename[restrict static 1]);

#endif // LIBMESSAGING_H_
