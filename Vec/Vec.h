/**
 * @file
 * A dynamically-resizing array (a.k.a. vector)
 *
 * The vector is exposed as an "abstract data type" whose implementation details
 * are encapsulated in this module's implementation file. This way, callers
 * can't depend on those details directly. Callers also can't access data
 * members directly (and, in so doing, corrupt the integrity of a struct), and
 * this module can thus guarantee the integrity of the caller's struct
 * instances. For example, it's impossible for the caller to mess their vectors'
 * integrity up by, say, directly changing the struct member that indicates how
 * many elements a vector holds without adding or removing any elements. This
 * being C, they can still find a way to manipulate a vector objects' memory
 * directly, like by applying offsets to pointers they receive to memory within
 * the vector (like when accessing vector elements). The encapsulation just
 * makes determined callers go out of their way to do so.
 *
 * Vector elements are "generic", retrievable as `void` pointers to be cast back
 * into their underlying type, and are stored contiguously. A vector can only be
 * used with elements of the same size that the vector was created with via
 * `Vec_new()`. Elements are appended to the vector via `Vec_append()`.
 * Functions which add elements to the vector enforce a weak "type check" by
 * taking the size of the element to be added as an argument.
 *
 * Element storage is handled automatically, expanding via reallocation as
 * necessary when elements are added. Since reallocations are costly, the
 * `Vec_new()` function reserves capacity for a minimum number of elements up
 * front, to eliminate automatic reallocations up to that number of elements.
 * The number of elements in a vector is queried via `Vec_count()`.
 *
 * Complexity (efficiency) of common vector operations (assuming `realloc()` is
 * O(n)):
 *     - Random access is constant, O(1).
 *     - Insertion or removal of elements at the end is amortized constant,
 *       O(1)+, since there may be occasional, and linear (which is greater than
 *       constant and hence amortizing), resizing when necessary.
 *     - Insertion or removal of elements NOT at the end is linear in the
 *       distance to the end of the vector, O(n).
 */
#ifndef VEC_H
#define VEC_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @typedef
 * The generic vector struct, `typedef`'d so that its implementation details are
 * encapsulated
 */
typedef struct Vec Vec;

/**
 * @brief Allocates a new vector prepared to hold the given number of elements,
 * each of which is the given size in bytes
 *
 * WARNING: This returns a dynamically allocated vector whose memory should
 * eventually be freed with `Vec_destroy()`. Otherwise, the vector's memory will
 * leak.
 *
 * NOTE: The capacity parameter is only a hint for how many elements the vector
 * should be prepared to hold. Internally, the vector may allocate memory for
 * anywhere from exactly that number of elements to around twice that number of
 * elements.
 *
 * This fails, and returns a null pointer (or, if assertions are enabled, causes
 * an assert crash), if any of the following are true:
 *     - The capacity or element size are 0
 *     - The capacity and/or element size are so astronomically huge that the
 *       total number of bytes to be allocated can't be numerically represented
 *       in the vector without overflow
 *     - Allocating memory for the vector failed, internally
 *
 * @param least_capacity The vector should be able to hold at least this many
 * elements
 * @param element_size The byte size of each element in the vector
 * @return A pointer to the newly-allocated vector
 */
Vec* Vec_new(size_t const least_capacity,
             size_t const element_size);

/**
 * @brief Destroys the given vector, taking it as a double pointer so that it
 * can null out the caller's single pointer to the vector (for convenience)
 *
 * The double pointer or inner vector pointer can be null, in which case this
 * does nothing.
 *
 * @param v A double pointer to a vector
 */
void Vec_destroy(Vec** v);

/**
 * @brief Gets the number of elements that the given vector has allocated memory
 * for so far
 *
 * If the vector pointer is null, this just returns 0 (or, if assertions are
 * enabled, causes an assert crash).
 *
 * @param v The vector
 * @return How many elements the vector can currently store
 */
size_t Vec_capacity(Vec const* v);

/**
 * @brief Gets the number of elements currently stored in the given vector
 *
 * If the vector pointer is null, this just returns 0 (or, if assertions are
 * enabled, causes an assert crash).
 *
 * @param v The vector
 * @return How many elements are in the vector
 */
size_t Vec_count(Vec const* v);

/**
 * @brief Gets the size of the element type used in the given vector
 *
 * If the vector pointer is null, this just returns 0 (or, if assertions are
 * enabled, causes an assert crash).
 *
 * @param v The vector
 * @return How many bytes worth each element in the vector is
 */
size_t Vec_element_size(Vec const* v);

/**
 * @brief Determines whether two different vectors have equivalent elements
 *
 * Two empty vectors are equivalent if both expect the same element size.
 *
 * What it means for the vectors' elements to be "equivalent" is can be
 * specified via the optional comparator function parameter. If the comparator
 * returns 0 for two elements, this function will consider those elements to be
 * equivalent. If given, the comparator function will be used to compare the two
 * vectors' elements, one by one.
 *
 * If the pointer to the comparator function is null, elements' equality will be
 * based on whether the elements' bytes are identical.
 *
 * WARNING: Bytewise vector comparison is unsuitable when vectors contain
 * structs (which may have bytewise differences even with equivalent members due
 * random values in padding bytes) or floating point values (which can differ
 * slightly due to rounding errors). If the elements of the vectors are structs
 * or floating point values, provide a comparator function (do not use a null
 * comparator pointer argument).
 *
 * This fails, and returns false (or, if assertions are enabled, causes an
 * assert crash), if either of the vector pointers are null. (The optional
 * comparator pointer may be null, however.)
 *
 * @param v_a The first vector
 * @param v_b The second vector
 * @param cmp An optional comparator function used to compare two elements that
 * returns zero when it considers its two arguments to be "equal" to each other
 * @return Whether the two vectors have the same data
 */
bool Vec_equal(Vec const* v_a,
               Vec const* v_b,
               int (*cmp)(void const*, void const*));

/**
 * @brief Determines where the first (lowest indexed) element that matches the
 * given item is located in the given vector
 *
 * An element matches if its bytes are the same as the given item's.
 *
 * WARNING: Bytewise element comparison makes this function unsuitable when
 * elements are structs (which may have bytewise differences even with
 * equivalent members due random values in padding bytes) or floating point
 * values (which can differ slightly due to rounding errors). If the elements of
 * the vector are structs or floating point values, use the form of this
 * function which takes a predicate instead.
 *
 * On failure, the following values are returned instead, each indicating one or
 * more corresponding reasons for failure (or, if assertions are enabled, an
 * assert crash happens for each failure case):
 *     - The number of elements in the vector is returned if:
 *         - The item's size does not match the vector's expected element size
 *         - The item pointer is null
 *     - 0 is returned if:
 *         - The vector pointer is null
 *
 * @param v The vector to search
 * @param item An item to search for
 * @param item_size The item's size in bytes (as a size-based "type check" to
 * make sure the item looks like it's the same type as the vector's elements)
 * @return The index of the first element that matches the item (or, if no
 * matching element was found, the number of elements) in the vector
 */
size_t Vec_where(Vec const* v,
                 void const* item,
                 size_t const item_size);

/**
 * @brief Determines where the first (lowest indexed) element that satisfies the
 * given predicate is located in the given vector
 *
 * For example, if the vector is known to hold `int`s, a predicate used to find
 * the first element less than 3 could be:
 *
 * ```
 * bool less_than_three(void const* element, size_t const element_size)
 * {
 *     if (NULL == element ||
 *         sizeof(int) != element_size) // Explicit size check
 *     {
 *         return false; // Error; return early.
 *     }
 *
 *     int const* i = (int const*) element; // Cast to the known type.
 *
 *     return *i < 3;
 * }
 * ```
 *
 * On failure, the following values are returned instead, each indicating one or
 * more corresponding reasons for failure (or, if assertions are enabled, an
 * assert crash happens for each failure case):
 *     - The number of elements in the vector is returned if:
 *         - The predicate pointer is null
 *     - 0 is returned if:
 *         - The vector pointer is null
 *
 * @param v The vector to search
 * @param predicate A function that takes an element pointer and an element size
 * and returns true if the element behind the pointer looks like the one we're
 * searching for
 * @return The index of the first element that satisfies the predicate (or, if
 * no satisfactory element was found, the number of elements) in the vector
 */
size_t Vec_where_if(Vec const* v,
                    bool (*predicate)(void const*, size_t const));

/**
 * @brief Determines if the given vector contains an element that matches the
 * given item
 *
 * An element matches if its bytes are the same as the given item's.
 *
 * WARNING: Bytewise element comparison makes this function unsuitable when
 * elements are structs (which may have bytewise differences even with
 * equivalent members due random values in padding bytes) or floating point
 * values (which can differ slightly due to rounding errors). If the elements of
 * the vector are structs or floating point values, use the form of this
 * function which takes a predicate instead.
 *
 * This fails, and returns false (or, if assertions are enabled, causes an
 * assert crash), if any of the following are true:
 *     - The item's size does not match the vector's expected element size
 *     - The vector or item pointers are null
 *
 * @param v The vector to search
 * @param item An item to search for
 * @param item_size The item's size in bytes (as a size-based "type check" to
 * make sure the item looks like it's the same type as the vector's elements)
 * @return Whether the vector has an element that matches the item
 */
bool Vec_has(Vec const* v,
             void const* item,
             size_t const item_size);

/**
 * @brief Determines if the given vector contains an element that satisfies the
 * given predicate
 *
 * For example, if the vector is known to hold `int`s, a predicate used to check
 * whether the vector contains an element less than 3 could be:
 *
 * ```
 * bool less_than_three(void const* element, size_t const element_size)
 * {
 *     if (NULL == element ||
 *         sizeof(int) != element_size) // Explicit size check
 *     {
 *         return false; // Error; return early.
 *     }
 *
 *     int const* i = (int const*) element; // Cast to the known type.
 *
 *     return *i < 3;
 * }
 * ```
 *
 * This fails, and returns false (or, if assertions are enabled, causes an
 * assert crash), if any of the following are true:
 *     - The item's size does not match the vector's expected element size
 *     - The vector or predicate pointers are null
 *
 * @param v The vector to search
 * @param predicate A function that takes an element pointer and an element size
 * and returns true if the element behind the pointer looks like the one we're
 * searching for
 * @return Whether the vector has an element that satisfies the predicate
 */
bool Vec_has_if(Vec const* v,
                bool (*predicate)(void const*, size_t const));

/**
 * @brief Accesses vector elements
 *
 * This fails, and returns a null pointer (or, if assertions are enabled, causes
 * an assert crash), if any of the following are true:
 *     - The index is out of bounds of the vector's elements
 *     - The vector pointer is null
 *
 * @param v The vector to access
 * @param i An index
 * @return A pointer to the element at the given index in the given vector (or a
 * null pointer if no element exists at the index)
 */
void* Vec_get(Vec const* v,
              size_t const i);

/**
 * @brief Appends the given item to the given vector
 *
 * When the vector is full, this function attempts to automatically resize it to
 * a larger capacity before appending the given item.
 *
 * WARNING: This may invalidate stored pointers! If the vector undergoes
 * automatic expansion to fit the new element, the vector's data block is
 * resized, so the data block may reside in a totally different region of memory
 * than the one still being pointed to by outside pointers!
 *
 * This fails, and doesn't modify the vector (or, if assertions are enabled,
 * causes an assert crash), if any of the following are true:
 *     - The given item size does not match the vector's expected element size
 *     - The number of elements in the vector becomes so huge that it can no
 *       longer be represented by the vector's internal data without overflow
 *     - Expanding the vector (if it needed to be resized) failed
 *     - The given vector or item pointers are null
 *
 * @param v The vector to append to
 * @param item The item to add
 * @param item_size The item's size in bytes (as a size-based "type check" to
 * make sure the item looks like it's the same type as the vector's elements)
 * @return Whether the item was added to the vector
 */
bool Vec_append(Vec* v,
                void const* item,
                size_t const item_size);

/**
 * @brief Inserts the given item at the given index in the given vector,
 * shifting the element at that index (and all elements after it) to the right
 *
 * For example, given a vector that looks like this
 *
 * ```
 * [a][b][ ][ ][ ]
 *  0  1  2  3  4
 * ```
 *
 * inserting an element `x` at index 1 will make it look like this:
 *
 * ```
 *     Inserted here
 *     v
 * [a][x][b][ ][ ]
 *  0  1  2  3  4
 * ```
 *
 * Insertion at the end of the elements, index 3, is also possible. However,
 * insertion past the end, at index 4, is not possible.
 *
 * ```
 *           Can insert here
 *           |  Cannot insert here
 *           v  v
 * [a][x][b][ ][ ]
 *  0  1  2  3  4
 * ```
 *
 * Inserting an element `y` at index 3 will make the vector look like this:
 *
 * ```
 *           Inserted here
 *           v
 * [a][x][b][y][ ]
 *  0  1  2  3  4
 * ```
 *
 * When the vector is full, this function attempts to automatically resize it to
 * a larger capacity before inserting the given item.
 *
 * WARNING: This may invalidate stored pointers or indices! Before an element is
 * inserted, the vector's other elements are automatically shifted to make room.
 * As a result, pre-existing pointers or indices may no longer correspond to the
 * elements they did before insertion! Also, if the vector undergoes automatic
 * expansion to fit the new element, the vector's data block is resized, so the
 * data block may reside in a totally different region of memory than the one
 * still being pointed to by outside pointers!
 *
 * This fails, leaving the vector unmodified and returning false (or, if
 * assertions are enabled, causes an assert crash), if any of the following are
 * true:
 *     - The given index is out of the bound `[0, n]` where `n` is the number of
 *       elements in the vector
 *     - The given item size does not match the vector's expected element size
 *     - The number of elements in the vector becomes so huge that it can no
 *       longer be represented by the vector's internal data without overflow
 *     - Expanding the vector (if it needed to be resized) failed
 *     - The given vector or item pointers are null
 *
 * @param v The vector to insert to
 * @param i The index to insert at
 * @param item The item to insert
 * @param item_size The item's size in bytes (as a size-based "type check" to
 * make sure the item looks like it's the same type as the vector's elements)
 * @return Whether the item was inserted in the vector
 */
bool Vec_insert(Vec* v,
                size_t const i,
                void const* item,
                size_t const item_size);

/**
 * @brief Removes the element at the given index from the given vector
 *
 * WARNING: This may invalidate stored pointers or indices! After an element is
 * removed, the vector's other elements are automatically shifted to fill the
 * empty gap created by the removal so that the elements are kept contiguous. As
 * a result, pre-existing pointers or indices may no longer correspond to the
 * elements they did before the removal (or to any element at all)!
 *
 * This fails, leaving the vector unmodified and returning 0 (or, if assertions
 * are enabled, causing an assert crash), if any of the following are true:
 *     - The vector is empty
 *     - The vector is not empty but given index is out of the bound `[0, n]`
 *       where `n` is the number of elements in the vector
 *     - The given vector pointer is null
 *
 * @param v The vector to remove from
 * @param i The index to remove from
 * @return The index of the element after the one that was removed (or the index
 * to one-past the last element if either the removed element was the final one)
 */
size_t Vec_remove(Vec* v,
                  size_t const i);

/**
 * @brief Removes all elements that match the given item from the given vector
 *
 * Elements match if their bytes match the given item's.
 *
 * WARNING: Bytewise element comparison makes this function unsuitable when
 * elements are structs (which may have bytewise differences even with
 * equivalent members due random values in padding bytes) or floating point
 * values (which can differ slightly due to rounding errors). If the elements of
 * the vector are structs or floating point values, use the form of this
 * function which takes a predicate instead.
 *
 * WARNING: This may invalidate stored pointers or indices! After an element is
 * removed, the vector's other elements are automatically shifted to fill the
 * empty gap created by the removal so that the elements are kept contiguous. As
 * a result, pre-existing pointers or indices may no longer correspond to the
 * elements they did before the removal (or to any element at all)!
 *
 * NOTE: This function is more efficient for removing multiple elements than
 * removing one element at a time via the single element removal function.
 *
 * This fails, leaving the vector unmodified and returning 0 (or, if assertions
 * are enabled, causing an assert crash), if any of the following are true:
 *     - The given item size does not match the vector's expected element size
 *     - The given vector or item pointers are null
 *
 * @param v The vector to remove from
 * @param item An item remove all instances of
 * @param item_size The item's size in bytes (as a size-based "type check" to
 * make sure the item looks like it's the same type as the vector's elements)
 * @return How many elements were removed
 */
size_t Vec_remove_all(Vec* v,
                      void const* item,
                      size_t const item_size);

/**
 * @brief Removes all elements that satisfy the given predicate from the given
 * vector
 *
 * For example, if the vector is known to hold `int`s, a predicate used to
 * remove all elements less than 3 could be:
 *
 * ```
 * bool less_than_three(void const* element, size_t const element_size)
 * {
 *     if (NULL == element ||
 *         sizeof(int) != element_size) // Explicit size check
 *     {
 *         return false; // Error; return early.
 *     }
 *
 *     int const* i = (int const*) element; // Cast to the known type.
 *
 *     return *i < 3;
 * }
 * ```
 *
 * WARNING: This may invalidate stored pointers or indices! After an element is
 * removed, the vector's other elements are automatically shifted to fill the
 * empty gap created by the removal so that the elements are kept contiguous. As
 * a result, pre-existing pointers or indices may no longer correspond to the
 * elements they did before the removal (or to any element at all)!
 *
 * NOTE: This function is more efficient for removing multiple elements than
 * removing one element at a time via the single element removal function.
 *
 * This fails, leaving the vector unmodified and returning 0 (or, if assertions
 * are enabled, causing an assert crash), if the given vector or predicate
 * pointers are null.
 *
 * @param v The vector to remove from
 * @param predicate A function that takes an element pointer and an element size
 * and returns true if the element behind the pointer looks like one of the ones
 * to remove
 * @return How many elements were removed
 */
size_t Vec_remove_all_if(Vec* v,
                         bool (*predicate)(void const*, size_t const));

/**
 * @brief Sorts the elements of the given vector according to the given
 * comparator function (using `stdlib.h`'s implementation of `qsort()`)
 *
 * WARNING: This may invalidate stored pointers or indices! After elements are
 * re-ordered by the sort, pre-existing pointers or indices may no longer
 * correspond to the elements they did before the sort! The sort is also not
 * stable, so the order of equivalent elements is unpredictable. This may matter
 * when the given comparator function considers only parts of the elements such
 * that an outside pointer to, for example, a struct may end up pointing to a
 * different struct instance with different fields and padding bytes (which are
 * ignored by the comparator) after this function is called.
 *
 * This fails, leaving the vector unmodified and returning false (or, if
 * assertions are enabled, causing an assert crash), if the given vector or
 * comparator pointers are null.
 *
 * @param v The vector to sort
 * @param cmp A comparator function that returns a negative integer when it
 * considers its first argument to be "less" than the second, a positive integer
 * when the first argument is "greater" than the second, and zero when the
 * arguments are "equal" to each other
 * @return Whether the vector was sorted
 */
bool Vec_qsort(Vec* v,
               int (*cmp)(void const*, void const*));

/**
 * @brief Applies the given function to the elements of the given vector
 *
 * For example, if the vector is known to hold `int`s, a function used to
 * increment all elements by 1 could be:
 *
 * ```
 * int add_one(void* element, size_t const element_size, void* state)
 * {
 *     if (NULL == element ||
 *         sizeof(int) != element_size) // Explicit size check
 *     {
 *         return 1; // Error; return early.
 *     }
 *
 *     int* i = (int*) element;
 *
 *     *i += 1;
 *
 *     return 0; // Continue to the next element.
 * }
 * ```
 *
 * The given function is expected to return 0 in the base case ("everything's
 * fine; proceed to call me on the next element") and non-0 when there's a
 * problem ("stop; return early"). When a non-0 returned, this function stops
 * iterating over the vector's elements, and the given function is not called on
 * any further elements.
 *
 * Using the optional `state` pointer, a function could be used to find the
 * maximum integer in the vector, reporting it back to the caller via the
 * `state` pointer:
 *
 * ```
 * int store_max(void* element, size_t const element_size, void* state)
 * {
 *     if (NULL == element ||
 *         NULL == state ||
 *         sizeof(int) != element_size) // Explicit size check
 *     {
 *         return 1; // Error; terminate early.
 *     }
 *
 *     int const* i = (int const*) element;
 *     int* curr_max = (int*) state;
 *
 *     if (*i > *curr_max)
 *     {
 *         *curr_max = *i;
 *     }
 *
 *     return 0; // Continue to the next element.
 * }
 * ```
 *
 * This fails, leaving the vector unmodified and returning 1 (or, if assertions
 * are enabled, causing an assert crash), if the vector or function pointers are
 * null. (The optional caller state pointer may be null, however.)
 *
 * @param v The vector to apply over
 * @param fun A function that takes an element pointer, an element size, and an
 * optional state pointer (for, e.g., communication to/from the caller), does
 * something with the element (including potentially modifying the element), and
 * returns a non-zero `int` to indicate an error/early return (which will cause
 * this vector function to cease iterating over the vector's elements and also
 * return early with that non-zero error value) or a zero `int` to proceed
 * @param caller_state An optional pointer to call the function with (so
 * the function can communicate with the caller if it needs to)
 * @return 1 if an assert-worthy precondition failed or if the vector is empty;
 * the first non-0 returned by a call of the given function (indicating early
 * termination and return) or 0 if no calls of the given function returned non-0
 * (indicating the given function was called on all the elements in the vector
 * without early termination)
 */
int Vec_apply(Vec* v,
              int (*fun)(void* element, size_t const element_size, void* state),
              void* caller_state);

#endif
