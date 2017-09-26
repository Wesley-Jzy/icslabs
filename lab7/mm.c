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
    "515030910211",
    /* First member's full name */
    "姜子悦",
    /* First member's email address */
    "ZiyueJiang@sjtu.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* 
 * My core method: Use a blank_list to save the unused block explicitly
 *
 * My mem struct: # One |...| represents one word #
 * |BLACK_LISTHEAD|PADDING|PADDING|PROLOGUE HD|
 * |PROLOGUE FT|.........DATE........|EDILOGUR|
 *
 * My data struct: 
 * (ALLOCATED) |HEADER|.....|FOOTER|
 * (BLANK)     |HEADER|......|PREV|NEXT|FOOTER|   
 *
 * Init: HEADLIST = NULL
 *
 * Malloc: Search the blank_list for the proper block(first-fit)
 *         if no one fit, expand the mem.
 *         Then, remove the allocated block from list
 *
 * Free: Set the alloc bits to 0, then insert the blank block
 *       into blank_list
 * 
 * Realloc: Malloc, copy the data to new block, then free the old one
 *
 */


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<9)
#define MAX(x,y) ((x) > (y)? (x) : (y))

/* Malloc list macros */

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word ar address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
/* It fits both data block and blank block */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Read and write the pre and next from address bp */
#define LISTHEAD_VAL (GET(blankList_head))
#define GET_PRE(bp) (GET(FTRP(bp) - DSIZE))
#define GET_NEXT(bp) (GET(FTRP(bp) - WSIZE))
#define SET_PRE(bp, val) (PUT((FTRP(bp) - DSIZE), (val)))
#define SET_NEXT(bp, val) (PUT((FTRP(bp) - WSIZE), (val)))

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Global Var */
static void * heap_listp;
static size_t * blankList_head;

/* Help function define */
static void * extend_heap(size_t words);
static void * coalesce(void * bp);
static void * find_fit(size_t new_size);
static void place(void *bp, size_t new_size);
static void insert_blank(void *bp);
static void delete_allocated(void *bp);

/* Check funciton define */
void show_allocated_block(void * bp);
void show_blank_block(void * bp);
void show_block(void * bp);
void show_BlankList();
void show_HeapList();

/* Help funtion */
static void * extend_heap(size_t words) {
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
    insert_blank(bp);

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

static void * coalesce(void * bp) {
    size_t prew_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* Case 1 */
    if (prew_alloc && next_alloc) {
        return bp;
    }

    /* Case 2 */
    else if (!prew_alloc && next_alloc) {
        delete_allocated(bp);
        delete_allocated(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        insert_blank(bp);
    }

    /* Case 3 */
    else if (prew_alloc && !next_alloc) {
        delete_allocated(bp);
        delete_allocated(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert_blank(bp);
    }

    /* Case 4 */
    else {
        delete_allocated(bp);
        delete_allocated(PREV_BLKP(bp));
        delete_allocated(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) 
        + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        insert_blank(bp);
    }

    return bp;
}

static void * find_fit(size_t new_size) {
    /* First fit search */
    void *bp;

    if (LISTHEAD_VAL == NULL) {
        return NULL;
    }

    for (bp = LISTHEAD_VAL; bp != NULL; bp = GET_NEXT(bp)) {
        if (new_size <= GET_SIZE(HDRP(bp))) {
            return bp;
        }
    }

    return NULL;
}

/* Set the alloc = 1 and remove the mem from list  */
static void place(void *bp, size_t new_size) {
    size_t csize = GET_SIZE(HDRP(bp));

    /* Cut the blank place to save mem */
    if ((csize - new_size) >= (2 * DSIZE)) {

        /* remove the allocated mem from blank list */
        delete_allocated(bp);

        /* Cut */
        PUT(HDRP(bp), PACK(new_size, 1));
        PUT(FTRP(bp), PACK(new_size, 1));
        void* new_bp = NEXT_BLKP(bp);
        PUT(HDRP(new_bp), PACK(csize - new_size, 0));
        PUT(FTRP(new_bp), PACK(csize - new_size, 0));

        /* insert the blank into list */
        insert_blank(new_bp);
    }

    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));

        /* remove the allocated mem from blank list */
        delete_allocated(bp);
    }
}

/* Insert the blank into the list_head */
static void insert_blank(void *bp) {
    SET_NEXT(bp, LISTHEAD_VAL);
    SET_PRE(bp, blankList_head);

    if (LISTHEAD_VAL != NULL) {
        SET_PRE(LISTHEAD_VAL, bp);
    }

    LISTHEAD_VAL = bp;
}

/* Delete the allocated block from list_head */
static void delete_allocated(void *bp) {
    if (bp == LISTHEAD_VAL) {
        LISTHEAD_VAL = GET_NEXT(bp);
        if (GET_NEXT(bp) != NULL) {
            SET_PRE(GET_NEXT(bp), blankList_head);
        }
    }

    else {
        SET_NEXT(GET_PRE(bp), GET_NEXT(bp));
        if (GET_NEXT(bp) != NULL) {
            SET_PRE(GET_NEXT(bp), GET_PRE(bp));
        }
    }
}

/* 
 * mm_init - initialize the malloc package.
 * initialize list_head and list_tail
 * The struct is: |list_head_0, list_head_1...,list_tail_0,
 * list_tail_1...| |prologue header, prologue footer, ....data, 
 * epilogue footer
 */
int mm_init(void) {
    /* Create blankList_head */
    if ((heap_listp = mem_sbrk(2 * WSIZE)) == (void*)-1) {
        return -1;
    }
    blankList_head = heap_listp;
    LISTHEAD_VAL = NULL; /* Init list_head */
    heap_listp +=  WSIZE;
    PUT(heap_listp, 0); /* Alignment padding */
    heap_listp +=  WSIZE;

    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)-1) {
        return -1;
    }
    PUT(heap_listp, 0); /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); /* Epilogue header */
    heap_listp += (2 * WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t new_size;
    size_t extend_size;
    char * bp;

    if (size == 0) {
        return NULL;
    }

    if (size <= DSIZE) {
        new_size = 2 * DSIZE;
    }

    else {
        new_size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    if ((bp = find_fit(new_size)) != NULL) {
        place(bp, new_size);
        return bp;
    }

    extend_size = MAX(new_size, CHUNKSIZE);
    if ((bp = extend_heap(extend_size / WSIZE)) == NULL) {
        return NULL;
    }
    place(bp, new_size);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    insert_blank(bp);
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size) {
    /* If bp is NULL, use malloc */
    if(bp == NULL) {
        return mm_malloc(size);
    }
    
    /* If size == 0, use free */
    if(size == 0) {
        mm_free(bp);
        return NULL;
    }

    void *old_bp = bp;
    void *new_bp;
    size_t copySize = GET_SIZE(HDRP(bp));
    
    new_bp = mm_malloc(size);
    if (new_bp == NULL) {
      return NULL;
    }
    if (size < copySize) {
      copySize = size;
    }
    memcpy(new_bp, old_bp, copySize);
    mm_free(old_bp);
    return new_bp;
}

/* Check Function */
void show_allocated_block(void * bp) {
    int size = GET_SIZE(HDRP(bp));
    int alloc = GET_ALLOC(HDRP(bp));
    printf("|Header:size=%d alloc=%d|...|pre=none|next=none|Footer|\n",
            size, alloc );
}

void show_blank_block(void * bp) {
    int size = GET_SIZE(HDRP(bp));
    int alloc = GET_ALLOC(HDRP(bp));
    void * pre = GET_PRE(bp);
    void * next = GET_NEXT(bp);
    printf("|Header:size=%d alloc=%d|...|pre=%d|next=%d|Footer|\n",
            size, alloc, (int)pre, (int)next );
}

void show_block(void * bp) {
    int alloc = GET_ALLOC(HDRP(bp));
    if (alloc == 0) {
        show_blank_block(bp);
    }

    else {
        show_allocated_block(bp);
    }
}

void show_BlankList() {
	void * bp;
    if (LISTHEAD_VAL == NULL) {
        printf("NULL\n");
        return;
    }

    for (bp = LISTHEAD_VAL; bp != NULL; bp = GET_NEXT(bp)) {
        show_block(bp);
    }
    return;
}

void show_HeapList() {
	void * bp;
    if (heap_listp == NULL) {
        printf("NULL\n");
        return;
    }

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = GET_NEXT(bp)) {
        show_block(bp);
    }
    return;
}













