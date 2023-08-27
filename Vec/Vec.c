#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "Vec.h"

typedef struct Vec Vec;
struct Vec
{
    uint8_t* data; // Vector data (`uint8_t` for bytewise pointer arithmetic)
    size_t element_size; // Size of the vector's element type in bytes
    size_t capacity; // Number of elements the vector can store before resizing
    size_t capacity_bytes; // Capacity in bytes
    size_t count; // Current number of elements stored in the vector
    size_t count_bytes; // Current element count in bytes
};

/**
 * @brief Rounds the given number of bytes up to the next (probably)
 * word-aligned size
 *
 * This is meant mainly to calculate a "big enough and then some" size based on
 * the memory size requested that minimizes the vector's future reallocations.
 * For that, any scheme of allocating-a-little-more-than-you-actually-need would
 * do, but word-alignment MAY also boost performance by MAYBE reducing the
 * number of reads the processor has to do when working with the vector's data
 * block. The hope is (I think), if the vector's data block is a multiple of the
 * processor's word size (e.g., the data block is EXACTLY 4 words long or
 * EXACTLY 9 words long -- EXACTLY some number of words long, not a little more
 * or a little less), it's more likely to be aligned at a word boundary in
 * memory. That means the CPU will be able to read a word of the data block in
 * one (word-sized) read. Were the data block not aligned on a word boundary
 * (picture a word of the block half in one word segment in RAM, half in the
 * next word segment), the CPU would need to do 2 reads to get both halves of
 * the data block's word then and  reassemble them. Legends say some (RISC?)
 * CPUs even throw an  exception when reading non-aligned memory!
 *
 * This fails, and returns 0 (or, if assertions are enabled, causes an assert
 * crash), if the given number is so huge that calculating the next-biggest
 * word-aligned size overflows.
 *
 * @param requested_bytes A number of bytes (e.g., from a new capacity request)
 * @return A word-aligned number greater-than-or-equal to the given number
 */
static size_t word_align_size(size_t const requested_bytes)
{
    /*
     * To word-align the number of bytes, it'll be sized as a multiple of
     * `sizeof(void*) * 8`.
     *
     * Rationale: it's (dangerously) assumed that `sizeof(void*)` is equal to
     * the processor's word size (which is not always true). It's also assumed
     * the word size is a multiple of 8, hence `* 8`.
     *
     * On a 64-bit CPU with 8 byte words, aligned sizes will be multiples of
     * `8 * 8 = 64` bytes, or 8 words, of memory.
     *
     * On a 32-bit CPU with 4 byte words, they will be multiples of `4 * 8 = 32`
     * bytes, again 8 words, of memory.
     */
    size_t const alignment = sizeof(void*) * 8;
    size_t aligned_bytes = 1;

    if (requested_bytes % alignment == 0)
    {
        // If the requested size already appears word-aligned, use it as-is.
        aligned_bytes = requested_bytes;
    }
    else
    {
        /*
         * Word-align the block size.
         *
         * E.g., if the requested size is 65, the next-largest aligned size is
         * 128.
         */
        size_t const next_aligned_exponent = (size_t) log2(requested_bytes) + 1;

        /*
         * Exponentiate in a loop so overflow can be caught without having to
         * use `errno` (because using `errno` is unpleasant).
         */
        for (size_t i = 0; i < next_aligned_exponent; ++i)
        {
            // (If okay with requiring C23, this should just be a `ckd_mul()`.)
            bool const align_overflow = aligned_bytes > (SIZE_MAX / 2);

            assert(!align_overflow);
            if (align_overflow)
            {
                return 0;
            }

            aligned_bytes *= 2;
        }

        if (aligned_bytes < alignment)
        {
            /*
             * If the requested size was so small that the next-largest aligned
             * size is under the alignment factor, just use the alignment
             * factor.
             */
            aligned_bytes = alignment;
        }
    }

    return aligned_bytes;
}

Vec* Vec_new(size_t const least_capacity,
             size_t const element_size)
{
    assert(least_capacity != 0);
    assert(element_size != 0);
    if (least_capacity == 0 ||
        element_size == 0)
    {
        return NULL;
    }

    /*
     * Fail if total number of bytes requested overflows.
     *
     * (If okay with requiring C23, this should just be a `ckd_mul()`.)
     */
    bool const total_byte_overflow = least_capacity > (SIZE_MAX / element_size);

    assert(!total_byte_overflow);
    if (total_byte_overflow)
    {
        return NULL;
    }

    // Allocate the vector.
    Vec* v = malloc(sizeof(Vec));
    bool const vector_allocation_succeeded = (v != NULL);

    assert(vector_allocation_succeeded);
    if (!vector_allocation_succeeded)
    {
        return NULL;
    }

    // Initialize vector members
    v->data = NULL;
    v->element_size = element_size;
    v->count = 0;
    v->count_bytes = 0;

    /*
     * We want an initial capacity that's at least as large as the requested
     * capacity but whose size in bytes is (hopefully) word-aligned.
     */
    size_t const aligned_bytes = word_align_size(least_capacity * element_size);

    if (aligned_bytes == 0)
    {
        // Abort if alignment failed.
        free(v);
        return NULL;
    }

    /*
     * The actual element capacity will be the number of elements that fit in
     * the word-aligned number of bytes.
     *
     * Any spare bytes left over will be unused.
     *
     * E.g., if the caller passed in a minimum capacity of 5 and an element size
     * of 13, that's a requested size of `5 * 13 = 65` bytes. We word-align that
     * to 128 bytes. The actual capacity is `128 / 13 = 9.8 ~= 9` elements, with
     * `128 - (9 * 13) = 11` bytes left over, which will be unused.
     */
    v->capacity = aligned_bytes / element_size;
    v->capacity_bytes = aligned_bytes;

    // Allocate the vector's element block.
    v->data = malloc(v->capacity_bytes);
    bool const element_allocation_succeeded = (v->data != NULL);

    assert(element_allocation_succeeded);
    if (!element_allocation_succeeded)
    {
        free(v);
        return NULL;
    }

    return v;
}

void Vec_destroy(Vec** v)
{
    if (v == NULL ||
        (*v) == NULL)
    {
        return;
    }

    free((*v)->data);
    free(*v);
    *v = NULL;
}

size_t Vec_capacity(Vec const* v)
{
    assert(v != NULL);
    if (v == NULL)
    {
        return 0;
    }

    return v->capacity;
}

size_t Vec_count(Vec const* v)
{
    assert(v != NULL);
    if (v == NULL)
    {
        return 0;
    }

    return v->count;
}

size_t Vec_count_bytes(Vec const* v)
{
    assert(v != NULL);
    if (v == NULL)
    {
        return 0;
    }

    return v->count_bytes;
}

size_t Vec_element_size(Vec const* v)
{
    assert(v != NULL);
    if (v == NULL)
    {
        return 0;
    }

    return v->element_size;
}

bool Vec_equal(Vec const* v_a,
               Vec const* v_b,
               int (*cmp)(void const*, void const*))
{
    assert(v_a != NULL);
    assert(v_b != NULL);
    assert(v_a->data != NULL);
    assert(v_b->data != NULL);
    if (v_a == NULL ||
        v_b == NULL ||
        v_a->data == NULL ||
        v_b->data == NULL)
    {
        return false;
    }

    /*
     * Note that a tempting optimization here is to compare the vector pointers
     * to each other to see if they simply point to the same vector since, in
     * that case, we don't need to bother comparing their elements -- we know
     * they're all the same!
     *
     * There seems to be disagreement about whether comparing pointers to
     * different, unrelated, and non-adjacent objects is always well-defined or
     * consistent in GCC:
     *     - https://stackoverflow.com/q/8225501
     *     - https://stackoverflow.com/q/36035782
     *     - https://stackoverflow.com/q/45966762
     *
     * Also, glibc's and musl's `memcmp()` implementations don't seem to take
     * advantage of this optimization the last time I looked, although neither
     * had comments explaining why.
     *
     * To err on the side of safety/portability, the pointers are assumed to be
     * pointing to different vectors, so we do not compare the pointers to each
     * other.
     */

    // Do the vectors differ in metadata?
    if (v_a->count != v_b->count ||
        v_a->element_size != v_b->element_size)
    {
        return false;
    }

    // Two empty vectors that expect the same element size are considered equal.
    if (v_a->count == 0 &&
        v_b->count == 0 &&
        v_a->element_size == v_b->element_size)
    {
        return true;
    }

    /*
     * Compare the vectors' contents.
     *
     * Note that vectors' empty regions should NOT be scanned -- not just for
     * efficiency but also for correctness. Since a vector's empty bytes are
     * unmanaged, they can be random garbage. That means two vectors with
     * identical elements could report a false difference because their empty
     * bytes differ.
     */
    if (cmp != NULL)
    {
        /*
         * If an element comparator function was given, use it to compare the
         * vectors' elements one by one.
         */
        for (size_t i = 0; i < v_a->count_bytes; i += v_a->element_size)
        {
            if (0 != cmp(&(v_a->data[i]), &(v_b->data[i])))
            {
                return false;
            }
        }
    }
    else
    {
        // With no element comparator, just compare all element bytes.
        if (memcmp(v_a->data, v_b->data, v_a->count_bytes) != 0)
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Converts the given element index between an "internal" byte-wise form
 * and an "external" element-wise form
 *
 * An "internal" index corresponds to the internal byte array of this vector
 * implementation. An "external" index is a conceptual, more user-friendly index
 * corresponding to where a caller, from the outside, might expect elements to
 * exist in a vector.
 *
 * For example, consider an vector that holds 3 elements, where the elements'
 * type takes 2 bytes to store, and the vector's capacity is 4 elements. From
 * the outside, the vector conceptually looks like this:
 *
 * ```
 * [a][b][c][ ]
 *  0  1  2  3
 * ```
 *
 * A caller expects element `c` to be at index 2. This is the "external" index
 * of `c`.
 *
 * Internally, however, the implementation has a byte array that really looks
 * like this:
 *
 * ```
 * [a][a][b][b][c][c][ ][ ]
 *  0     1     2     3     External indices
 *  0  1  2  3  4  5  6  7  Internal indices
 * ```
 *
 * In the byte array, element `c` actually starts at index 4. This is the
 * "internal" index of `c`.
 *
 * Each element (`a`, `b`, and `c`) takes 2 bytes, hence indices 0 and 1 both
 * containing `a`, 2 and 3 both containing `b`, and so on.
 *
 * To convert the internal index (4) to the external index (2), the internal
 * index is divided by the element size (2). To convert the other way, an
 * external index is multiplied by the element size.
 *
 * This fails, and returns 0 (or, if assertions are enabled, causes an assert
 * crash), if the vector pointer is null.
 *
 * @param v The vector whose index is being converted
 * @param index The index to convert
 * @param to_external Whether to convert the given (internal) index to an
 * external index (otherwise, it's assumed the given index is an external index
 * and is converted to an internal index)
 * @return The converted index
 */
static size_t convert_index(Vec const* v,
                            size_t const index,
                            bool const to_external)
{
    assert(v != NULL);
    if (v == NULL)
    {
        return 0;
    }

    if (to_external)
    {
        return index / v->element_size;
    }
    else
    {
        /*
         * We should assume there's no overflow since we verified that the index
         * is within bounds of the vector's elements and the other vector
         * functions already safeguard against this overflow.
         */
        return index * v->element_size;
    }
}

/**
 * @brief A convenience wrapper around the index conversion function
 *
 * This fails, and returns 0 (or, if assertions are enabled, causes an assert
 * crash), if the vector pointer is null.
 *
 * @param v The vector whose index is being converted
 * @param index The index to convert
 * @return The converted internal index
 */
static size_t to_internal_index(Vec const* v,
                                size_t const index)
{
    return convert_index(v, index, false);
}

/**
 * @brief A convenience wrapper around the index conversion function
 *
 * This fails, and returns 0 (or, if assertions are enabled, causes an assert
 * crash), if the vector pointer is null.
 *
 * @param v The vector whose index is being converted
 * @param index The index to convert
 * @return The converted external index
 */
static size_t to_external_index(Vec const* v,
                                size_t const index)
{
    return convert_index(v, index, true);
}

/**
 * @var
 * The item pointer passed in to functions which use the built-in `memcmp()`
 * predicate
 *
 * This pointer is set aside as a global variable so that the `memcmp()`
 * predicate can compare elements to the item pointer without requiring an extra
 * parameter (allowing it to have the same signature as normal, caller-provided
 * predicates).
 */
static void const* item_for_memcmp_predicate = NULL;

/**
 * @brief A built-in predicate that compares, bytewise via `memcmp()`, the given
 * element to a global item
 *
 * Some functions take an item pointer and check whether an element in the
 * vector matches that item. There can also be "sibling" functions that, for
 * added flexibility, take a predicate and check whether an element in the
 * vector satisfies that predicate.
 *
 * "Does this element match this item" is an example of such a predicate. The
 * functions that take an item pointer can just reuse the code of the "sibling"
 * functions that take a predicate by calling them with that predicate.
 *
 * This function is that predicate.
 *
 * However, because the expected predicates only take one element argument,
 * there's nowhere for the "item" in "Does this element match this item" to be
 * passed in. As a workaround, the functions that take an item pointer store the
 * item pointer in a global variable, which this function accesses to compare
 * its element argument to.
 *
 * This fails, and returns false (or, if assertions are enabled, causes an
 * assert crash), if either the element pointer or global item pointer are null
 * or the element size is 0.
 *
 * @param element The element to compare to a global item
 * @return Whether the given element's bytes match a global item's
 */
static bool memcmp_predicate(void const* element, size_t const element_size)
{
    assert(element != NULL);
    assert(item_for_memcmp_predicate != NULL);
    assert(element_size > 0);
    if (element == NULL ||
        item_for_memcmp_predicate == NULL ||
        element_size == 0)
    {
        return false;
    }

    /*
     * Note that a tempting optimization here is to compare the global item
     * pointer to the element pointer, to see if both point to the same memory
     * address in the vector.
     *
     * If both pointers are pointing to the same memory, we don't have to bother
     * comparing the bytes. If elements are large, that's a lot of work saved!
     *
     * But, given the uncertainty surrounding the safety/portability of pointer
     * comparison, this optimization is NOT done. Item pointers are always
     * assumed to be pointing to memory outside the vector.
     */
    if (memcmp(item_for_memcmp_predicate, element, element_size) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

size_t Vec_where_if(Vec const* v,
                    bool (*predicate)(void const*, size_t const))
{
    assert(v != NULL);
    assert(v->data != NULL);
    if (v == NULL ||
        v->data == NULL)
    {
        return 0;
    }

    assert(predicate != NULL);
    if (predicate == NULL ||
        v->count == 0)
    {
        return v->count;
    }

    for (size_t i = 0; i < v->count_bytes; i += v->element_size)
    {
        if (predicate(&(v->data[i]), v->element_size))
        {
            return to_external_index(v, i);
        }
    }

    return v->count;
}

size_t Vec_where(Vec const* v,
                 void const* item,
                 size_t const item_size)
{
    assert(v != NULL);
    assert(v->data != NULL);
    if (v == NULL ||
        v->data == NULL)
    {
        return 0;
    }

    assert(item != NULL);
    assert(item_size == v->element_size);
    if (item == NULL ||
        item_size != v->element_size ||
        v->count == 0)
    {
        return v->count;
    }

    /*
     * Compare the given item's bytes with those of each element.
     *
     * First, store the given item pointer globally, allowing us to re-use the
     * predicate form of this function with the `memcmp()` predicate.
     */
    item_for_memcmp_predicate = item;

    return Vec_where_if(v, memcmp_predicate);
}

bool Vec_has(Vec const* v,
             void const* item,
             size_t const item_size)
{
    assert(v != NULL);
    assert(v->data != NULL);
    assert(item != NULL);
    assert(item_size == v->element_size);
    if (v == NULL ||
        v->data == NULL ||
        item == NULL ||
        item_size != v->element_size ||
        v->count == 0)
    {
        return false;
    }

    if (v->count != Vec_where(v, item, item_size))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Vec_has_if(Vec const* v,
                bool (*predicate)(void const*, size_t const))
{
    assert(v != NULL);
    assert(v->data != NULL);
    assert(predicate != NULL);
    if (v == NULL ||
        v->data == NULL ||
        predicate == NULL ||
        v->count == 0)
    {
        return false;
    }

    if (v->count != Vec_where_if(v, predicate))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void* Vec_get(Vec const* v, size_t const external_index)
{
    assert(v != NULL);
    assert(v->data != NULL);
    if (v == NULL ||
        v->data == NULL)
    {
        return NULL;
    }

    bool const index_out_of_bounds = external_index >= v->count;

    assert(!index_out_of_bounds);
    if (index_out_of_bounds)
    {
        return NULL;
    }

    if (external_index >= v->count)
    {
        // No element exists at the index.
        return NULL;
    }

    return v->data + to_internal_index(v, external_index);
}

/**
 * @brief Resizes the given vector to exactly the given capacity
 *
 * WARNING: This may invalidate stored pointers or indices! After a vector's
 * data block is resized, the data block may reside in a totally different
 * region of memory than the one still being pointed to by outside pointers!
 * Also, if the vector was downsized and elements were truncated, pointers or
 * indices that used to correspond to elements in the vector may now be outside
 * the vector's bounds!
 *
 * This fails, and doesn't modify the vector (or, if assertions are enabled,
 * causes an assert crash), if any of the following are true:
 *     - The new capacity is zero or less than the vector's current capacity
 *     - The total byte size (type size * capacity) of the vector will be too
 *       huge to be numerically represented in the vector without overflow
 *     - Reallocating the memory (`realloc()`) failed, internally
 *     - The given vector pointer is null
 *
 * @param v The vector to resize
 * @param new_capacity The new capacity
 * @return Whether the vector was successfully resized
 */
static bool Vec_resize(Vec* v, size_t const new_capacity)
{
    assert(v != NULL);
    assert(v->data != NULL);
    assert(new_capacity != 0);
    assert(new_capacity > v->capacity);
    if (v == NULL ||
        v->data == NULL ||
        new_capacity == 0 ||
        new_capacity <= v->capacity)
    {
        return false;
    }

    // (If okay with requiring C23, this should just be a `ckd_mul()`.)
    size_t const max_elements = SIZE_MAX / v->element_size;
    bool const total_byte_size_overflowed = new_capacity > max_elements;

    assert(!total_byte_size_overflowed);
    if (total_byte_size_overflowed)
    {
        return false;
    }

    size_t const new_capacity_bytes = v->element_size * new_capacity;
    uint8_t* success = realloc(v->data, sizeof(uint8_t*[new_capacity_bytes]));
    bool const vector_realloc_succeeded = (success != NULL);

    assert(vector_realloc_succeeded);
    if (!vector_realloc_succeeded)
    {
        return false;
    }

    v->data = success;
    v->capacity = new_capacity;
    v->capacity_bytes = new_capacity_bytes;

    return true;
}

/**
 * @brief Handles expansion of the given vector when it's run out of room for
 * new elements
 *
 * WARNING: This may invalidate stored pointers or indices! After a vector's
 * data block is resized, the data block may reside in a totally different
 * region of memory than the one still being pointed to by outside pointers!
 * Also, if the vector was downsized and elements were truncated, pointers or
 * indices that used to correspond to elements in the vector may now be outside
 * the vector's bounds!
 *
 * This fails, and doesn't modify the vector (or, if assertions are enabled,
 * causes an assert crash), if any of the following are true:
 *     - The vector's not full (i.e., its capacity is not actually exhausted)
 *     - When expanded, the vector's capacity would become so huge that it can
 *       no longer be represented by the vector's internal data without overflow
 *     - The vector pointer is null
 *
 * @param v The vector to expand
 * @return Whether the vector was successfully expanded
 */
static bool handle_capacity_exhaustion(Vec* v)
{
    assert(v != NULL);
    assert(v->count == v->capacity);
    if (v == NULL ||
        v->count != v->capacity)
    {
        return false;
    }

    size_t const expansion_factor = 2;

    /*
     * Do nothing if the total bytes overflow when multiplied by some factor.
     *
     * (If okay with requiring C23, this should just be a `ckd_mul()` on the new
     * capacity.)
     */
    bool const expanded_capacity_overflowed =
        (v->capacity > 0 &&
        (v->capacity > (SIZE_MAX / expansion_factor)));

    assert(!expanded_capacity_overflowed);
    if (expanded_capacity_overflowed)
    {
        return false;
    }

    size_t const expanded_capacity = v->capacity_bytes * expansion_factor;
    size_t const aligned_bytes = word_align_size(expanded_capacity);

    // Similarly, do nothing if alignment fails.
    if (aligned_bytes == 0)
    {
        return false;
    }

    return Vec_resize(v, aligned_bytes);
}

bool Vec_append(Vec* v, void const* item, size_t const item_size)
{
    assert(v != NULL);
    assert(v->data != NULL);
    assert(item != NULL);
    assert(item_size == v->element_size);
    if (v == NULL ||
        v->data == NULL ||
        item == NULL ||
        item_size != v->element_size)
    {
        return false;
    }

    /*
     * Do nothing if size will overflow after addition.
     *
     * (If okay with requiring C23, this should just be a `ckd_add()` on the
     * new size.)
     */
    bool const element_count_overflowed = ((v->count + 1) < v->count);

    assert(!element_count_overflowed);
    if (element_count_overflowed)
    {
        return false;
    }

    // If at capacity, try to expand the vector first.
    if (v->count == v->capacity)
    {
        if (!handle_capacity_exhaustion(v))
        {
            // Do nothing if expansion failed.
            return false;
        }

        // We're assured we expanded.
        assert(v->count < v->capacity);

        // Check that we expanded such that we have room for one more element.
        assert(v->capacity_bytes - v->count_bytes >= v->element_size);
    }

    // Append the item.
    memcpy((v->data + (v->count * v->element_size)),
           item,
           item_size);
    v->count += 1;
    v->count_bytes += v->element_size;

    return true;
}

bool Vec_insert(Vec* v,
                size_t const insertion_index_e,
                void const* item,
                size_t const item_size)
{
    assert(v != NULL);
    assert(v->data != NULL);
    assert(item != NULL);
    if (v == NULL ||
        v->data == NULL ||
        item == NULL)
    {
        return false;
    }

    /*
     * Do nothing if size will overflow after addition.
     *
     * (If okay with requiring C23, this should just be a `ckd_add()` on the
     * new size.)
     */
    bool const element_count_overflowed = ((v->count + 1) < v->count);
    bool const index_out_of_bounds = (insertion_index_e > v->count);

    assert(!element_count_overflowed);
    assert(!index_out_of_bounds);
    assert(item_size == v->element_size);
    if (element_count_overflowed ||
        index_out_of_bounds ||
        item_size != v->element_size)
    {
        return false;
    }

    /*
     * If we have to resize the vector or to shift elements to make room for the
     * insertion, we must consider whether the given item pointer is pointing to
     * an element already inside the vector. If we don't, we'll run into a
     * problem if the element behind the pointer shifts out from under the
     * pointer or if, when the vector resizes, its data gets moved to a
     * different block of memory. In the latter case, the data behind the
     * pointer might be different, causing that different data to be inserted
     * instead of the element originally behind the item pointer.
     *
     * To avoid all this, we'll reassign the item pointer to the first identical
     * byte-for-byte instance of the element found in the vector. That instance
     * might be the original element or a byte-for-byte copy of it. Either way,
     * this will allow us to track the element data we want to insert if it
     * moves before insertion.
     *
     * (If, however, the item pointer is pointing to an element outside the
     * vector, and that element just happens to have a byte-identical copy
     * inside the vector, this is just benign overhead. It's still necessary,
     * however, since there's no way to know if the item pointer is pointing
     * inside the vector -- at least not without pointer address comparison,
     * which is avoided for safety/portability and totally not superstition
     * surrounding rumored GCC bugs in that StackOverflow article linked
     * elsewhere in this file.)
     */
    size_t reassigned_target_e = 0; // External index to reassigned target
    bool const should_reassign_target = Vec_has(v, item, v->element_size);

    if (should_reassign_target)
    {
        /*
         * Reassign to the first byte-for-byte identical instance of the element
         * found in the vector (whether that's the actual element already behind
         * the item pointer or a copy).
         */
        reassigned_target_e = Vec_where(v, item, v->element_size);
    }

    // If at capacity, try to expand the vector first.
    if (v->count == v->capacity)
    {
        if (!handle_capacity_exhaustion(v))
        {
            // Do nothing if expansion failed.
            return false;
        }
    }

    /*
     * Shift the elements at and after the insertion site to the right to make
     * room for insertion.
     */
    size_t const insertion_index_i = to_internal_index(v, insertion_index_e);
    size_t elements_to_shift = (v->count - insertion_index_e);

    if (elements_to_shift >= 1)
    {
        if (should_reassign_target)
        {
            // If the reassigned target will be shifted, account for that.
            if (insertion_index_e <= reassigned_target_e)
            {
                reassigned_target_e += 1;
            }
        }

        // Shift element(s) to the right by one element's size in bytes.
        memmove(v->data + insertion_index_i + v->element_size,
                v->data + insertion_index_i,
                elements_to_shift * v->element_size);
    }

    // Insert the (potentially reassigned) item.
    void const* insertion_target = NULL;

    if (should_reassign_target)
    {
        /*
         * If we reassigned the insertion target to an element already in the
         * vector, use that element's (possibly shift-adjusted) index.
         */
        insertion_target = v->data + to_internal_index(v, reassigned_target_e);
    }
    else
    {
        /*
         * Otherwise, we're guaranteed that the given item pointer is pointing
         * to an element outside the vector, so no reassignment is necessary.
         * Just use the given item pointer.
         */
        insertion_target = item;
    }

    memcpy(v->data + insertion_index_i,
           insertion_target,
           v->element_size);
    v->count += 1;
    v->count_bytes += v->element_size;

    return true;
}

size_t Vec_remove(Vec* v, size_t const external_index)
{
    assert(v != NULL);
    assert(v->data != NULL);
    assert(v->count > 0);
    if (v == NULL ||
        v->data == NULL ||
        v->count == 0)
    {
        return 0;
    }

    bool const index_out_of_bounds = (external_index >= v->count);

    assert(!index_out_of_bounds);
    if (index_out_of_bounds)
    {
        return v->count;
    }

    size_t internal_index = to_internal_index(v, external_index);

    /*
     * "Removing" the element from the vector's data block primarily means
     * changing the vector's metadata to let it reuse the element's bytes in the
     * block as if there were no element there anymore.
     */
    v->count -= 1;
    v->count_bytes -= v->element_size;

    /*
     * To maintain contiguity of the block data, though (and we assume it
     * was contiguous prior to removing the element), we have to shift the
     * vector's elements leftward.
     *
     * E.g., if removing the element at index 2 and the vector looks like
     * this
     *
     * ```
     * [a][b][ ][d][e][ ]
     *  0  1  2  3  4  5
     * ```
     *
     * the data is no longer contiguous, and we have to shift the elements
     * so it looks like this
     *
     * ```
     * [a][b][d][e][ ][ ]
     *  0  1  2  3  4  5
     * ```
     *
     * so the data is contiguous again.
     */
    size_t elements_to_shift = (v->count - external_index);

    while (elements_to_shift > 0)
    {
        // Shift the elements leftward to fill the gap.
        memcpy(v->data + internal_index,
               v->data + internal_index + v->element_size,
               v->element_size);

        internal_index += v->element_size;
        elements_to_shift -= 1;
    }

    /*
     * Next, whether or not any elements were shifted, there now exists a "ghost
     * element" at the end of the vector:
     *
     * ```
     * [a][b][d][e][-][ ]
     *  0  1  2  3  4  5
     * ```
     *
     * The "ghost element" is `-`.
     *
     * If we shifted elements (after removing an element `c` near the beginning
     * of the vector), `-` contains the memory of the element `e`, which used to
     * occupy the bytes at index 4 in the data block but has been shifted left
     * to index 3.
     *
     * If we didn't shift elements, `-` contains the memory of the element that
     * was removed, which was the final element in the vector.
     *
     * While the vector is free to reuse and overwrite that segment of the data
     * block, the "ghost data" in that segment might still be read (accidentally
     * or otherwise) by a user of the vector.
     *
     * The caller should reasonably expect the data of removed elements to be
     * completely "gone", but nothing prevents a later caller from obtaining the
     * `void*` to the element at index 3, casting it to a pointer type they can
     * do pointer arithmetic with, and incrementing it by one in order to read
     * the memory at index 4, recovering `-`'s ghost data. Since this data might
     * be sensitive and this might pose a security concern, we zero out the
     * "ghost element", setting its bytes to `0`:
     *
     * ```
     * [a][b][d][e][0][ ]
     *  0  1  2  3  4  5
     * ```
     *
     * (If okay with requiring C23, `memset_explicit()` should be used here
     * instead, which prevents the compiler from optimizing it away, unlike
     * `memset()`.)
     */
    memset((v->data + internal_index),
           0,
           v->element_size);

    /*
     * Finally, return the index of the next element.
     *
     * Since the elements are contiguous and we shifted them down by one, the
     * element after the one we removed is at the same index as the element we
     * removed.
     *
     * If the element we removed was the final one, we return the index of the
     * "one-past the last element". Since the elements are contiguous, that's
     * also just the index of the element we removed.
     */
    return external_index;
}

/**
 * @brief Swaps the elements at the given internal indices in the given vector
 *
 * WARNING: This may invalidate stored pointers or indices! After a vector's
 * elements are swapped, pointers or indices that used to correspond to those
 * elements in the vector will now correspond to the swapped element!
 *
 * This fails, and doesn't modify the vector (or, if assertions are enabled,
 * causes an assert crash), if any of the following are true:
 *     - The indices are the same
 *     - Either index is not aligned on an element boundary (and thus invalid
 *       since it's not pointing to the first byte of an element)
 *     - Either index is out of bounds of the vector's elements
 *     - The vector pointer is null
 *
 * @param v The vector to swap the elements of
 * @param a The internal index of an element
 * @param b The internal index of another element
 * @return Whether the elements were swapped
 */
static bool swap(Vec* v, size_t const a, size_t const b)
{
    assert(v != NULL);
    assert(v->data != NULL);
    if (v == NULL ||
        v->data == NULL)
    {
        return false;
    }

    assert(a != b);
    assert(a % v->element_size == 0); // Aligned on element boundary
    assert(b % v->element_size == 0);
    assert(a < v->count_bytes); // Within element bounds
    assert(b < v->count_bytes);
    if (a == b ||
        a % v->element_size != 0 ||
        b % v->element_size != 0 ||
        a >= v->count_bytes ||
        b >= v->count_bytes)
    {
        return false;
    }

    // Iterate through the elements' bytes and swap one byte pair at a time.
    for (size_t i = 0; i < v->element_size; ++i)
    {
        uint8_t* a_byte = (v->data + a + i);
        uint8_t* b_byte = (v->data + b + i);

        *a_byte ^= *b_byte;
        *b_byte ^= *a_byte;
        *a_byte ^= *b_byte;
    }

    return true;
}

size_t Vec_remove_all_if(Vec* v,
                         bool (*predicate)(void const*, size_t const))
{
    assert(v != NULL);
    assert(v->data != NULL);
    assert(predicate != NULL);
    if (v == NULL ||
        v->data == NULL ||
        predicate == NULL ||
        v->count == 0)
    {
        return 0;
    }

    /*
     * Remove all elements which satisfy the predicate.
     *
     * The most obvious way to do this would be to iterate through the vector
     * and, for every element that satisfies the predicate, call the regular
     * remove method with that element's index.
     *
     * However, for every removed element `x`, this would shift all the elements
     * after `x` to keep the vector contiguous. Since we're potentially removing
     * some of those shifted elements, those shifts are a waste! And each shift
     * can be thought of as a linear traversal of the vector's byte array, so
     * that's `r` traversals where `r` is the number of elements removed. That's
     * on top of the traversal we were doing to find elements that satisfy the
     * predicate, so it's a total of `r + 1` traversals.
     *
     * We can actually do it in just 1 traversal if we swap the elements that we
     * know we're NOT removing!
     *
     * We iterate through the vector while keeping an "end of the elements we're
     * NOT removing" index. Call that index `e`. For every element that does NOT
     * satisfy the predicate (i.e., that we're NOT removing), swap it with the
     * element at index `e` and then increment `e`. By the end of the iteration,
     * all the elements we're removing will be clustered together at the tail
     * end of the array. Let the elements to be removed be `x` and suppose the
     * vector initially looks like this:
     *
     * ```
     * [a][x][b][x][c][x]
     *  0  1  2  3  4  5
     *  ^
     *  e starts here
     * ```
     *
     * At the end of the iteration, the vector will look like this:
     *
     * ```
     * [a][b][c][x][x][x]
     *  0  1  2  3  4  5
     *           ^
     *           e ends up here
     * ```
     *
     * Then the `x`s can be removed all at once by "cutting off" the tail end of
     * the vector at index `e`.
     *
     * That's all elements removed with only 1 traversal and, in the worst case,
     * `k` O(1) swaps where `k` is the number of elements kept after all the
     * removals (to be convinced of that, picture an array with a single `x` at
     * the beginning and count the swaps made). This is the "erase-remove idiom"
     * from C++, but it's being done all in this function instead of through
     * container-generic algorithms.
     */
    size_t e = 0; // The end of the elements that we're NOT removing

    for (size_t i = 0; i < v->count_bytes; i += v->element_size)
    {
        bool const removing = predicate(v->data + i, v->element_size);

        // For an element we're NOT removing...
        if (!removing)
        {
            /*
             * ...if it follows an element we ARE removing (at which point `i`
             * outpaces `e`, with the element are ARE removing at `e`), swap it
             * with the element at `e`.
             */
            if (i > e)
            {
                if (!swap(v, i, e))
                {
                    // Terminate early if there was a problem swapping elements.
                    return 0;
                }
                /*
                 * After the swap, the element we're NOT removing, which was at
                 * `e`, is now at `i`, closer to the tail end of the vector.
                 *
                 * Also, update the built-in `memcmp()` predicate item pointer
                 * to the swapped element's new location, at `i`. If this call
                 * is using not using the built-in `memcmp()` predicate, this is
                 * just unfortunate overhead. However, if we are using the
                 * built-in `memcmp()` predicate, and if the caller passed in an
                 * item pointer to memory inside the vector, the swap might've
                 * just changed the bytes from underneath that pointer! That'd
                 * cause the built-in `memcmp()` predicate to compare subsequent
                 * elements against totally different bytes than those of the
                 * target element. Updating the pointer to the just-swapped
                 * element safeguards against this. Assuming the caller heeded
                 * the warning in the docs to not use bytewise comparisons for
                 * element types whose bytes may vary for otherwise equivalent
                 * elements (such as structs), the element we just swapped has
                 * bytes that match the target element's, so we can confidently
                 * use it as the target even if it's a different instance.
                 */
                item_for_memcmp_predicate = (void const*) (v->data + i);
            }
            e += v->element_size;
        }
    }

    /*
     * Remove all the elements now concentrated at the tail by chopping off the
     * tail at `e`.
     *
     * Just like when removing a single element, all this takes is updating the
     * vector's metadata to consider that number of elements gone from its tail
     * end.
     */
    size_t const elements_removed = (v->count - to_external_index(v, e));

    v->count -= elements_removed;
    v->count_bytes -= (v->element_size * elements_removed);

    /*
     * Also just like when removing a single element, zero out the memory of the
     * removed elements.
     *
     * (If okay with requiring C23, `memset_explicit()` should be used here
     * instead, which prevents the compiler from optimizing it away, unlike
     * `memset()`.)
     */
    memset((v->data + e),
           0,
           (v->element_size * elements_removed));

    return elements_removed;
}

size_t Vec_remove_all(Vec* v,
                      void const* item,
                      size_t const item_size)
{
    assert(v != NULL);
    assert(v->data != NULL);
    assert(item != NULL);
    assert(item_size == v->element_size);
    if (v == NULL ||
        v->data == NULL ||
        item == NULL ||
        item_size != v->element_size ||
        v->count == 0)
    {
        return 0;
    }

    /*
     * Remove all elements whose bytes match the given item's.
     *
     * First, store the given item pointer globally, allowing us to re-use the
     * predicate form of this function with the `memcmp()` predicate.
     */
    item_for_memcmp_predicate = item;

    return Vec_remove_all_if(v, memcmp_predicate);
}

bool Vec_qsort(Vec* v,
               int (*cmp)(void const*, void const*))
{
    assert(v != NULL);
    assert(v->data != NULL);
    assert(cmp != NULL);
    if (v == NULL ||
        v->data == NULL ||
        cmp == NULL ||
        v->count == 0)
    {
        return false;
    }

    qsort(v->data, v->count, v->element_size, cmp);

    return true;
}

int Vec_apply(Vec* v,
              int (*fun)(void* element,
                         size_t const element_size,
                         void* state),
              void* caller_state)
{
    assert(v != NULL);
    assert(v->data != NULL);
    assert(fun != NULL);
    if (v == NULL ||
        v->data == NULL ||
        fun == NULL ||
        v->count == 0)
    {
        return 1;
    }

    size_t i = 0;

    while (i < v->count_bytes)
    {
        int return_value = fun(v->data + i, v->element_size, caller_state);

        if (return_value != 0)
        {
            // Terminate early, relaying the function's error return value.
            return return_value;
        }

        i += v->element_size;
    }

    return 0;
}
