#pragma once
#include <stdlib.h>
#include <stdio.h>
#define MEMPAGE 4096  //the size of a memory page
#define INIT_POOL_SIZE (MEMPAGE*2) //the initial size of a memory pool
#define INIT_FREE_SIZE (INIT_POOL_SIZE/2)
#define INIT_NODE_NUM (16*32)

#define RADIX_INSERT_VALUE_OCCUPY -1
#define RADIX_INSERT_VALUE_SAME -2
#define RADIX_DELETE_ERROR -3
typedef unsigned int ptr_t;
typedef unsigned int uint32;

#define BITS 2
const int radix_tree_height = sizeof(ptr_t) * 8 / BITS;

//return the value of the specified bit in key
#define CHECK_BITS(key,pos) ((((unsigned int) (key))<<sizeof(int)*8 - ((pos+1))*BITS)>> (sizeof(int)*8-BITS))

typedef struct radix_node_t radix_node_t;

struct radix_node_t {
    radix_node_t* child[4];
    radix_node_t* parent;
    ptr_t value;
};

//use memory pool to decrease the time used when creating new nodes
// put at the beginning of memory pool
typedef struct radix_pool { //circular doubly linked list
    struct radix_pool* next;
    struct radix_pool* prev;
// the location of the first unused memory in the allocated memory pool   
    char* start;
// the size of the unused memory in the allocated memory pool
    size_t size;
}radix_pool, * pool_t;


typedef struct radix_tree_t {
    //root node
    radix_node_t* root;
    //pointer to the pool
    pool_t pool;

    radix_node_t* free;

}radix_tree_t;

//used to enlarge memory pool
pool_t get_new_pool(radix_tree_t* t, size_t num) 
{
    if (num == -1) num = INIT_POOL_SIZE;
    pool_t pool = (pool_t) malloc(num * MEMPAGE);
    if (pool == NULL) return NULL;
    pool->start = (char*)pool + sizeof(radix_pool);
    pool->size = num* MEMPAGE - sizeof(radix_pool);
    pool->next = t->pool->next;
    pool->prev = t->pool;
    t->pool->next->prev = pool;
    t->pool->next = pool;
    t->pool = pool;
    return pool;
}

// create a node, fetch an usable node from memory pool
radix_node_t* radix_node_alloc(radix_tree_t* t) 
{
    radix_node_t* node;
    if (t->free != NULL) {
        node = t->free;
        t->free = node->parent;
    }
    else {
        if (t->pool->size < sizeof(radix_node_t)) {
            get_new_pool(t,-1);
        }
        node = (radix_node_t*)t->pool->start;
        t->pool->start += sizeof(radix_node_t);
        t->pool->size -= sizeof(radix_node_t);
    }
    node->child[0] = NULL;
    node->child[1] = NULL;
    node->child[2] = NULL;
    node->child[3] = NULL;
    node->parent = NULL;
    node->value = NULL;
    return node;

}


radix_tree_t* radix_tree_create()
{
    int i;
    radix_tree_t* tree = (radix_tree_t*)malloc(sizeof(radix_tree_t));
    if (tree == NULL) return NULL;
    char* p = (char*)malloc(INIT_POOL_SIZE);
    radix_node_t* ns;
    if (!p) {
        free(tree); return NULL;
    }
    //
    ((pool_t)p)->next = (pool_t)p;
    ((pool_t)p)->prev = (pool_t)p;
    ns = (radix_node_t*)((char*)p + sizeof(radix_pool));

    for (i = 1; i < INIT_NODE_NUM - 2; ++i) {
        ns[i].parent = &ns[i+1];
    }
    ns[i].parent = NULL;
    ns[0].child[0] = NULL;
    ns[0].child[1] = NULL;
    ns[0].child[2] = NULL;
    ns[0].child[3] = NULL;
    ns[0].parent = NULL;
    ns[0].value = NULL;
    tree->pool = (pool_t)p;
    tree->root = ns;
    tree->free = &ns[1];
    ((pool_t)p)->start = (char*)ns + sizeof(radix_node_t) * INIT_NODE_NUM;
    ((pool_t)p)->size = INIT_POOL_SIZE - sizeof(radix_pool) - sizeof(radix_node_t) * INIT_NODE_NUM;
    return tree;
    
}

int radix_tree_insert(radix_tree_t* t, uint32 key, ptr_t value)
{
    int i, temp;
    radix_node_t* node, * child;
    node = t->root;
    for (i = 0; i < radix_tree_height; i++) {
        temp = CHECK_BITS(key,i);
        if (!node->child[temp]) {
            child = radix_node_alloc(t);
            if (!child) return NULL;
            child->parent = node;
            node->child[temp] = child;
            node = node->child[temp];
        }
        else {
            node = node->child[temp];
        }
    }
    if (node->value == value) return RADIX_INSERT_VALUE_SAME;
    if (node->value != NULL) return RADIX_INSERT_VALUE_OCCUPY;
    node->value = value;
    return 0;
}

int radix_tree_delete(radix_tree_t* t, uint32 key)
{
    radix_node_t* node = t->root, * par;
    int i = 0, temp = 0;
    if (node == NULL) return RADIX_DELETE_ERROR;
    do {
        temp = CHECK_BITS(key, i++);
        node = node->child[temp];
    } while (node && i < radix_tree_height);

    if (node == NULL) return RADIX_DELETE_ERROR;
    par = node->parent;
    par->child[temp] = NULL;
    node->value = NULL;
    node->child[0] = NULL;
    node->child[1] = NULL;
    node->child[2] = NULL;
    node->child[3] = NULL;
    node->parent = t->free->parent;
    t->free->parent = node;
    return 0;
        
}



void radix_print(radix_node_t* node) 
{
    if (node == NULL) return;
    if (node -> value != NULL) 
        printf("%x\n",node->value);
    radix_print(node->child[0]);
    radix_print(node->child[1]);
    radix_print(node->child[2]);
    radix_print(node->child[3]);
}


ptr_t radix_tree_find(radix_tree_t* t, uint32 key)
{
    int i = 0, temp;
    radix_node_t* node;
    node = t->root;
    while (node && i < radix_tree_height) {
        temp = CHECK_BITS(key,i++);
        node = node->child[temp];
    }
    if (node == NULL) return NULL;
    return node->value;
}


int main() {
    radix_tree_t* tree = radix_tree_create();
    radix_tree_insert(tree,32,2048);
    radix_tree_insert(tree,9999,2049);
    radix_print(tree->root);
    radix_tree_delete(tree,9999);
    radix_print(tree->root);
    printf("%x\n",radix_tree_find(tree,32));
}