/*
All the main functions with respect to the MeMS are inplemented here
read the function discription for more details

NOTE: DO NOT CHANGE THE NAME OR SIGNATURE OF FUNCTIONS ALREADY PROVIDED
you are only allowed to implement the functions
you can also make additional helper functions a you wish

REFER DOCUMENTATION FOR MORE DETAILS ON FUNSTIONS AND THEIR FUNCTIONALITY
*/
#include <stdio.h>
#include <stdlib.h>
// add other headers as required
#include <sys/mman.h>

/*
Use this macro where ever you need PAGE_SIZE.
As PAGESIZE can differ system to system we should have flexibility to modify this
macro to make the output of all system same and conduct a fair evaluation.
*/
#define PAGE_SIZE 4096
#define HOLE 0
#define PROCESS 1

// Structure for Sub chain

typedef struct Sub_Node
{
    int type; // 0-holes, 1-process
    int base;
    size_t size; // size of memory segment(bytes)
    struct Sub_Node *prev;
    struct Sub_Node *next;
} Sub_Node;

// Structure for main chain

typedef struct Main_Node
{
    int pages; // No of pages  // size of main_node(bytes) = int pages * PAGE_SIZE
    int base;
    Sub_Node *sub_chain_head;
    struct Main_Node *prev;
    struct Main_Node *next;
} Main_Node;

const size_t SUB_NODE_SIZE = sizeof(Sub_Node);
const size_t MAIN_NODE_SIZE = sizeof(Main_Node);

Main_Node *free_list; // free_list = main_chain_head
Main_Node *main_chain_tail;
void *free_list_page_address;  // address of page where free list is present
void *free_list_memory_offset; // address(inside free list page) free to allocate a node (Main / Sub) in free list

const size_t FREE_LIST_SIZE = 10 * PAGE_SIZE; // assuming max size free list occupy will be 10 * 4KB

/*
Initializes all the required parameters for the MeMS system. The main parameters to be initialized are:
1. the head of the free list i.e. the pointer that points to the head of the free list
2. the starting MeMS virtual address from which the heap in our MeMS virtual address space will start.
3. any other global variable that you want for the MeMS implementation can be initialized here.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_init()
{
    free_list = NULL;
    main_chain_tail = NULL;
    free_list_page_address = mmap(NULL, FREE_LIST_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (free_list_page_address == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }
    free_list_memory_offset = free_list_page_address;
}

/*
This function will be called at the end of the MeMS system and its main job is to unmap the
allocated memory using the munmap system call.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_finish()
{
    // munmap(free_list_page_address, free_list_size);
}

/*
Allocates memory of the specified size by reusing a segment from the free list if
a sufficiently large segment is available.

Else, uses the mmap system call to allocate more memory on the heap and updates
the free list accordingly.

Note that while mapping using mmap do not forget to reuse the unused space from mapping
by adding it to the free list.
Parameter: The size of the memory the user program wants
Returns: MeMS Virtual address (that is created by MeMS)
*/

Sub_Node *find_hole(size_t size_req) // return NULL if size not found
{
    Main_Node *cur_main_node = free_list;
    while (cur_main_node != NULL)
    {
        Sub_Node *cur_sub_node = cur_main_node->sub_chain_head;
        while (cur_sub_node != NULL)
        {
            if ((cur_sub_node->type == HOLE) && (cur_sub_node->size >= size_req))
            {
                return cur_sub_node;
            }
            cur_sub_node = cur_sub_node->next;
        }
        cur_main_node = cur_main_node->next;
    }
    return NULL;
}

void *allocate_hole(Sub_Node *hole, size_t size)
{
    if (hole->size == size)
    {
        hole->type = PROCESS;
        ; // adddon
    }
    else // (hole->size > size)
    {
        Sub_Node *remaining_hole = (Sub_Node *)free_list_memory_offset; // creating a new hole node for unallocated segment
        free_list_memory_offset = (char *)free_list_memory_offset + SUB_NODE_SIZE;

        remaning_hole->prev = hole;
        remaning_hole->next = hole->next;
        remaning_hole->type = HOLE;
        remaning_hole->base = hole->base + size;
        remaning_hole->size = hole->size - size;

        hole->next = remaining_hole;
        if (remaining_hole->next != NULL)
        {
            remaining_hole->next->prev = remaining_hole;
        }

        ; // addon
    }
}

void *mems_malloc(size_t size)
{
    // Edge cases
    if (size <= 0)
    {
        return NULL;
    }

    // Main code
    Sub_Node *free_hole = find_hole(size);
    if (free_hole != NULL) // free hole founded // make free_hole node process node
    {
        return allocate_hole(free_hole, size);
    }
    else // (free_hole == NULL) //  free hole not founded // add new main node
    {
        Main_Node *new_main_node = (Main_Node *)free_list_memory_offset;
        free_list_memory_offset = (char *)free_list_memory_offset + MAIN_NODE_SIZE;
        // (char*)page casts offset to a char*, which treats it as a byte pointer.

        if (free_list == NULL)
        {
            new_main_node->prev = NULL;
            free_list = new_main_node;
        }
        else
        {
            new_main_node->prev = main_chain_tail;
            main_chain_tail->next = new_main_node;
        }

        new_main_node->next = NULL;
        main_chain_tail = new_main_node;

        int total_pages = (size / PAGE_SIZE) + 1; // total pages to allocated for new main node(ideally should use ceil to calculate )
        new_main_node->pages = total_pages;

        Sub_Node *new_head_hole = (Sub_Node *)free_list_memory_offset;
        free_list_memory_offset = (char *)free_list_memory_offset + SUB_NODE_SIZE;

        new_head_hole->prev = NULL;
        new_head_hole->next = NULL;
        new_head_hole->type = HOLE;
        new_head_hole->base = 0;
        new_head_hole->size = total_pages * PAGE_SIZE;

        new_main_node->sub_chain_head = new_head_hole;

        // void *user_memory = mmap(NULL, total_pages * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        // new_main_node->base = user_memory;
    }
}

/*
this function print the stats of the MeMS system like
1. How many pages are utilised by using the mems_malloc
2. how much memory is unused i.e. the memory that is in freelist and is not used.
3. It also prints details about each node in the main chain and each segment (PROCESS or HOLE) in the sub-chain.
Parameter: Nothing
Returns: Nothing but should print the necessary information on STDOUT
*/
void mems_print_stats()
{
    Main_Node *cur_main_node = free_list;
    int mainnode_count = 1;
    while (cur_main_node != NULL)
    {
        Sub_Node *cur_sub_node = cur_main_node->sub_chain_head;
        int subnode_count = 1;
        while (cur_sub_node != NULL)
        {
            cur_sub_node = cur_sub_node->next;
            subnode_count++;
        }
        cur_main_node = cur_main_node->next;
        mainnode_count++;
    }
    cur_main_node = NULL;
}

/*
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: MeMS physical address mapped to the passed ptr (MeMS virtual address).
*/
void *mems_get(void *v_ptr)
{
}

/*
this function free up the memory pointed by our virtual_address and add it to the free list
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: nothing
*/
void mems_free(void *v_ptr)
{
}
