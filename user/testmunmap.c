#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// Test munmap functionality
int main() {
  int fd;
  char *p;
  char test_content[] = "Test content for munmap test\n";
  int len = strlen(test_content);

  // Create a test file
  fd = open("testfile2", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("test_mmap_munmap: cannot create testfile2\n");
    exit(1);
  }

  // Write test content
  if (write(fd, test_content, len) != len) {
    printf("test_mmap_munmap: write error\n");
    close(fd);
    exit(1);
  }
  close(fd);

  // Open for reading
  fd = open("testfile2", O_RDONLY);
  if (fd < 0) {
    printf("test_mmap_munmap: cannot open testfile2\n");
    exit(1);
  }

  // Map the file
  p = (char*)mmap(0, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
  if (p == (char*)-1) {
    printf("test_mmap_munmap: mmap failed\n");
    close(fd);
    exit(1);
  }

  // Access the mapped memory
  printf("Before munmap: %c\n", p[0]);

  // Unmap
  if (munmap(p, 4096) < 0) {
    printf("test_mmap_munmap: munmap failed\n");
    close(fd);
    exit(1);
  }

  printf("test_mmap_munmap: munmap succeeded\n");

  close(fd);
  unlink("testfile2");  // Cleanup test file
  printf("test_mmap_munmap: PASS\n");
  exit(0);
}
