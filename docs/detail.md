# Malloc lab

I grasp some ideas from glibc, and implement my own malloc.

## Basic structures

Memories are divided into chunks. Each chunk has memory layout as follows:

```C
/**
 * (... Previous Chunk end)
 * -------------------------------- (Lowest Address)
 * Previous chunk size.             (32-bits)
 * Current  chunk size + Flags.     (29-bits + 3-bits)
 * User memory. (Current chunk size - 8 bytes).
 * -------------------------------- (Highest Address)
 * (... Next chunk begining)
 *
 * Flag = Rerserved + Reserved + Previous in use.
*/
```

Moreover, we catergorize the chunk by its size. There are 5 sorts of chunks:

- Fast chunks. Exactly 32 bytes.
- Tiny chunks. Ranging from 48 bytes to 512 bytes.      16 bytes aligned.
- Middle chunks. Ranging from 512 bytes to 4096 bytes.  16 bytes aligned.
- Huge chunks. Ranging from 4096 bytes to 65536 bytes.  16 bytes aligned.
- Extremely large chunks. More than 65535 bytes. (Not supported yet)

### Fast chunks

All user memory no more than 24 bytes, we use fast chunks to manage them. These chunks are all put tightly together in pages. A 4096-aligned page is divided into 4096 / 32 - 1 = 127 available chunks, where the first in-page chunk is used as the header. This huge page just behaves as a 4096-size chunk which has been allocated (previous in use of next chunk is set). The header chunk manages a bitmap (128 bits = 2 bytes) and double link list forvisiting the next chunk. If all chunks in a page goes free, we may free it some time later.

### Tiny chunks

These chunk are in size of 16 * (x + 1), where x is in 2 to 31. We give exactly one slot to each of these sizes. These chunks are easy to merge.
