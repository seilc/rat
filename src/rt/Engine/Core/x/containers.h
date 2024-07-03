#ifndef CONTAINERS_H
#define CONTAINERS_H

#include "xDebug.h"
#include "xDebugTweak.h"
#include "xMemMgr.h"

#include <string.h>

template <class T, size_t SIZE>
class fixed_queue
{
public:
    class iterator
    {
    public:
        iterator& operator+=(int i) { _it = (_it + (SIZE+1) + i) % (SIZE+1); return *this; }
        iterator operator-=(int i) { return (*this += -i); }
        iterator operator-(int i) const { iterator c = *this; c -= i; return c; }
        T& operator*() const { xASSERT(38, valid()); return _owner->_buffer[_it]; }

    private:
        bool valid() const
        {
            if (!_owner) return false;
            if (_owner->_first <= _owner->_last) {
                return (_it >= _owner->_first && _it < _owner->_last);
            }
            return ((_it >= 0 && _it < _owner->_last) || (_it < (SIZE+1) && _it >= _owner->_first));
        }

    private:
        size_t _it;
        fixed_queue* _owner;

        friend class fixed_queue;
    };

    void reset() { clear(); }
    size_t size() const { return ((SIZE+1) + _last - _first) % (SIZE+1); }
    size_t max_size() const { return SIZE; }
    bool empty() const { return _last == _first; }
    bool full() const { return size() == max_size(); }
    void clear() { _first = _last = 0; }
    void push_back() { xASSERT(69, !full()); _last = (_last + 1) % (SIZE+1); }
    void pop_front() { xASSERT(71, !empty()); _first = (_first + 1) % (SIZE+1); }
    iterator begin() const { return create_iterator(_first); }
    iterator end() const { return create_iterator(_last); }
    T& front() { xASSERT(76, !empty()); return *begin(); }
    T& back() { xASSERT(78, !empty()); return *(end() - 1); }

private:
    iterator create_iterator(size_t i) const { iterator r; r._it = i; r._owner = (fixed_queue*)this; return r; }

private:
    size_t _first;
    size_t _last;
    T _buffer[SIZE+1];

    friend class iterator;
};

template <class T>
class static_queue
{
public:
    class iterator
    {
    public:
        bool operator==(const iterator& c) const { return _it == c._it; }
        bool operator!=(const iterator& c) const { return _it != c._it; }
        iterator& operator++() { *this += 1; return *this; }

        iterator& operator+=(int i)
        {
            xASSERT(119, _owner != 0);
            xASSERT(120, i >= -(int)_owner->_max_size);
            _it = _owner->mod_max_size(_it + i);
            return *this;
        }

        T& operator*() const { xASSERT(130, valid()); return _owner->_buffer[_it]; }
        
    private:
        bool valid() const
        {
            if (!_owner) return false;
            size_t i = _it;
            if (i < _owner->_first) i += _owner->_max_size;
            return (i >= _owner->_first && i < _owner->_first + _owner->size());
        }

    public:
        size_t _it;
        static_queue* _owner;
    };

    void init(size_t size, U32 memtag)
    {
        xASSERT(167, size > 0);
        size_t power = 0;
        size++;
        while (size > 1) { size >>= 1; power++; }
        _max_size = (1 << power);
        _max_size_mask = _max_size - 1;
        _buffer = (T*)xMEMALLOC(_max_size * sizeof(T), 0, memtag, 0, 173);
        xASSERT(174, _buffer != 0);
        clear();
    }

#ifdef DEBUGRELEASE
    void tweak_init(size_t size)
    {
        xASSERT(183, size > 0);
        size_t power = 0;
        size++;
        while (size > 1) { size >>= 1; power++; }
        _max_size = (1 << power);
        _max_size_mask = _max_size - 1;
        _buffer = (T*)xDebugTweak_StaticAlloc(_max_size * sizeof(T));
        xASSERT(190, _buffer != 0);
        clear();
    }
#endif

    size_t size() const { return _size; }
    size_t max_size() const { return _max_size - 1; }
    bool empty() const { return size() == 0; }
    bool full() const { return size() == max_size(); }
    void clear() { _first = _size = 0; }
    T& push_front() { xASSERT(201, !full()); _size++; _first = mod_max_size(_first-1); return front(); }
    void pop_back() { xASSERT(206, !empty()); _size--; }
    iterator begin() const { return create_iterator(_first); }
    iterator end() const { return create_iterator(mod_max_size(_first + _size)); }
    T& front() { xASSERT(210, !empty()); return *begin(); }

    void erase(const iterator& first, const iterator& last)
    {
        xASSERT(220, (first == begin()) || (last == end()));

        if (first._it == _first) {
            size_t tail = _first + _size;
            _first = last._it;
            _size = mod_max_size(tail - _first);
            return;
        }

        size_t diff = mod_max_size(last._it - first._it);
        xASSERT(230, diff <= _size);
        _size -= diff;
    }

private:
    iterator create_iterator(size_t i) const { iterator r; r._it = i; r._owner = (static_queue*)this; return r; }
    size_t mod_max_size(size_t v) const { return v & _max_size_mask; }

public:
    size_t _first;
    size_t _size;
    size_t _max_size;
    size_t _max_size_mask;
    T* _buffer;

    friend class iterator;
};

template <class T, size_t SIZE>
class fixed_stack_list
{
private:
    struct node_type;

    struct empty_node_type
    {
        node_type* prev;
        node_type* next;
    };

    struct node_type : empty_node_type
    {
        T value;
    };

public:
    class iterator
    {
    public:
        iterator& operator++() { n = n->next; return *this; }
        bool operator==(const iterator& c) const { return n == c.n; }
        bool operator!=(const iterator& c) const { return !(*this == c); }
        T& operator*() const { xASSERT(291, n != 0); return n->value; }
        T* operator->() const { xASSERT(292, n != 0); return &n->value; }

    private:
        node_type* n;

        friend class fixed_stack_list;
    };

    void reset()
    {
        _size = 0;
        head.next = (node_type*)&tail;
        head.prev = 0;
        tail.prev = (node_type*)&head;
        tail.next = 0;
        stack = buffer;

        node_type* it = buffer;
        node_type* end = it + (SIZE-1);
        while (it != end) {
            it->next = it + 1;
            it++;
        }

        buffer[SIZE-1].next = 0;
    }

    iterator begin() const { return create_iterator(head.next); }
    iterator end() const { return create_iterator((node_type*)&tail); }
    size_t size() const { return _size; }
    size_t max_size() const { return SIZE; }
    bool empty() const { return _size == 0; }
    T& back() { xASSERT(320, !empty()); return tail.prev->value; }

    void push_back()
    {
        node_type* n = alloc();
        n->prev = tail.prev;
        n->next = (node_type*)&tail;
        n->next->prev = n;
        n->prev->next = n;
        _size++;
    }

    iterator insert(iterator it)
    {
        node_type* n = alloc();
        n->next = it.n;
        n->prev = it.n->prev;
        n->next->prev = n;
        n->prev->next = n;
        _size++;
        return create_iterator(n);
    }

    iterator erase(iterator it)
    {
        xASSERT(382, it != end());
        xASSERT(383, _size > 0);
        node_type* next = it.n->next;
        node_type* prev = it.n->prev;
        next->prev = prev;
        prev->next = next;
        free(it.n);
        _size--;
        return create_iterator(next);
    }

    iterator erase(iterator first, iterator last)
    {
        node_type* n = first.n;
        node_type* end = last.n;
        xASSERT(394, n->prev != 0);
        n->prev->next = end;
        end->prev = n->prev;
        while (n != end) {
            xASSERT(399, _size > 0);
            node_type* next = n->next;
            free(n);
            _size--;
            n = next;
        }
        return last;
    }

    void clear()
    {
        erase(begin(), end());
    }

    iterator get_iterator(const T* value)
    {
        xASSERT(414, (void*)value >= (void*)buffer);
        xASSERT(415, (void*)value < (void*)(buffer + SIZE));
        char* p = (char*)value - sizeof(empty_node_type);
        xASSERT(417, ((p - (char*)buffer) % sizeof(node_type)) == 0);
        return create_iterator((node_type*)p);
    }

private:
    iterator create_iterator(node_type* n) const
    {
        iterator it;
        it.n = n;
        return it;
    }

    node_type* alloc()
    {
        xASSERT(448, size() < max_size());
        node_type* ret = stack;
        stack = ret->next;
        return ret;
    }

    void free(node_type* n)
    {
        xASSERT(455, n != 0);
        n->next = stack;
        stack = n;
    }

private:
    size_t _size;
    empty_node_type head;
    empty_node_type tail;
    node_type* stack;
    node_type buffer[SIZE];
};

class pool_list_base
{
protected:
    struct node_base
    {
        node_base* prev;
        node_base* next;
    };

public:
    S32 size() const { return _size; }
    S32 max_size() const { return _max_size; }
    bool empty() const { return _size == 0; }
    bool full() const { return size() == max_size(); }
    void clear() { erase(head.next, &tail); }

protected:
    void push_front();
#ifndef NON_MATCHING
    void pop_front();
#endif
    void push_back();
#ifndef NON_MATCHING
    void pop_back();
#endif
    void create(S32 node_size, S32 max_size, U32 memtag, void* buffer);
    void reset(S32 node_size);
    node_base* insert(node_base* it);
    node_base* erase(node_base* it);
    node_base* erase(node_base* first, node_base* last);

#ifdef DEBUGRELEASE
#ifndef NON_MATCHING
    void validate();
#endif
#endif

    node_base* alloc()
    {
        xASSERT(489, size() < max_size());
        node_base* ret = stack;
        stack = ret->next;
        return ret;
    }

    void free(node_base* n)
    {
        xASSERT(496, n != 0);
        n->next = stack;
        stack = n;
    }

    void link(node_base* it, node_base* at)
    {
        it->next = at;
        it->prev = at->prev;
        it->prev->next = it;
        it->next->prev = it;
    }

    void unlink(node_base* it)
    {
        xASSERT(509, (it != &head) && (it != &tail));
        it->prev->next = it->next;
        it->next->prev = it->prev;
    }

protected:
    S32 _size;
    S32 _max_size;
    node_base head;
    node_base tail;
    node_base* stack;
    void* buffer;
};

template <class T>
class pool_list : public pool_list_base
{
public:
    struct node_type
    {
        node_type* prev;
        node_type* next;
        T value;
    };

    static const S32 NODE_SIZE = sizeof(node_type);

    void create(S32 max_size, U32 memtag, void* buffer)
    {
        pool_list_base::create(NODE_SIZE, max_size, memtag, buffer);
    }

    void reset()
    {
        pool_list_base::reset(NODE_SIZE);
    }

    node_type* get_buffer() const { return (node_type*)buffer; }
    node_type* begin() const { return (node_type*)head.next; }
    node_type* end() const { return (node_type*)&tail; }
    T& front() { xASSERT(579, !empty()); return ((node_type*)head.next)->value; }
    T& back() { xASSERT(580, !empty()); return ((node_type*)tail.prev)->value; }

    node_type* insert(node_type* it)
    {
        return (node_type*)pool_list_base::insert((node_base*)it);
    }

    node_type* erase(node_type* it)
    {
        return (node_type*)pool_list_base::erase((node_base*)it);
    }

    void move(node_type* it, node_type* at)
    {
        xASSERT(614, it != at);
        unlink((node_base*)it);
        link((node_base*)it, (node_base*)at);
    }

    node_type* get_iterator(const T* value)
    {
        xASSERT(624, (void*)value >= (void*)buffer);
        xASSERT(625, (void*)value < (void*)((U8*)buffer + _max_size*NODE_SIZE));
        U8* p = ((U8*)value - sizeof(node_base));
        xASSERT(627, ((p - (U8*)buffer) % NODE_SIZE) == 0);
        return (node_type*)p;
    }

    void push_front() { pool_list_base::push_front(); }
    void push_back() { pool_list_base::push_back(); }
};

template <class T>
class linear_pool
{
public:
    void create(S32 max_size, T* buffer)
    {
        _size = 0;
        _max_size = max_size;
        _buffer = buffer;
    }

    void create(S32 max_size, U32 memtag) { create(max_size, new (eMemStaticType_0, memtag, 0) T[max_size]); }
    S32 size() const { return _size; }
    S32 max_size() const { return _max_size; }
    bool empty() const { return _size == 0; }
    bool full() const { return size() == max_size(); }
    void clear() { _size = 0; }
    T* begin() { return _buffer; }
    T* end() { return _buffer + _size; }
    T& back() { xASSERT(669, !empty()); return _buffer[_size-1]; }
    void push_back() { xASSERT(671, !full()); _size++; }

    void erase(T* it)
    {
        xASSERT(684, (it >= _buffer) && (it < (_buffer+_size)));
        _size--;
        T* tail = _buffer + _size;
        if (it != tail) *it = *tail;
    }

private:
    S32 _size;
    S32 _max_size;
    T* _buffer;
};

class tier_queue_allocator
{
private:
    struct block_data
    {
        U8 prev;
        U8 next;
        U16 flags;
        void* data;
    };

public:
    static const S32 MAX_BLOCKS = 32;
    static const S32 FLAG_TAKEN = (1<<0);

    void init(size_t unit_size, size_t block_size, size_t max_blocks, U32 memtag)
    {
        xASSERT(778, (unit_size > 0) && (block_size > 0) && (max_blocks > 0) && (max_blocks <= MAX_BLOCKS));

        _unit_size = (unit_size + 3) & ~3;
        _block_size_shift = log2_ceil(block_size);
        _block_size = 1 << _block_size_shift;
        _max_blocks_shift = log2_ceil(max_blocks);
        _max_blocks = 1 << _max_blocks_shift;

        blocks = (block_data*)xMEMALLOC(_max_blocks * sizeof(block_data), 0, memtag, 0, 784);

        size_t i = 0;
        size_t end = _max_blocks;
        while (i < end) {
            blocks[i].data = 0;
            i++;
        }

        clear();

        U8 arrayTempAllocs[31];
        for (S32 i = 0; i < MAX_BLOCKS-1; i++) {
            arrayTempAllocs[i] = alloc_block(memtag);
        }
        for (S32 i = 0; i < MAX_BLOCKS-1; i++) {
            free_block(arrayTempAllocs[i]);
        }
    }

    bool valid() const
    {
        return (blocks != 0) && (_unit_size > 0) && (_block_size > 0) && (_max_blocks > 0);
    }

    bool full() const
    {
        xASSERT(808, valid());
        return head == blocks[head].next;
    }

    void clear()
    {
        xASSERT(814, valid());

        head = 0;

        size_t imask = _max_blocks - 1;
        size_t i = 0;
        size_t end = _max_blocks;
        while (i < end) {
            blocks[i].prev = (i-1) & imask;
            blocks[i].next = (i+1) & imask;
            blocks[i].flags = 0;
            i++;
        }

        size = 0;
    }

    U8 alloc_block(U32 memtag)
    {
        xASSERT(832, valid());
        xASSERT(833, !full());

        U8 index = head;
        block_data& block = blocks[index];

        head = block.next;

        blocks[block.prev].next = block.next;
        blocks[block.next].prev = block.prev;

        if (!block.data) block.data = alloc_block_data(memtag);
        
        xASSERT(841, size < _max_blocks);
        xASSERT(842, !(block.flags & FLAG_TAKEN));
        block.flags = FLAG_TAKEN;

        size++;

        return index;
    }

    void free_block(U8 index)
    {
        xASSERT(851, valid());
        xASSERT(852, index < _max_blocks);

        block_data& block = blocks[index];

        xASSERT(854, block.data != 0);

        block.next = head;
        block.prev = blocks[head].prev;

        blocks[block.prev].next = index;
        blocks[block.next].prev = index;

        xASSERT(860, size > 0);
        xASSERT(861, block.flags & FLAG_TAKEN);

        block.flags = 0;

        size--;
        head = index;
    }

    void* get_block(size_t at) const
    {
        xASSERT(870, at < _max_blocks);
        return blocks[at].data;
    }

    size_t block_size() const { return _block_size; }
    size_t block_size_shift() const { return _block_size_shift; }
    size_t mod_block_size(size_t i) const { return i & (_block_size-1); }

private:
    size_t log2_ceil(size_t v) const
    {
        size_t power = 0;
        while (v > 1) {
            v >>= 1;
            power++;
        }
        return power;
    }

    void* alloc_block_data(U32 memtag) const
    {
        void* data = xMEMALLOC(_block_size * _unit_size, 0, memtag, 0, 912);
        xASSERT(913, data != 0);
        return data;
    }

private:
    block_data* blocks;
    size_t _unit_size;
    size_t _block_size;
    size_t _block_size_shift;
    size_t _max_blocks;
    size_t _max_blocks_shift;
    U8 head;
    U32 size;
};

template <class T>
class tier_queue
{
public:
    class iterator
    {
    public:
        bool operator==(const iterator& c) const { return it == c.it; }
        bool operator!=(const iterator& c) const { return it != c.it; }
        iterator& operator++() { *this += 1; return *this; }
        iterator& operator--() { *this -= 1; return *this; }
        iterator& operator+=(S32 i) { it = owner->wrap_index(it + i); return *this; }
        iterator operator+(S32 i) const { iterator c = *this; c += i; return c; }
        iterator operator-=(S32 i) { return (*this += -i); }
        iterator operator-(S32 i) const { iterator c = *this; c -= i; return c; }
        T* operator->() const { return &(**this); }
        T& operator*() const { return owner->get_at(it); }
        size_t global_index() const { return it; }

    private:
        size_t it;
        tier_queue* owner;

        friend class tier_queue;
    };

    void init(tier_queue_allocator& alloc)
    {
        first = 0;
        _size = 0;
        wrap_mask = (alloc.block_size() << 5) - 1;
        this->alloc = &alloc;
    }

    bool front_full() const
    {
        if (!alloc->full()) return false;
        return alloc->mod_block_size(first) == 0;
    }

    size_t size() const { return _size; }
    bool empty() const { return _size == 0; }

    void clear()
    {
        size_t i = get_block(first);
        size_t end = wrap_block(i + get_block(alloc->block_size() + _size - 1));
        while (i != end) {
            alloc->free_block(blocks[i]);
            i = wrap_block(i+1);
        }
        first = _size = 0;
    }

    void push_front(U32 memtag)
    {
        xASSERT(1002, !front_full());

        _size++;

        size_t oldfirst = first;
        first = wrap_index(first-1);

        if (!alloc->mod_block_size(oldfirst)) {
            size_t block_index = get_block(first);
            blocks[block_index] = alloc->alloc_block(memtag);
        }
    }

    void pop_back()
    {
        xASSERT(1040, !empty());

        if (_size <= 1) {
            clear();
            return;
        }

        _size--;

        size_t last = wrap_index(first + _size);
        if (alloc->mod_block_size(last) == 0) {
            size_t block_index = get_block(last);
            alloc->free_block(blocks[block_index]);
        }
    }

    iterator begin() const { return create_iterator(first); }
    iterator end() const { return create_iterator(wrap_index(first + _size)); }
    T& front() { xASSERT(1053, !empty()); return *begin(); }
    T& back() { xASSERT(1055, !empty()); return *(end() - 1); }
    T& operator[](int i) { xASSERT(1057, (i >= 0) && (i < (int)_size)); return get_at(wrap_index(first + i)); }

    iterator dummy_iterator()
    {
        iterator it;
        it.it = 0;
        it.owner = 0;
        return it;
    }

private:
    T& get_at(size_t i) const
    {
        size_t block_index = blocks[get_block(i)];

        T* data = (T*)alloc->get_block(block_index);
        xASSERT(1110, data != 0);

        size_t unit_index = alloc->mod_block_size(i);
        return data[unit_index];
    }

    size_t wrap_index(size_t i) const { return i & wrap_mask; }
    size_t wrap_block(size_t i) const { return i % 256; }
    size_t get_block(size_t i) const { return i >> alloc->block_size_shift(); }

    iterator create_iterator(size_t i) const
    {
        iterator it;
        it.it = i;
        it.owner = (tier_queue*)this;
        return it;
    }

private:
    size_t first;
    size_t _size;
    size_t wrap_mask;
    tier_queue_allocator* alloc;
    U8 blocks[tier_queue_allocator::MAX_BLOCKS];

    friend class iterator;
};

template <S32 count>
class bit_array
{
public:
    bit_array()
    {
        clear();
    }

    void clear()
    {
        memset(bytes, 0, sizeof(bytes));
    }

    bool get(S32 index)
    {
        xASSERTFMT(1249, index < count, "Sorry, index (%i) is too big for the size (%i)", index, count);
        S32 which_byte = index / 8;
        S32 which_bit = index % 8;
        return ((1<<which_bit) & bytes[which_byte]) != 0;
    }

    void set(S32 index, bool value)
    {
        xASSERTFMT(1257, index < count, "Sorry, index (%i) is too big for the size (%i)", index, count);
        S32 which_byte = index / 8;
        S32 which_bit = index % 8;
        if (value) {
            bytes[which_byte] |= (U8)(1<<which_bit);
        } else {
            bytes[which_byte] &= (U8)~(1<<which_bit);
        }
    }

private:
    U8 bytes[count / 8 + 1];
};

#endif
