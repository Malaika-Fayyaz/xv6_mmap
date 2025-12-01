# mmap() / munmap() Implementation for xv6-riscv

## Overview

This implementation adds file-backed memory mapping to xv6-riscv, allowing user processes to map file contents into their virtual address space. Pages are **demand-loaded (lazy)** on page fault, meaning physical pages are only allocated when the process first accesses them.

## Features Implemented

### Must-have (MVP)
- ✅ `mmap()` and `munmap()` syscalls
- ✅ File-backed memory mapping with lazy page loading
- ✅ Reading from mapped regions returns correct file bytes
- ✅ `munmap()` unmaps pages and frees kernel resources
- ✅ Integration with `fork()` (child inherits mapping descriptors)
- ✅ `exec()` clears all mappings
- ✅ Basic test programs verifying lazy load and read mapping

### Nice-to-have (Partial)
- ⚠️ `PROT_WRITE` support: Basic support exists but write-back to file is not implemented in MVP
- ⚠️ `MAP_SHARED` / `MAP_PRIVATE`: Both flags are accepted but write semantics are not fully implemented
- ❌ Dirty tracking and write-back: Not implemented
- ❌ Partial `munmap()` with VMA splitting: Not implemented

## API

### User-facing Functions

```c
void *mmap(void *addr, int length, int prot, int flags, int fd, int offset);
int munmap(void *addr, int length);
```

### Constants

```c
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define MAP_SHARED  0x01
#define MAP_PRIVATE 0x02
```

### Syscall Semantics

**mmap(addr, length, prot, flags, fd, offset)**
- If `addr == NULL` (0), kernel chooses a page-aligned address at or above current heap (`p->sz`)
- `length` must be > 0; mapped region is rounded up to page size (4096 bytes)
- `fd` must be valid and refer to an open file
- `offset` must be page-aligned (4096-byte boundary)
- On success, returns virtual start address. On failure, returns `(void *) -1`

**munmap(addr, length)**
- Unmaps the region `[addr, addr+length)`
- Returns 0 on success, -1 on error

## Implementation Details

### Data Structures

**kernel/proc.h:**
- `struct mmap_area`: Tracks each memory-mapped region
  - `va_start`: Virtual start address
  - `length`: Size in bytes
  - `f`: File pointer
  - `file_offset`: Offset in file
  - `prot`: Protection flags (PROT_READ | PROT_WRITE)
  - `flags`: Mapping flags (MAP_SHARED | MAP_PRIVATE)
  - `used`: Whether this slot is in use

- `struct proc`: Extended with
  - `mmap_areas[MAX_MMAP_AREAS]`: Array of mapped regions (max 16 per process)
  - `mmap_hint`: Next free virtual address hint

### Key Functions

**kernel/vm.c:**
- `find_mmap_area()`: Find VMA containing a virtual address
- `alloc_mmap_area()`: Allocate a new VMA slot
- `remove_mmap_area()`: Remove VMA and close file reference
- `handle_mmap_fault()`: Handle page fault for mapped region (lazy loading)
- `do_mmap()`: Core mmap implementation
- `do_munmap()`: Core munmap implementation

**kernel/sysfile.c:**
- `sys_mmap()`: Syscall wrapper for mmap
- `sys_munmap()`: Syscall wrapper for munmap

**kernel/trap.c:**
- Modified `usertrap()` to call `handle_mmap_fault()` on page faults before falling back to lazy allocation

**kernel/proc.c:**
- Modified `kfork()` to copy mmap areas to child
- Modified `kexec()` to clean up all mmap areas
- Modified `kexit()` to clean up all mmap areas

## Building and Running

### Build xv6

```bash
make
```

This will:
1. Compile the kernel with mmap support
2. Compile user test programs
3. Create the filesystem image

### Run xv6

```bash
make qemu
```

### Run Tests

Inside xv6 shell:

```bash
$ test_mmap_basic
$ test_mmap_munmap
$ test_mmap_fork
```

## Test Programs

### test_mmap_basic
Tests basic mmap functionality:
- Creates a test file
- Maps it into memory
- Accesses different pages to trigger lazy loading
- Verifies correct content is read
- Unmaps the region

### test_mmap_munmap
Tests munmap functionality:
- Maps a file
- Accesses mapped memory
- Unmaps the region
- Verifies unmapping succeeds

### test_mmap_fork
Tests fork integration:
- Parent maps a file
- Forks a child process
- Both parent and child access mapped memory
- Verifies both can read correctly

## Limitations

1. **Page-aligned offsets**: File offsets must be page-aligned (multiples of 4096)
2. **Write-back not implemented**: Writes to MAP_SHARED regions are not written back to file
3. **No dirty tracking**: The kernel does not track which pages have been modified
4. **No partial munmap**: Unmapping part of a VMA does not split it into multiple VMAs
5. **Max mappings**: Limited to 16 mapped regions per process
6. **No MAP_ANONYMOUS**: Only file-backed mappings are supported

## Files Modified

### Kernel
- `kernel/proc.h`: Added `mmap_area` struct and fields to `proc`
- `kernel/syscall.h`: Added `SYS_mmap` and `SYS_munmap`
- `kernel/defs.h`: Added function declarations
- `kernel/vm.c`: Implemented VMA management and fault handling
- `kernel/sysfile.c`: Added syscall wrappers
- `kernel/syscall.c`: Registered new syscalls
- `kernel/trap.c`: Modified to handle mmap page faults
- `kernel/proc.c`: Integrated with fork/exec/exit
- `kernel/exec.c`: Clean up mmap areas on exec

### User
- `user/user.h`: Added prototypes and constants
- `user/usys.pl`: Added syscall stubs
- `user/test_mmap_basic.c`: Basic functionality test
- `user/test_mmap_munmap.c`: Munmap test
- `user/test_mmap_fork.c`: Fork integration test
- `Makefile`: Added test programs to build

## Debugging

To debug mmap issues, you can add kernel printf statements in:
- `do_mmap()`: Log when mappings are created
- `handle_mmap_fault()`: Log when pages are loaded
- `do_munmap()`: Log when mappings are removed

Example debug output:
```c
printf("mmap: pid=%d va=0x%lx len=%lu fd=%d\n", p->pid, start, len, fd);
```

## Future Improvements

1. Implement write-back for MAP_SHARED with PROT_WRITE
2. Add dirty page tracking
3. Implement copy-on-write for MAP_PRIVATE
4. Support partial munmap with VMA splitting
5. Add MAP_ANONYMOUS support
6. Increase MAX_MMAP_AREAS limit
7. Support non-page-aligned offsets

## References

- xv6-riscv documentation
- Linux mmap(2) man page for reference semantics
- RISC-V privileged specification for page fault handling

