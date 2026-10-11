/*
 * In this approach i have a heap that has both prologue and epilogue chunks 
 * in between them is our space to allocate and free chunks. you can extend the
 * heap using extend_heap it will extend the heap at the end and create a new 
 * epilogue. I also have a LIFO explicit free list to search for free chunks for 
 * malloc requests. 
 *
 * mm_malloc searches for free chunks from the explicit free list if found it unlinks 
 * it and mark it allocated. if no free chunk is found then it extends the heap
 * and allocates the new free chunk. It cuts any extra space from the allcoated
 * chunk in both the cases using place().
 *
 * mm_free marks the chunk freed then insert it to the free list then coalesce
 * them to any nearby freed chunk
 *
 * mm_realloc checks if the requested size is less than the original size, if 
 * yes then it shrinks the size if the residual size >= MIN_CHUNK_SIZE and 
 * frees the residual chunk. If not then it checks if the next chunk can be 
 * absorbed into the current chunk otherwise it mm_mallocs a new chunk of 
 * requested size then copies the data there and then frees the old chunk.
 *
 * overall score of my implementation is 84/100
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

/* size of chunk to grow the heap i.e. 4096 or 4KB */
#define CHUNKSIZE (1 << 12)

/* Minimum chunk size in this implementation */
#define MIN_CHUNK_SIZE 24

/* find max between two numbers */
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
#define PREV_FTRP(bp)  ((char *)(bp) - (2 * DSIZE))

/* retrieve address of next and previous block pointer from current block pointer */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - DSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - (2 * DSIZE))))

/* retrive the next_freed_pointer from a freed block pointer */
#define NEXT_FREEDP(bp)     GET(bp)

/* retrive the prev_freed_pointer from a freed block pointer */
#define PREV_FREEDP(bp)     GET((char *)bp+WSIZE)

/* put the next_freed_pointer to a freed block pointer */
#define PUT_NEXT(bp, val)    PUT(bp, val)

/*put the prev_freed_pointer to a freed block pointer */
#define PUT_PREV(bp, val)    PUT((char *)bp+WSIZE, val)

/* a pointer to the start of heap */
static void *heap_listp;

/* a pointer to the explicit free list */
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
    int prev_chunk_allocated = GET_ALLOC(PREV_FTRP(bp));
    int next_chunk_allocated = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
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
        unlink_chunk(bp);
        unlink_chunk(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        
        insert_at_head(bp);
    }
    
    /*
     * if prev adjacent chunk is freed but next is not then increase the size
     * of prev chunk with of the current chunk then unlink both chunks from the
     * free list and insert the new coalesced chunk at the head of free list 
     */
    else if (!prev_chunk_allocated && next_chunk_allocated) {
        unlink_chunk(bp);
        unlink_chunk(PREV_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

        insert_at_head(PREV_BLKP(bp));
        bp = PREV_BLKP(bp);
    }

    /*
     * if both adcanet chunks are freed then increment size with the size of 
     * but adjacent chunks then unlink all 3 chunks and insert the coalesced 
     * one at the head of free list
     */
    else if (!prev_chunk_allocated && !next_chunk_allocated) {
        unlink_chunk(bp);
        unlink_chunk(PREV_BLKP(bp));
        unlink_chunk(NEXT_BLKP(bp));

        size += GET_SIZE(bp-DSIZE-DSIZE);
        size += GET_SIZE(FTRP(bp)+DSIZE);
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));

        insert_at_head(PREV_BLKP(bp));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

/*
 * extends the heap by the given word count and returns the payload addr of 
 * chunk
 */
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
    PUT(HDRP(bp), PACK((size), 0));
    PUT(FTRP(bp), PACK((size), 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    /* insert the newly created free chunk in the free list */
    insert_at_head(bp);

    /* calls coalesce to coalesce any contigous free chunk */
    return coalesce(bp);
}

/* 
 * traverses the explicit free list and if the size of a freed chunk is 
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
        bp = (void *)NEXT_FREEDP(bp);
    }
    return NULL;
}

/*
 * if the residual size is greater or equal to the size of minimum chunk size 
 * then cut that extra size chunk and mark it free and insert it to the explicit
 * free list
 */
void place(void *bp, size_t size)
{
    if (GET_SIZE(HDRP(bp)) >= (size + (3 * DSIZE))) {
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
    /* reset the free_listp address */
    free_listp = NULL;
    /* call mem_sbrk to setup an empty free list */
    if ((long)(heap_listp = mem_sbrk(3 * DSIZE)) == -1)
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

    size_t esize = MAX(asize, (CHUNKSIZE+16));
    bp = extend_heap(esize/DSIZE);
    if (bp == NULL)
        return NULL;

    unlink_chunk(bp);
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
    if (ptr == NULL)
        return;
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
    if (ptr == NULL)
        return mm_malloc(size);

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    size_t asize = ALIGN(size);
    asize += 16;
    size_t initial_chunk_size = GET_SIZE(HDRP(ptr));

    /* 
     * if asize is less than or equal to the size of chunk pointed to by ptr 
     * then shrink the chunk and free the residual chunk if size of residual
     * chunk is greater or equal to minimum chunk size. no need to copy any 
     * payload contents 
     */
    if (asize <= initial_chunk_size) {
        size_t residual_size = initial_chunk_size - asize;
        if (residual_size >= MIN_CHUNK_SIZE) {
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));
            
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(residual_size, 0));
            PUT(FTRP(NEXT_BLKP(ptr)), PACK(residual_size, 0));
            insert_at_head(NEXT_BLKP(ptr));
            coalesce(NEXT_BLKP(ptr));
            return ptr;
        }
        return ptr;
    }

    /* 
     * If next chunk after current chunk is free and it will satisfy the chunk
     * size requirement then absorb the next chunk and see if the residual size
     * is greater or equal to MIN_CHUNK_SIZE, if yes then mark it free and add
     * it to the free list
     */
    int is_next_chunk_freed = !(GET_ALLOC(HDRP(NEXT_BLKP(ptr))));
    size_t next_chunk_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
    size_t new_chunk_size = initial_chunk_size + next_chunk_size;

    if (is_next_chunk_freed && (initial_chunk_size + next_chunk_size >= asize)) {
        unlink_chunk(NEXT_BLKP(ptr));
        PUT(HDRP(ptr), PACK(new_chunk_size, 1));
        PUT(FTRP(ptr), PACK(new_chunk_size, 1));

        size_t residual_size = GET_SIZE(HDRP(ptr)) - asize;
        if (residual_size >= MIN_CHUNK_SIZE) {
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));

            PUT(HDRP(NEXT_BLKP(ptr)), PACK(residual_size, 0));
            PUT(FTRP(NEXT_BLKP(ptr)), PACK(residual_size, 0));
            insert_at_head(NEXT_BLKP(ptr));
            coalesce(NEXT_BLKP(ptr));
        }
        return ptr;
    }

    /*
     * if next chunk is the epilogue then extend the heap and the absorb the 
     * newly created free block 
     */
    if (!is_next_chunk_freed && (next_chunk_size == 0)) {
        size_t deficit = asize - initial_chunk_size;
        //deficit = ALIGN(deficit) + 16;
        void *nbk = extend_heap(deficit/DSIZE);
        if (nbk == NULL)
            return NULL;

        unlink_chunk(nbk);
        size_t nsize = initial_chunk_size + GET_SIZE(HDRP(nbk));
        PUT(HDRP(ptr), PACK(nsize, 1));
        PUT(FTRP(ptr), PACK(nsize, 1));

        return ptr;
    }

    /*
     * last case if we can't find and absorb any next chunk then malloc a new
     * chunk and copy the contents there and free the old chunk 
     */
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    copySize = GET_SIZE(HDRP(ptr));
    copySize = copySize - (2 * DSIZE);
    
    if (size < copySize)
      copySize = size;
    
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














