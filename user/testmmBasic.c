#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// Test basic mmap functionality with lazy loading
int main() {
  int fd;
  char *p;
  char test_content[] = "Hello, mmap! This is a test file for memory mapping.\n";
  int len = strlen(test_content);

  // Create a test file
  fd = open("testfile", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("test_mmap_basic: cannot create testfile\n");
    exit(1);
  }

  // Write test content
  if (write(fd, test_content, len) != len) {
    printf("test_mmap_basic: write error\n");
    close(fd);
    exit(1);
  }
  close(fd);

  // Open for reading
  fd = open("testfile", O_RDONLY);
  if (fd < 0) {
    printf("test_mmap_basic: cannot open testfile\n");
    exit(1);
  }

  // Map the file (2 pages = 8192 bytes)
  p = (char*)mmap(0, 8192, PROT_READ, MAP_PRIVATE, fd, 0);
  if (p == (char*)-1) {
    printf("test_mmap_basic: mmap failed\n");
    close(fd);
    exit(1);
  }

  printf("mmap returned: %p\n", p);

  // Access first byte - should trigger page fault and load page 0
  printf("First char: %c\n", p[0]);

  // Access byte in second page - should trigger page fault and load page 1
  printf("Char at 5000: %c\n", p[5000]);

  // Read and print the mapped content
  printf("Mapped content: ");
  for (int i = 0; i < len && i < 8192; i++) {
    printf("%c", p[i]);
  }
  printf("\n");

  // Unmap
  if (munmap(p, 8192) < 0) {
    printf("test_mmap_basic: munmap failed\n");
    close(fd);
    exit(1);
  }

  close(fd);
  unlink("testfile");  // Cleanup test file
  printf("test_mmap_basic: PASS\n");
  exit(0);
}
