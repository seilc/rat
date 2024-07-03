#include "containers.h"

void pool_list_base::push_front()
{
    node_base* n = alloc();
    n->next = head.next;
    n->prev = &head;
    n->next->prev = n;
    n->prev->next = n;
    _size++;
}

#ifndef NON_MATCHING
void pool_list_base::pop_front()
{
    xASSERT(0, !empty());
}
#endif

void pool_list_base::push_back()
{
    node_base* n = alloc();
    n->prev = tail.prev;
    n->next = &tail;
    n->next->prev = n;
    n->prev->next = n;
    _size++;
}

#ifndef NON_MATCHING
void pool_list_base::pop_back()
{
    node_base* n = tail.prev;
    xASSERT(0, n != 0);
}
#endif

void pool_list_base::create(S32 node_size, S32 max_size, U32 memtag, void* buffer)
{
    xASSERT(41, node_size > sizeof(node_base));
    xASSERT(42, (node_size % 4) == 0);
    xASSERT(43, max_size > 0);

    if (!buffer) {
        this->buffer = xMEMALLOC(node_size * max_size, 0, memtag, 0, 45);
    } else {
        this->buffer = buffer;
    }

    _max_size = max_size;
    reset(node_size);
}

void pool_list_base::reset(S32 node_size)
{
    xASSERT(54, _max_size > 0);
    xASSERT(55, buffer != 0);

    _size = 0;
    head.next = &tail;
    head.prev = 0;
    tail.prev = &head;
    tail.next = 0;
    stack = (node_base*)buffer;

    U8* mem = (U8*)buffer;
    U8* end_mem = mem + (_max_size-1) * node_size;
    while (mem != end_mem) {
        ((node_base*)mem)->next = (node_base*)(mem + node_size);
        mem += node_size;
    }
    ((node_base*)mem)->next = 0;
}

pool_list_base::node_base* pool_list_base::insert(node_base* it)
{
    node_base* alive = head.next;
    while (alive) {
        if (it == alive) break;
        alive = alive->next;
    }
    xASSERT(77, alive != 0);

    node_base* n = alloc();
    n->next = it;
    n->prev = it->prev;
    n->next->prev = n;
    n->prev->next = n;

    _size++;

    return n;
}

pool_list_base::node_base* pool_list_base::erase(node_base* it)
{
#ifdef DEBUGRELEASE
    node_base* alive = head.next;
    while (alive != &tail) {
        if (it == alive) break;
        alive = alive->next;
    }
    xASSERT(96, alive != &tail);

    xASSERT(98, it != &tail);
    xASSERT(99, _size > 0);
#endif

    node_base* next = it->next;
    node_base* prev = it->prev;

    next->prev = prev;
    prev->next = next;

    free(it);

    _size--;

    return next;
}

pool_list_base::node_base* pool_list_base::erase(node_base* first, node_base* last)
{
    node_base* n = first;
    node_base* end = last;

    xASSERT(111, n->prev != 0);

    n->prev->next = end;
    end->prev = n->prev;

    while (n != end) {
        xASSERT(116, _size > 0);
        node_base* next = n->next;
        free(n);
        n = next;
        _size--;
    }
    
    return last;
}

#ifdef DEBUGRELEASE
#ifndef NON_MATCHING
void pool_list_base::validate()
{
    S32 dead_total = 0;
    S32 alive_total = 0;
    node_base* alive = 0;
    xASSERT(0, dead_total == (_max_size - _size));
    xASSERT(0, alive->prev != 0);
    xASSERT(0, alive->next != 0);
    xASSERT(0, alive->prev->next == alive);
    xASSERT(0, alive->next->prev == alive);
    xASSERT(0, alive_total == _size);
}
#endif
#endif
