/*
 * DOS near-heap descriptor allocation and compaction.
 * Placement and boundary evidence are recorded in docs/ORDER.md.
 */
#include "wc1.h"

static volatile short nNearHeapActive = 0; /* 0x004688c0 */
short nNearHeapMaxDescriptors = 0x80; /* 0x004688c4 */
static volatile int nNearHeapRelocationBytes = 0; /* 0x004688c8 */

/* Function start: 0x40E890 */
unsigned short MergeAdjacentNearHeapBlocks(int descriptorAddress)
{
    NearHeapBlock *block;

    block = DosNearPtrToFar(descriptorAddress);
    if ((block->sizeAndFlags & 0x80000000) == 0 &&
        (block[1].sizeAndFlags & 0x80000000) == 0 &&
        block->address + (block->sizeAndFlags & 0xfffff) ==
            block[1].address) {
        block[1].address = block->address;
        block[1].sizeAndFlags += block->sizeAndFlags & 0xfffff;
        while (descriptorAddress > nNearHeapFirstDescriptor) {
            descriptorAddress -= 8;
            block = DosNearPtrToFar(descriptorAddress);
            block[1].address = block->address;
            block[1].sizeAndFlags = block->sizeAndFlags;
        }
        nNearHeapFirstDescriptor += 8;
        return 1;
    }
    return 0;
}

/* Function start: 0x40E900 */
int ReleaseNearHeapBlock(int descriptorAddress)
{
    NearHeapBlock *block;
    int nextDescriptorAddress;

    block = DosNearPtrToFar(descriptorAddress);
    block->sizeAndFlags &= 0x7fffffff;
    nextDescriptorAddress = descriptorAddress + 8;
    if (nextDescriptorAddress <
            nNearHeapBase + nNearHeapSize &&
        MergeAdjacentNearHeapBlocks(descriptorAddress) != 0)
        descriptorAddress = nextDescriptorAddress;
    if (descriptorAddress > nNearHeapFirstDescriptor)
        MergeAdjacentNearHeapBlocks(descriptorAddress - 8);
    return descriptorAddress;
}

/* Function start: 0x40E950 */
void PurgeNearHeapBlocks(unsigned short flags)
{
    NearHeapBlock *block;
    int descriptorAddress;
    int descriptorBytes;

    if (nNearHeapActive != 0) {
        if ((flags & 0x10) != 0) {
            descriptorAddress = nNearHeapBase;
            descriptorAddress += nNearHeapSize;
            descriptorAddress -= 8;
            nNearHeapFirstDescriptor = descriptorAddress;
            block = DosNearPtrToFar(descriptorAddress);
            block->address = nNearHeapBase;
            descriptorBytes = nNearHeapMaxDescriptors * 8;
            block->sizeAndFlags =
                nNearHeapSize - descriptorBytes;
            return;
        }
        descriptorAddress = nNearHeapBase +
                            nNearHeapSize - 8;
        for (; descriptorAddress >= nNearHeapFirstDescriptor;
             descriptorAddress -= 8) {
            block = DosNearPtrToFar(descriptorAddress);
            if ((block->sizeAndFlags & 0x40000000) == 0)
                descriptorAddress = ReleaseNearHeapBlock(descriptorAddress);
        }
    }
}

/* Function start: 0x40E9E0 */
unsigned short InitializeNearHeap(void)
{
    short initialSize;
    int adjustedSize;

    if (nNearHeapActive == 0) {
        initialSize = (short)GetNavRangeSentinel();
        pNearHeapAllocation = 0;
        nNearHeapSize = initialSize;
        if (nNearHeapMaxDescriptors * 8 < nNearHeapSize)
            pNearHeapAllocation =
                AllocateTaggedMemory(nNearHeapSize, 0);
        if (pNearHeapAllocation != 0) {
            nNearHeapActive++;
            nNearHeapBase =
                DosFarPtrToNear(pNearHeapAllocation);
            if (*(unsigned short *)0x00400013 == 0x270) {
                adjustedSize = 0x9c000 - nNearHeapSize;
                adjustedSize -= nNearHeapBase;
                nNearHeapRelocationBytes = adjustedSize;
                adjustedSize =
                    0x98000 - nNearHeapRelocationBytes;
                adjustedSize -= nNearHeapBase;
                nNearHeapSize = adjustedSize;
                if (nNearHeapMaxDescriptors * 8 <
                    nNearHeapSize) {
                    DosMemcpy(
                        DosNearPtrToFar(
                            nNearHeapBase +
                            nNearHeapSize),
                        DosNearPtrToFar(
                            0x9c000 -
                            nNearHeapRelocationBytes),
                        nNearHeapRelocationBytes);
                } else {
                    FreeIfNotNull(pNearHeapAllocation);
                    nNearHeapActive = 0;
                }
            }
            if (nNearHeapActive != 0)
                PurgeNearHeapBlocks(0x10);
        }
    }
    return nNearHeapActive;
}

/* Function start: 0x40EB10 */
/* Nothing in the shipped executable calls this. It undoes
   InitializeNearHeap: copies the relocated bytes back below 0x9C000 and
   frees the heap's allocation. */
void ShutdownNearHeap(void)
{
    int relocationBytes;

    relocationBytes = nNearHeapRelocationBytes;
    if (relocationBytes != 0)
        DosMemcpy(DosNearPtrToFar(0x9c000 - relocationBytes),
                  DosNearPtrToFar(nNearHeapBase + nNearHeapSize),
                  relocationBytes);
    if (nNearHeapActive != 0)
        FreeIfNotNull(pNearHeapAllocation);
    nNearHeapActive = 0;
}

/* Function start: 0x40EB70 */
void *AllocateNearHeapBlockFromEnd(int size, unsigned short flags)
{
    NearHeapBlock *block;
    NearHeapBlock *lastBlock;
    int descriptorAddress;
    int shiftAddress;
    int allocationAddress;
    int blockSize;
    unsigned int allocationFlags;
    unsigned short alignment;

    if (size <= 0)
        return 0;
    allocationFlags = 0x80000000;
    alignment = flags & 3;
    if (alignment == 1) {
        allocationFlags = 0x90000000;
        size++;
    } else if (alignment == 2) {
        allocationFlags = 0xa0000000;
        size += 0xf;
    }
    if ((flags & 0x10) != 0)
        allocationFlags |= 0x40000000;

    allocationAddress = 0;
    descriptorAddress =
        nNearHeapBase + nNearHeapSize - 8;
    for (; descriptorAddress >= nNearHeapFirstDescriptor;
         descriptorAddress -= 8) {
        block = DosNearPtrToFar(descriptorAddress);
        blockSize = block->sizeAndFlags & 0xfffff;
        if ((block->sizeAndFlags & 0x80000000) != 0)
            continue;
        if ((int)blockSize < size)
            continue;
        if ((int)blockSize > size) {
            if (nNearHeapBase -
                    nNearHeapMaxDescriptors * 8 +
                    nNearHeapSize >=
                nNearHeapFirstDescriptor) {
                lastBlock = DosNearPtrToFar(
                    nNearHeapBase +
                    nNearHeapSize - 8);
                if ((lastBlock->sizeAndFlags & 0x80000000) == 0) {
                    blockSize = lastBlock->sizeAndFlags & 0xfffff;
                    if (lastBlock->address + blockSize ==
                            nNearHeapFirstDescriptor &&
                        (int)blockSize > 8) {
                        lastBlock->sizeAndFlags -= 8;
                        nNearHeapMaxDescriptors++;
                    }
                }
            }

            if (nNearHeapBase -
                    nNearHeapMaxDescriptors * 8 +
                    nNearHeapSize >=
                nNearHeapFirstDescriptor)
                continue;

            nNearHeapFirstDescriptor -= 8;
            shiftAddress = nNearHeapFirstDescriptor;
            for (; descriptorAddress > shiftAddress; shiftAddress += 8) {
                block = DosNearPtrToFar(shiftAddress);
                block->address = block[1].address;
                block->sizeAndFlags = block[1].sizeAndFlags;
            }
            descriptorAddress -= 8;
            block = DosNearPtrToFar(descriptorAddress);
            blockSize = block->sizeAndFlags - size;
            block->sizeAndFlags = blockSize;
            block[1].address += blockSize & 0xffffff;
            blockSize = allocationFlags + size;
            block[1].sizeAndFlags = blockSize;
            allocationAddress = block[1].address;
            break;
        }
        block->sizeAndFlags |= allocationFlags;
        allocationAddress = block->address;
        break;
    }

    if (allocationAddress != 0) {
        if (alignment == 1)
            allocationAddress = (allocationAddress + 1) & 0xfffffffe;
        else if (alignment == 2)
            allocationAddress = (allocationAddress + 0xf) & 0xfffffff0;
    }
    return DosNearPtrToFar(allocationAddress);
}

/* Function start: 0x40ED30 */
void *AllocateNearHeapBlockByFlags(int size, unsigned short flags)
{
    NearHeapBlock *block;
    NearHeapBlock *lastBlock;
    int descriptorAddress;
    int shiftAddress;
    int allocationAddress;
    unsigned int blockSize;
    unsigned int allocationFlags;
    unsigned short alignment;

    if ((flags & 0x20) != 0)
        return AllocateNearHeapBlockFromEnd(size, flags);
    if (size <= 0)
        return 0;
    allocationFlags = 0x80000000;
    alignment = flags & 3;
    if (alignment == 1) {
        allocationFlags = 0x90000000;
        size++;
    } else if (alignment == 2) {
        allocationFlags = 0xa0000000;
        size += 0xf;
    }
    if ((flags & 0x10) != 0)
        allocationFlags |= 0x40000000;

    allocationAddress = 0;
    descriptorAddress = nNearHeapFirstDescriptor;
    for (; descriptorAddress <
               nNearHeapBase + nNearHeapSize;
         descriptorAddress += 8) {
        block = DosNearPtrToFar(descriptorAddress);
        blockSize = block->sizeAndFlags & 0xfffff;
        if ((block->sizeAndFlags & 0x80000000) != 0)
            continue;
        if ((int)blockSize < size)
            continue;
        if ((int)blockSize > size) {
            if (nNearHeapBase -
                    nNearHeapMaxDescriptors * 8 +
                    nNearHeapSize >=
                nNearHeapFirstDescriptor) {
                lastBlock = DosNearPtrToFar(
                    nNearHeapBase +
                    nNearHeapSize - 8);
                if ((lastBlock->sizeAndFlags & 0x80000000) == 0) {
                    blockSize = lastBlock->sizeAndFlags & 0xfffff;
                    if (lastBlock->address + blockSize ==
                            nNearHeapFirstDescriptor &&
                        (int)blockSize > 8) {
                        lastBlock->sizeAndFlags -= 8;
                        nNearHeapMaxDescriptors++;
                    }
                }
            }

            if (nNearHeapBase -
                    nNearHeapMaxDescriptors * 8 +
                    nNearHeapSize >=
                nNearHeapFirstDescriptor)
                continue;

            nNearHeapFirstDescriptor -= 8;
            shiftAddress = nNearHeapFirstDescriptor;
            for (; descriptorAddress > shiftAddress; shiftAddress += 8) {
                block = DosNearPtrToFar(shiftAddress);
                block->address = block[1].address;
                block->sizeAndFlags = block[1].sizeAndFlags;
            }
            descriptorAddress -= 8;
            block = DosNearPtrToFar(descriptorAddress);
            block->sizeAndFlags = allocationFlags + size;
            block[1].address += size;
            block[1].sizeAndFlags -= size;
            allocationAddress = block->address;
            break;
        }
        block->sizeAndFlags |= allocationFlags;
        allocationAddress = block->address;
        break;
    }

    if (allocationAddress != 0) {
        if (alignment == 1)
            allocationAddress = (allocationAddress + 1) & 0xfffffffe;
        else if (alignment == 2)
            allocationAddress = (allocationAddress + 0xf) & 0xfffffff0;
    }
    return DosNearPtrToFar(allocationAddress);
}

/* Function start: 0x40EF00 */
/* Nothing in the shipped executable calls this. Releases the near heap
   block that contains the given pointer. */
void FreeNearHeapBlock(void *pointer)
{
    NearHeapBlock *block;
    int address;
    int descriptorAddress;

    address = DosFarPtrToNear(pointer);
    descriptorAddress =
        nNearHeapBase + nNearHeapSize - 8;
    for (; descriptorAddress >= nNearHeapFirstDescriptor;
         descriptorAddress -= 8) {
        block = DosNearPtrToFar(descriptorAddress);
        if (block->address <= address &&
            block->address + (int)(block->sizeAndFlags & 0xfffff) >
                address) {
            ReleaseNearHeapBlock(descriptorAddress);
            break;
        }
    }
}

/* Function start: 0x40EF60 */
/* Nothing in the shipped executable calls this. Returns the usable size of
   the near heap block that contains the given pointer, or 0. */
int GetNearHeapBlockSize(void *pointer)
{
    NearHeapBlock *block;
    int address;
    int descriptorAddress;
    int size;

    size = 0;
    address = DosFarPtrToNear(pointer);
    descriptorAddress =
        nNearHeapBase + nNearHeapSize - 8;
    for (; descriptorAddress >= nNearHeapFirstDescriptor;
         descriptorAddress -= 8) {
        block = DosNearPtrToFar(descriptorAddress);
        if (address >= block->address &&
            block->address + (int)(block->sizeAndFlags & 0xfffff) >
                address) {
            size = block->sizeAndFlags & 0xfffff;
            if ((block->sizeAndFlags & 0x10000000) != 0)
                size--;
            else if ((block->sizeAndFlags & 0x20000000) != 0)
                size -= 0xf;
            break;
        }
    }
    return size;
}
