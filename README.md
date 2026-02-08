# EOS32 File System Checker

File system consistency checker for EOS32 disk images. The tool reads an EOS32 partition from a raw disk image, traverses the inode table and directory tree, validates block usage against the free list, and exits with a specific non-zero code on the first detected inconsistency.

## Build

The binary is built as `bin/run`.

```sh
mkdir -p bin
make
```

## Usage

```sh
./bin/run <disk-image> <partition-index>
```

- `<disk-image>`: raw disk image file.
- `<partition-index>`: integer 0–15 (partition table entry).

The checker expects:
- Partition table at sector 1.
- Partition entry size of 32 bytes.
- EOS32 partition type `0x58` (checked as `type & 0x7FFFFFFF == 0x00000058`).
- EOS32 block size of 4096 bytes and sector size of 512 bytes.

On success, it prints `completed` and exits `0`.

## Error Codes

The program terminates on the first detected error with one of the following exit codes:

- `1` Incorrect invocation
- `2` Disk image not found
- `3` I/O error
- `4` Illegal partition index
- `5` Partition does not contain an EOS32 file system
- `6` Memory allocation failure
- `10` Block is neither in a file nor in the free list
- `11` Block is both in a file and in the free list
- `12` Block occurs multiple times in the free list
- `13` Block occurs multiple times in file data
- `14` Inconsistent inode data size vs. referenced blocks
- `15` Inode with link count 0 appears in a directory
- `16` Inode with link count 0 is not free
- `17` Inode appears in N directories but has link count != N
- `18` Inode type field is invalid
- `19` Inode is free but appears in a directory
- `20` Root inode is not a directory
- `21` Directory cannot be reached from root
- `99` Undefined file system error

## Notes

- The `run` script builds and then invokes `./bin/run` with no arguments; it is primarily a convenience wrapper for compilation.
- `tests.txt` documents manual test cases and expected exit codes.
