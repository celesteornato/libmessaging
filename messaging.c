/*
  This is a C23 library. You may port it to other versions if you want, should
  be fairly easy.
*/

#include "messaging.h"
// Wowowow that's a lot of UNIX stuff!
// Feel free to figure out a way to port this to windows
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// mmap() allocates a whole page at a time, I recommend setting this to a
// multiple of one of your system's supported page sizes.
static constexpr uint16_t MESSAGE_SIZE = 4096;

// As these should never be treated as part of the message since the first
// character is skipped, these can be whatever as long as they are not the
// same.
static constexpr char MAGIC_READ_TOKEN = '?';
static constexpr char MAGIC_WRITE_TOKEN = '!';

// Helper function, exits if something went wrong.
static inline void ferror_handle(int result, int expected, const char *msg,
                                 int errcode) {
  if (result == expected) {
    return;
  }
  puts(msg);
  exit(errcode);
}

// Used to pass around file data easier to avoid repetition.
struct file_info {
  int fd;
  size_t len;
  char *file;
  const char *name;
};

// Can't wait for defer to be a thing in a year or two.
static void close_shared(struct file_info inf) {
  munmap(inf.file, inf.len);
  close(inf.fd);
}

// More error handling than actual function.
// Message size is determined in the header.
static void create_file(const char filename[static 1]) {
  FILE *fp = fopen(filename, "w");
  ferror_handle(fseek(fp, MESSAGE_SIZE, SEEK_SET), 0,
                "Error seeking file: ", errno);
  ferror_handle(fputc('\0', fp), 0, "Error running fputc", -8);
  ferror_handle(fclose(fp), 0, "Error closing file: ", errno);
}

// Maps a file into memory, it will get tagged for deletion later on.
static struct file_info get_file(const char filename[static 1]) {
  if (access(filename, F_OK) == -1) {
    if (errno != ENOENT) {
      perror("Fatal file-sharing exception: ");
      exit(errno);
    }
    create_file(filename);
  }

  int fd = open(filename, O_RDWR);
  struct stat statbuff;
  fstat(fd, &statbuff);

  size_t file_size = (size_t)statbuff.st_size;

  return (struct file_info){
      .fd = fd,
      .file =
          mmap(nullptr, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0),
      .name = filename,
      .len = file_size};
}

// Can be called from any process to write at filename.
// Once sent, the process will block until the file is read.
// Once read, the data is deleted.
void send_data(const char *restrict contents,
               const char filename[restrict static 1]) {
  printf("Sending %s...\n", filename);
  struct file_info info = get_file(filename);

  strcpy(info.file + sizeof(char), contents);
  info.file[0] = MAGIC_WRITE_TOKEN;
  msync(info.file, info.len, MS_SYNC);

  puts("Waiting for consumption");

  while (info.file[0] != MAGIC_READ_TOKEN) {
    msync(info.file, info.len, MS_SYNC);
  }

  puts("File consumed");

  msync(info.file, info.len, MS_SYNC);
  close_shared(info);
  unlink(info.name);
}

// Thread-blocking, will await for data to be written somewhere before reading
// it.
void receive_data(char *restrict contents,
                  const char filename[restrict static 1]) {
  printf("Receiving %s\n", filename);
  struct file_info info = get_file(filename);

  while (info.file[0] != MAGIC_WRITE_TOKEN) {
    msync(info.file, info.len, MS_SYNC);
  }

  // hack, we are skipping the first (special) character
  strcpy(contents, info.file + sizeof(char));

  info.file[0] = MAGIC_READ_TOKEN;
  msync(info.file, info.len, MS_SYNC);

  printf("%s received\n", filename);
  printf("%s\n", contents);
  close_shared(info);
}
