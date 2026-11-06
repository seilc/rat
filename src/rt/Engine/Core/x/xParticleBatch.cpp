#include "xParticleBatch.h"

#include "xDebug.h"
#include "xDebugTweak.h"
#include "xGlobals.h"
#include "xMath.h"
#include "xMemMgr.h"
#include "xMemory.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zScene.h"

#include <stdlib.h>
#include <string.h>

namespace {

    static const S32 MAX_SYSTEMS = 128;
    static const S32 MAX_SYSTEMS_PER_GROUP = 32;
    static const S32 MAX_BLOCKS = 32;

    namespace tweak {

        void add_tweaks()
        {
            const char* prefix = "Debug|Particle Batch";
        }

    }

    struct block_info
    {
        U8* mem;
        S32 used;
        block_info* next;
    };

    struct internal_system : xParticleBatchSystem
    {
        block_info* block_list;
        S32 blocks_used;
        S32 units_used;
        S32 units_per_block;
        block_info* best_block;
        internal_system* next;
#if DEBUG || RELEASE
        const char* dbgFile;
        S32 dbgLine;
#endif
    };

    struct system_bucket
    {
        RwTexture* texture;
        union
        {
            S32 rskey;
            struct
            {
                U8 src_blend;
                U8 dst_blend;
                U16 flags;
            };
        };
        U32 order_group;
        S32 order_index;
        internal_system* systems;
    };

    internal_system systems[MAX_SYSTEMS];
    system_bucket buckets[MAX_PGT][MAX_SYSTEMS];
    block_info* head_free_block;
    S32 free_block_count;
    U8* block_buffer;
    block_info block_info_buffer[MAX_BLOCKS];
    S32 systems_size;
    S32 buckets_size[MAX_PGT];

    block_info* alloc_block()
    {
        xASSERT_L(105, head_free_block != 0);

        block_info* b = head_free_block;
        head_free_block = head_free_block->next;
        free_block_count--;
        return b;
    }

    void free_block(block_info* b)
    {
        b->next = head_free_block;
        head_free_block = b;
        free_block_count++;
    }

    void create_system(internal_system& isys, const xParticleBatchSystem& sys)
    {
        (xParticleBatchSystem&)isys = sys;
        isys.block_list = 0;
        isys.blocks_used = 0;
        isys.units_used = 0;
        isys.units_per_block = 0x1400 / isys.stride;
        isys.best_block = 0;

        if (!(isys.rs.flags & ptank_pool::FLAG_ORDERED)) {
            isys.order_group = 0;
            isys.order_index = 0;
        }
    }

    struct compare_buckets
    {
        S32 operator()(const void* e1, const void* e2) const
        {
            const system_bucket& b1 = *(const system_bucket*)e1;
            const system_bucket& b2 = *(const system_bucket*)e2;
            return (*this)(*(const system_bucket*)e1, *(const system_bucket*)e2);
        }

        S32 operator()(const system_bucket& b1, const system_bucket& b2) const
        {
            if (b1.texture != b2.texture) {
                return b1.texture - b2.texture;
            }
            S32 ordered1 = b1.flags & ptank_pool::FLAG_ORDERED;
            S32 ordered2 = b1.flags & ptank_pool::FLAG_ORDERED;
            if (ordered1 != ordered2) {
                return ordered1 - ordered2;
            }
            if (b1.order_group != b2.order_group) {
                return b1.order_group - b2.order_group;
            }
            if (b1.order_index != b2.order_index) {
                return b1.order_index - b2.order_index;
            }
            return b1.rskey - b2.rskey;
        }
    };

    void add_to_bucket(internal_system& isys)
    {
        xASSERT_L(163, (isys.pool_type >= 0) && (isys.pool_type < MAX_PGT));

        S32& bsize = buckets_size[isys.pool_type];
        system_bucket* const begin_bucket = buckets[isys.pool_type];
        system_bucket* const end_bucket = begin_bucket + bsize;

        system_bucket key;
        key.texture = isys.rs.texture;
        key.src_blend = isys.rs.src_blend;
        key.dst_blend = isys.rs.dst_blend;
        key.flags = isys.rs.flags;
        key.order_group = isys.order_group;
        key.order_index = isys.order_index;

        system_bucket* bucket = binary_find(begin_bucket, end_bucket, key, compare_buckets());

        if (bucket == end_bucket) {
            xASSERT_L(182, bsize < MAX_SYSTEMS);
            *bucket = key;
            bucket->systems = 0;
            bsize++;
        }

        isys.next = bucket->systems;
        bucket->systems = &isys;

        if (bucket == end_bucket)
            quicksort(begin_bucket, bsize, sizeof(system_bucket), compare_buckets());
    }

    void remove_from_bucket(internal_system& isys)
    {
        S32& bsize = buckets_size[isys.pool_type];
        system_bucket* const begin_bucket = buckets[isys.pool_type];
        system_bucket* const end_bucket = begin_bucket + bsize;

        system_bucket key;
        key.texture = isys.rs.texture;
        key.src_blend = isys.rs.src_blend;
        key.dst_blend = isys.rs.dst_blend;
        key.flags = isys.rs.flags;
        key.order_group = isys.order_group;
        key.order_index = isys.order_index;

        system_bucket* bucket = binary_find(begin_bucket, end_bucket, key, compare_buckets());
        xASSERT_L(212, bucket != end_bucket);

        internal_system *it = bucket->systems, **itp = &bucket->systems;
        for (; it != &isys; itp = &it->next, it = it->next) {
            xASSERT_L(218, it != 0);
        }

        *itp = it->next;

        if (bucket->systems == 0) {
            bsize--;
            xMemoryCopyUpA32(bucket, bucket + 1, (U8*)end_bucket - (U8*)(bucket + 1));
        }
    }

    void clear_system(internal_system& system)
    {
        block_info* block = system.block_list;
        for (; block; ) {
            block_info* next_block = block->next;
            free_block(block);
            block = next_block;
        }
        system.block_list = 0;
        system.blocks_used = 0;
        system.units_used = 0;
        system.best_block = 0;
    }

    inline block_info* grab_best_block(internal_system& isys)
    {
        if (isys.best_block) {
            xASSERT_L(255, isys.best_block->used < isys.units_per_block);
            return isys.best_block;
        }

        S32 most_used = -1; block_info* b = isys.block_list;
        for (; b; b = b->next) {
            if (b->used < isys.units_per_block && most_used < b->used) {
                isys.best_block = b;
                most_used = b->used;
            }
        }
        if (isys.best_block) {
            return isys.best_block;
        }

        if (!head_free_block)
            return 0;
        isys.best_block = alloc_block();
        isys.best_block->used = 0;
        isys.best_block->next = isys.block_list;
        isys.block_list = isys.best_block;

        isys.blocks_used++;
        return isys.best_block;
    }

    void merge_blocks(internal_system& system)
    {
        xASSERT_L(290, system.blocks_used >= 2);

        block_info *first = system.block_list, *second = first->next;
        block_info *s0, *s1, **ps0;
        if (first->used < second->used) {
            s0 = first; s1 = second; ps0 = &system.block_list;
        } else {
            s0 = second; s1 = first; ps0 = &first->next;
        }

        block_info *b = second->next, **pb = &second->next;
        for (; b; pb = &b->next, b = b->next) {
            if (b->used < s0->used) {
                s1 = s0; s0 = b; ps0 = pb;
            } else if (b->used < s1->used) {
                s1 = b;
            }
        }

        S32 total = s0->used + s1->used;
        if (total <= system.units_per_block) {
            U8* tail = s1->mem + s1->used * system.stride;
            memcpy(tail, s0->mem, s0->used * system.stride);
            s1->used += s0->used;
            *ps0 = s0->next;
            free_block(s0);
            system.blocks_used--;
        }
    }

    void refresh_blocks(internal_system& system)
    {
        system.best_block = 0;

        block_info *b = system.block_list, **pb = &system.block_list;
        for (; b; b = *pb) {
            if (b->used <= 0) {
                *pb = b->next;
                free_block(b);
                system.blocks_used--;
            } else {
                pb = &b->next;
            }
        }

        if (system.blocks_used > 1)
            merge_blocks(system);
    }

    template <class T>
    void pre_render_buckets(system_bucket* first, system_bucket* last, F32 dt)
    {
        T pool;
        for (system_bucket* bucket = first; bucket != last; bucket++) {
            internal_system* system = bucket->systems;
            bool started = false;
            for (; system; system = system->next) {
                if (system->units_used <= 0) {
                    continue;
                }

                if (!started) {
                    pool.rs = system->rs;
                    pool.order_group = system->order_group;
                    pool.order_index = system->order_index;
                    pool.reset();
                    started = true;
                }

                bool lost_any = false;
                block_info* block = system->block_list;
                for (; block; block = block->next) {
                    xASSERT_L(378, block->used > 0);
                    S32 newused = system->update(block->mem, block->used, pool, dt, system->context);
                    xASSERT_L(380, newused <= block->used);
                    if (newused < block->used)
                        lost_any = true;
                    system->units_used += newused - block->used;
                    block->used = newused;
                }
                if (lost_any)
                    refresh_blocks(*system);
            }
            if (started)
                pool.flush();
        }
    }

#if DEBUG || RELEASE
    void validate_system(const internal_system& isys)
    {
        xASSERT_L(398, (isys.pool_type >= 0) && (isys.pool_type < MAX_PGT));
        xASSERT_L(399, isys.stride > 0);
        xASSERT_L(400, isys.stride <= 256);
        xASSERT_L(401, (isys.stride & 3) == 0);
        xASSERT_L(402, isys.update != 0);
        xASSERT_L(403, (isys.units_used <= 0) == (isys.blocks_used <= 0));

        S32 blocks_used = 0, units_used = 0;
        for (const block_info* b = isys.block_list; b; b = b->next) {
            xASSERT_L(409, b->used > 0);
            blocks_used++;
            units_used += b->used;
        }
        xASSERT_L(413, blocks_used == isys.blocks_used);
        xASSERT_L(414, units_used == isys.units_used);
    }
#else
    inline void validate_system(const internal_system&) {}
#endif

    inline void adjust_batch_size()
    {
    }

}

void xParticleBatchSceneEnter()
{
    adjust_batch_size();

    block_buffer = (U8*)xMEMALLOC_L(MAX_BLOCKS * 0x1400, 0, eMemMgrTag_Particle, 0, 458);
    xASSERT_L(459, block_buffer != 0);

    systems_size = 0;

    memset(buckets_size, 0, sizeof(buckets_size));

    xParticleBatchReset();

    tweak::add_tweaks();
}

void xParticleBatchSceneExit()
{
}

void xParticleBatchReset()
{
    internal_system *sys = systems, *end_sys = sys + systems_size;
    for (; sys != end_sys; sys++) {
        sys->block_list = 0;
        sys->blocks_used = 0;
        sys->units_used = 0;
        sys->best_block = 0;
    }

    head_free_block = block_info_buffer;
    free_block_count = MAX_BLOCKS;
    block_info *b = block_info_buffer, *endb = b + MAX_BLOCKS - 1;
    U8* mem = block_buffer;
    for (; b != endb; b++, mem += 0x1400) {
        b->mem = mem;
        b->next = b + 1;
    }
    endb->mem = mem;
    endb->next = 0;
}

void xParticleBatchPreRender()
{
    F32 dt = xglobals->update_dt;

    pre_render_buckets<ptank_pool__color_mat>(buckets[PGT_COLOR_MAT], buckets[PGT_COLOR_MAT] + buckets_size[PGT_COLOR_MAT], dt);
    pre_render_buckets<ptank_pool__color_mat_uv2>(buckets[PGT_COLOR_MAT_UV2], buckets[PGT_COLOR_MAT_UV2] + buckets_size[PGT_COLOR_MAT_UV2], dt);
    pre_render_buckets<ptank_pool__pos_color_size>(buckets[PGT_POS_COLOR_SIZE], buckets[PGT_POS_COLOR_SIZE] + buckets_size[PGT_POS_COLOR_SIZE], dt);
    pre_render_buckets<ptank_pool__pos_color_size_uv2>(buckets[PGT_POS_COLOR_SIZE_UV2], buckets[PGT_POS_COLOR_SIZE_UV2] + buckets_size[PGT_POS_COLOR_SIZE_UV2], dt);
    pre_render_buckets<ptank_pool__pos_color_size_rot>(buckets[PGT_POS_COLOR_SIZE_ROT], buckets[PGT_POS_COLOR_SIZE_ROT] + buckets_size[PGT_POS_COLOR_SIZE_ROT], dt);
    pre_render_buckets<ptank_pool__pos_color_size_rot_uv2>(buckets[PGT_POS_COLOR_SIZE_ROT_UV2], buckets[PGT_POS_COLOR_SIZE_ROT_UV2] + buckets_size[PGT_POS_COLOR_SIZE_ROT_UV2], dt);
}

#if DEBUG || RELEASE
S32 xParticleBatchAddSystem(const xParticleBatchSystem& system, const char* dbgFile, S32 dbgLine)
#else
S32 xParticleBatchAddSystem(const xParticleBatchSystem& system)
#endif
{
    xASSERT_L(525, systems_size < MAX_SYSTEMS);

    S32 system_id = systems_size;
    systems_size++;
    internal_system& isys = systems[system_id];
    create_system(isys, system);
#if DEBUG || RELEASE
    isys.dbgFile = dbgFile;
    isys.dbgLine = dbgLine;
#endif
    add_to_bucket(isys);
    validate_system(isys);
    return system_id;
}

void xParticleBatchReplaceSystem(S32 system_id, const xParticleBatchSystem& system)
{
    xASSERT_L(547, (system_id >= 0) && (system_id < systems_size));

    internal_system& isys = systems[system_id];
    if (system.stride != isys.stride) {
        clear_system(isys);
        isys.units_per_block = 0x1400 / isys.stride;
    }

    remove_from_bucket(isys);
    (xParticleBatchSystem&)isys = system;
    add_to_bucket(isys);
    validate_system(isys);
}

xTWEAKABLE F32 SCALE_FREE_PERCENT = 0.3f;
xTWEAKABLE F32 SCALE_SYSTEM_FREE_PERCENT = 0.75f;

F32 xParticleBatchRequest(S32 system_id, F32 amount, S32 max_blocks)
{
    F32 throttledAmount = amount;
    xASSERT_L(580, (system_id >= 0) && (system_id < systems_size));
    internal_system& s = systems[system_id];

    F32 newBlockUsage = (s.units_used + amount) / s.units_per_block;
    if (max_blocks != 0) {
        F32 systemFreePercent = 1.0f - newBlockUsage / max_blocks;
        if (systemFreePercent < SCALE_SYSTEM_FREE_PERCENT) {
            if (systemFreePercent < 0.0f)
                throttledAmount = s.units_per_block * max_blocks - (F32)s.units_used;
            else
                throttledAmount = amount * systemFreePercent / SCALE_SYSTEM_FREE_PERCENT;
            return throttledAmount;
        }
    }

    if (amount < 1.0f || newBlockUsage <= 0.75f) {
        return throttledAmount;
    }

    F32 free_percent = free_block_count / (F32)MAX_BLOCKS;
    if (free_block_count == 0 || free_percent >= SCALE_FREE_PERCENT) {
        return throttledAmount;
    }

    F32 newAmount = amount * free_percent / SCALE_FREE_PERCENT;
    return MIN(newAmount, throttledAmount);
}

S32 xParticleBatchEmit(S32 system_id, S32 amount, U8** mem)
{
    xASSERT_L(625, (system_id >= 0) && (system_id < systems_size));
    xASSERT_L(634, amount > 0);

    internal_system& isys = systems[system_id];
    block_info* block = grab_best_block(isys);
    if (!block) {
        return 0;
    }

    S32 remain = isys.units_per_block - block->used;
    *mem = block->mem + block->used * isys.stride;
    if (amount >= remain) {
        isys.best_block = 0; amount = remain;
    }

    block->used += amount;
    isys.units_used += amount;
    return amount;
}

void xParticleBatchGroup::create()
{
    elements = new (eMemStaticType_0, eMemMgrTag_Particle, 0) U8[MAX_SYSTEMS_PER_GROUP];
    elements_size = 0;
}

namespace {

    inline S32 compare_systems(const xParticleBatchSystem& sys1, const xParticleBatchSystem& sys2)
    {
        if (sys1.pool_type != sys2.pool_type)
            return sys1.pool_type - sys2.pool_type;
        if (sys1.rs.texture != sys2.rs.texture)
            return (S32)sys1.rs.texture - (S32)sys2.rs.texture;
        if (sys1.rs.src_blend != sys2.rs.src_blend)
            return sys1.rs.src_blend - sys2.rs.src_blend;
        if (sys1.rs.dst_blend != sys2.rs.dst_blend)
            return sys1.rs.dst_blend - sys2.rs.dst_blend;
        if (sys1.rs.flags != sys2.rs.flags)
            return sys1.rs.flags - sys2.rs.flags;
        if (sys1.rs.flags & ptank_pool::FLAG_ORDERED) {
            if (sys1.order_group != sys2.order_group)
                return sys1.order_group - sys2.order_group;
            if (sys1.order_index != sys2.order_index)
                return sys1.order_index - sys2.order_index;
        }
        if (sys1.stride != sys2.stride)
            return sys1.stride - sys2.stride;
        if (sys1.update != sys2.update)
            return (S32)sys1.update - (S32)sys2.update;
        return (S32)sys1.context - (S32)sys2.context;
    }

    struct compare_group_systems_functor
    {
        S32 operator()(const xParticleBatchSystem& system, const U8& system_id) const
        {
            xASSERT_L(732, system_id < systems_size);
            const xParticleBatchSystem& isys = systems[system_id];
            return compare_systems(system, isys);
        }
    };

    S32 compare_group_systems_function(const void* e1, const void* e2)
    {
        S32 sysid1 = *(const U8*)e1, sysid2 = *(const U8*)e2;
        xASSERT_L(741, (sysid1 < systems_size) && (sysid2 < systems_size));
        const xParticleBatchSystem& isys1 = systems[sysid1];
        const xParticleBatchSystem& isys2 = systems[sysid2];
        return compare_systems(systems[sysid1], systems[sysid2]);
    }

}

#if DEBUG || RELEASE
S32 xParticleBatchGroup::create_system(const xParticleBatchSystem& system, const char* dbgFile, S32 dbgLine)
#else
S32 xParticleBatchGroup::create_system(const xParticleBatchSystem& system)
#endif
{
    const U8* end_elements = elements + elements_size;
    const U8* found = binary_find((const U8*)elements, end_elements, system, compare_group_systems_functor());

    if (found != end_elements) {
        return *found;
    }

    xASSERT_L(757, elements_size < MAX_SYSTEMS_PER_GROUP);

#if DEBUG || RELEASE
    S32 system_id = xParticleBatchAddSystem(system, dbgFile, dbgLine);
#else
    S32 system_id = xParticleBatchAddSystem(system);
#endif
    elements[elements_size] = system_id;
    elements_size++;

    qsort(elements, elements_size, 1, compare_group_systems_function);

    return system_id;
}

#if DEBUG || RELEASE
namespace {

    struct reload_context
    {
        xParticleBatchGroup* group;
        S32* system_id;
        bool privatized;
        U32 texture;
        U32 blend_src;
        U32 blend_dst;
        S32 flags;

        reload_context(xParticleBatchGroup* group, S32* system_id, bool privatized) : group(group), system_id(system_id), privatized(privatized) {}
    };

}

void xParticleBatchGroup::add_tweaks(S32* system_id, const char* prefix)
{
    xASSERT_L(792, system_id != 0);

#if DEBUG
    S32 stack_pointer[4];
    xASSERT_L(796, ((system_id < stack_pointer) || (system_id > (stack_pointer + 4096))) &&"system_id reference on stack?");
#endif

    xASSERT_L(799, (*system_id >= 0) && (*system_id < systems_size));
    xParticleBatchSystem& isys = systems[*system_id];

    const char** texture_labels;
    const U32* texture_ids;
    U32 textures = zSceneGetTextureTweakMap(texture_labels, texture_ids);

    reload_context* context = new (eMemTweakType_0, eMemMgrTag_Particle) reload_context(this, system_id, false);
    context->texture = isys.rs.texture == 0 ? 0 : zSceneGetTextureIDFromRaster(RwTextureGetRaster(isys.rs.texture));
    context->blend_src = isys.rs.src_blend;
    context->blend_dst = isys.rs.dst_blend;
    context->flags = isys.rs.flags;

    static const tweak_callback cb_reload = { on_reload };
    xAUTOTWEAK_SELECT(prefix, "context-> texture", &context->texture, texture_labels, texture_ids, textures, &cb_reload, context, 0, true);
    xAUTOTWEAK_SELECT(prefix, "context-> blend_src", &context->blend_src, tweak_blend_mode_labels, tweak_blend_mode_values, (U32)ARRAY_LENGTH(tweak_blend_mode_labels), &cb_reload, context, 0, true);
    xAUTOTWEAK_SELECT(prefix, "context-> blend_dst", &context->blend_dst, tweak_blend_mode_labels, tweak_blend_mode_values, (U32)ARRAY_LENGTH(tweak_blend_mode_labels), &cb_reload, context, 0, true);
    xAUTOTWEAK_FLAG(prefix, "Disable Z-Test", &context->flags, ptank_pool::FLAG_NOZTEST, &cb_reload, context, 0);
    xAUTOTWEAK_FLAG(prefix, "Bright", &context->flags, ptank_pool::FLAG_BRIGHT, &cb_reload, context, 0);
}

void xParticleBatchGroup::on_reload(const tweak_info& info)
{
    reload_context& context = *(reload_context*)info.context;

    S32* system_id = context.system_id;
    xASSERT_L(825, system_id != 0);
    xASSERT_L(826, (*system_id >= 0) && (*system_id < systems_size));
    const xParticleBatchSystem& isys = systems[*system_id];

    xParticleBatchSystem system = isys;
    system.rs.texture = context.texture == 0 ? 0 : (RwTexture*)xSTFindAsset(context.texture, 0);
    system.rs.src_blend = context.blend_src;
    system.rs.dst_blend = context.blend_dst;
    system.rs.flags &= ~(ptank_pool::FLAG_NOZTEST | ptank_pool::FLAG_BRIGHT);
    system.rs.flags |= context.flags;

    if (memcmp(&system, &isys, sizeof(system)) == 0) {
        return;
    }

    if (context.privatized) {
        xParticleBatchReplaceSystem(*system_id, system);
    } else {
        context.privatized = true;
        *system_id = xPARTICLEBATCHADDSYSTEM_L(system, 846);
    }
}

void xParticleBatchDebugCB()
{
    S32 numFreeBlocks = 0;
    for (block_info* block = head_free_block; block; block = block->next) {
        numFreeBlocks++;
    }
    xASSERT_L(858, numFreeBlocks == free_block_count);
    xprintf("\nParticle Blocks free: %d\n", numFreeBlocks);

    F32 free_percent = free_block_count / (F32)MAX_BLOCKS;
    if (free_block_count == 0) {
        xprintf("NO FREE BLOCKS\n");
    } else {
        if (free_percent < SCALE_FREE_PERCENT) {
            xprintf("Reducing new particle requests to %f\n", free_percent / SCALE_FREE_PERCENT);
        }
    }

    xprintf("Blocks Units Name\n");
    xprintf("------ ----- ----------\n");
    for (S32 i = 0; i < systems_size; i++) {
        internal_system& s = systems[i];
        if (s.units_used != 0) {
            xprintf("%6.2f %5d %s:%d\n", (F32)s.units_used / s.units_per_block, s.units_used, s.dbgFile, s.dbgLine);
        }
    }
}
#endif
