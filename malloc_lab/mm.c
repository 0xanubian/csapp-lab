/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "my solo team",
    /* First member's full name */
    "kumar anshuman (0xanubian)",
    /* First member's email address */
    "kumaranshuman1553@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* definitions by me */

/* word size */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* pack size and alloc bit */
#define PACK(size, alloc)  ((size) | (alloc))

/* get and put the value stored at a chunk pointer */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* retrive size and allocation bit from a chunk pointer */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* retrieve header and footer address from a block pointer */
#define HDRP(bp)       ((char *)(bp) - DSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - (2 * DSIZE))

/* retrieve address of next and previous block pointer from current block pointer */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - DSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - (2 * DSIZE))))

/* retrive the next_freed_pointer from a freed block pointer */
#define NEXT_FREEDP(bp)     GET(bp)

/* retrive the prev_freed_pointer from a freed block pointer */
#define PREV_FREEDP(bp)     GET(bp+WSIZE)

/* put the next_freed_pointer to a freed block pointer */
#define PUT_NEXT(bp, val)    PUT(bp, val)

/*put the prev_freed_pointer to a freed block pointer */
#define PUT_PREV(bp, val)    PUT(bp+WSIZE, val);

static void *heap_listp;
void *free_listp = 0;

/* insert a free chunk at the head of free list */
void insert_at_head(void *bp) 
{
    PUT_PREV(bp, 0);
    PUT_NEXT(bp, (long)free_listp);
    if(free_listp != 0)
        PUT_PREV(free_listp, (long)bp);
    free_listp = bp;
}

/* unlinks a free chunk from the linked list of freed chunks */
void unlink_chunk(void *bp)
{
    if (PREV_FREEDP(bp) != 0)
        PUT_NEXT(PREV_FREEDP(bp), NEXT_FREEDP(bp));
    else free_listp = (void *)NEXT_FREEDP(bp);
    
    if (NEXT_FREEDP(bp) != 0)
        PUT_PREV(NEXT_FREEDP(bp), PREV_FREEDP(bp));
}

/* coalesce any contigous freed chunks */
static void *coalesce(void *bp)
{
    int prev_chunk_allocated = GET_ALLOC(bp-DSIZE-DSIZE);
    int next_chunk_allocated = GET_ALLOC(FTRP(bp)+DSIZE);
    size_t size = GET_SIZE(HDRP(bp));

    /* if both adjacent chunks are allocated then return bp */
    if (prev_chunk_allocated && next_chunk_allocated)
        return bp;

    /*
     * if prev adjacent chunk is allocated but next is freed then increase the 
     * size of current chunk with of next chunk then unlink both chunks from
     * the free list and insert the new coalesced chunk at the head of free list
     */
    else if (prev_chunk_allocated && !next_chunk_allocated) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        
        unlink_chunk(bp);
        unlink_chunk(NEXT_BLKP(bp));
        insert_at_head(bp);
    }
    
    /*
     * if prev adjacent chunk is freed but next is not then increase the size
     * of prev chunk with of the current chunk then unlink both chunks from the
     * free list and insert the new coalesced chunk at the head of free list 
     */
    else if (!prev_chunk_allocated && next_chunk_allocated) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

        unlink_chunk(bp);
        unlink_chunk(PREV_BLKP(bp));
        insert_at_head(PREV_BLKP(bp));
        bp = PREV_BLKP(bp);
    }

    /*
     * if both adcanet chunks are freed then increment size with the size of 
     * but adjacent chunks then unlink all 3 chunks and insert the coalesced 
     * one at the head of free list
     */
    else if (!prev_chunk_allocated && !next_chunk_allocated) {
        size += GET_SIZE(bp-DSIZE-DSIZE);
        size += GET_SIZE(FTRP(bp)+DSIZE);
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));

        unlink_chunk(bp);
        unlink_chunk(PREV_BLKP(bp));
        unlink_chunk(NEXT_BLKP(bp));
        insert_at_head(PREV_BLKP(bp));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

/* extends the heap by the given word count */
static void *extend_heap(size_t words)
{
    void *bp;
    size_t size;

    /* adjust no of words to form allignement and then initialize size */
    size = (words % 2) ? (words + 1) * DSIZE : words * DSIZE;

    /* calls mem_sbrk to extend the heap */
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* puts the header and pooter into the new free chunk and puts the new epilogue */
    PUT(bp, PACK(size, 0));
    PUT(bp+size-DSIZE, PACK(size, 0));
    PUT(bp+size, PACK(0, 1));
    bp += DSIZE;

    /* calls coalesce to coalesce any contigous free chunk */
    return coalesce(bp);
}

/* traverses the segregated free list and if the size of a freed chunk is 
 * greater than or equal to the size requested then it sets the allocaated
 * of the chunk, unlinks it from the free list and return the block pointer
 * otherwise it returns NULL
 */
void *find_free(size_t size)
{
    void *bp = free_listp;

    while (bp != 0) {
        size_t chunk_size = GET_SIZE(HDRP(bp));
        if (chunk_size == 0)
            return NULL;

        if (chunk_size >= size) {
            unlink_chunk(bp);
            PUT(HDRP(bp), PACK(chunk_size, 1));
            PUT(FTRP(bp), PACK(chunk_size, 1));
            return bp;
        }
        bp = NEXT_BLKP(bp);
    }
    return NULL;
}

void place(void *bp, size_t size)
{
    if (GET_SIZE(HDRP(bp)) >= 2 * size) {
        size_t default_size = GET_SIZE(HDRP(bp));
        size_t curr_size = size;
        size_t next_size = default_size - size;

        // put size and alloc bit of the allocated current block
        PUT(HDRP(bp), PACK(curr_size, 1));
        PUT(FTRP(bp), PACK(curr_size, 1));

        // put size and unalloc bit of the new freed block
        void *nxt_bk = NEXT_BLKP(bp);
        PUT(HDRP(nxt_bk), PACK(next_size, 0));
        PUT(FTRP(nxt_bk), PACK(next_size, 0));
        insert_at_head(nxt_bk);
    }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* call mem_sbrk to setup an empty free list */
    if ((long)(heap_listp = mem_sbrk(4 * WSIZE)) == -1)
        return -1;
    
    /* puts padding, prologue and epilogue into the empty free list */
    PUT((char *)(heap_listp), PACK(8, 1));
    PUT(((char *)heap_listp + DSIZE), PACK(8, 1));
    heap_listp = (char*)heap_listp + (2 * DSIZE);
    PUT(heap_listp, PACK(0, 1));

    if (extend_heap(CHUNKSIZE/DSIZE) == NULL)
        return -1;

    return 0;
}

/* 
 * mm_malloc - allocate a new chunk by first searching the free list. If no
 * freed block satisfies the size then extend the heap and cut extra size in 
 * bothe condition by calling place(). returns the payload address of new chunk
 * allocated
 */
void *mm_malloc(size_t size)
{
    if (size == 0)
        return NULL;

    size_t asize = ALIGN(size);
    asize += 16;
    
    void *bp = find_free(asize);
    if (bp != NULL) {
        place(bp, asize);
        return bp;
    }

    asize = MAX(asize, CHUNKSIZE);
    bp = extend_heap(asize/DSIZE);
    PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
    PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
  
    place(bp, asize);
    return bp;
}

/*
 * mm_free - sets the allocated bit of the chunk to 0 and adds the chunk to 
 * free list and then calls coalesce().
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    insert_at_head(ptr);
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














