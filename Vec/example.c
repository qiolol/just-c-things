#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#include "Vec.h"

/**
 * @brief Prints the given vector of `int64_t`s
 * @param v A vector of `int64_t`s
 */
static void print_i64_Vec(Vec const* v)
{
    assert(v != NULL);

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        int64_t const* p = (int64_t const*) Vec_get(v, i);

        assert(p != NULL);
        printf("[%ld]", *p);
    }
    printf("\n");
}

/**
 * @brief An integer comparator usable for sorting elements in descending order
 * @param a The first integer
 * @param b The second integer
 * @return A positive `int` if the first integer is smaller than the second, a
 * zero `int` when it's the same size as the second, or a negative `int` when
 * it's bigger than the second or when either integer pointer is null
 */
static int i64_comparator_desc(void const* int64_a, void const* int64_b)
{
    if (int64_a == NULL ||
        int64_b == NULL)
    {
        return -1;
    }

    // We know we're dealing with `int64_t`s; cast to the known type.
    int64_t const* a = (int64_t const*) int64_a;
    int64_t const* b = (int64_t const*) int64_b;

    return (int) (*b - *a);
}

/**
 * @brief A predicate that can be used by a vector's search or removal functions
 * to target `int64_t`s that are odd
 * @param element An element provided by the vector when it calls this function
 * @param element_size The size of the element reported by the vector (usable as
 * a size-based "type check" to make sure the element looks like an `int`)
 */
static bool is_i64_odd(void const* element, size_t const element_size)
{
    /*
     * We know we're supposed to be dealing with `int64_t`s.
     *
     * If this gets used by a vector with a different element size, the wrong
     * element size will be passed in by that vector, allowing us to return
     * early.
     */
    size_t const expected_size = sizeof(int64_t);

    if (element == NULL ||
        element_size != expected_size)
    {
        return false; // Error; return early.
    }

    int64_t const* i = (int64_t const*) element; // Cast to the known type.

    return *i % 2 == 1;
}

#define UNUSED(x) (void)(x)

/**
 * @brief Multiplies the given element by -1
 * @param element A pointer to an `int64_t`
 * @param element_size For size-based type checking
 * @param state An unused pointer
 * @return 1 if there was an input problem; 0 otherwise
 */
static int negate_i64(void* element, size_t const element_size, void* state)
{
    UNUSED(state);
    if (element == NULL ||
        sizeof(int64_t) != element_size) // Explicit size check
    {
        return 1; // Signal input error
    }

    int64_t* i = (int64_t*) element; // Cast to the known type.

    *i *= -1;

    return 0; // Signal success
}

/**
 * @brief Records the smallest element seen so far
 * @param element An `int64_t`
 * @param element_size For size-based type checking
 * @param state Used to update the smallest element seen so far
 * @return 1 if there was an input problem; 0 otherwise
 */
static int store_min(void* element, size_t const element_size, void* state)
{
    if (element == NULL ||
        state == NULL ||
        sizeof(int64_t) != element_size) // Explicit size check
    {
        return 1; // Signal input error
    }

    // Cast to the known type.
    int64_t const* i = (int64_t const*) element;
    int64_t* curr_min = (int64_t*) state;

    if (*i < *curr_min)
    {
        *curr_min = *i; // Update smallest element so far
    }

    return 0; // Signal success
}

int main(void)
{
    /*
     * Make a vector big enough for five integers.
     *
     * The vector struct is an abstract data type. Instances of it are only
     * accessible via pointers obtained with `Vec_new()`, and their members are
     * inaccessible out here (not exposed through the header). A vector's state
     * is encapsulated and meant to be accessed and modified only through the
     * functions exposed by the header.
     */
    Vec* v = Vec_new(5, sizeof(int64_t));

    assert(v != NULL);

    /*
     * Add NINE integers to it.
     *
     * It'll expand its memory to fit more elements automatically. The
     * `sizeof()`s are a size-based "type check" to decrease the odds that
     * elements of the wrong type (or at least wrong size) are added by mistake.
     *
     *                                                Index:
     */
    Vec_append(v, &(int64_t){1}, sizeof(int64_t)); // 0
    Vec_append(v, &(int64_t){2}, sizeof(int64_t)); // 1
    Vec_append(v, &(int64_t){3}, sizeof(int64_t)); // 2
    Vec_append(v, &(int64_t){4}, sizeof(int64_t)); // 3
    Vec_append(v, &(int64_t){5}, sizeof(int64_t)); // 4
    Vec_append(v, &(int64_t){6}, sizeof(int64_t)); // 5
    Vec_append(v, &(int64_t){7}, sizeof(int64_t)); // 6
    Vec_append(v, &(int64_t){8}, sizeof(int64_t)); // 7
    Vec_append(v, &(int64_t){9}, sizeof(int64_t)); // 8
    assert(Vec_count(v) == 9);

    {
        // Access an element and modify it.
        void* p = Vec_get(v, 5); // Elements are returned as `void*`s...
        int64_t* pi = (int64_t*) p; // ...and can be casted to the known type.

        assert(pi != NULL);
        assert(*pi == 6);

        int64_t const new_val = 0;

        *pi = new_val;

        // The vector now contains the modified element.
        assert(5 == Vec_where(v, &new_val, sizeof(new_val)));

        /*
         * Pointers inside the vector are invalidated by vector functions like
         * additions or removals, so it's best not to keep pointers around for
         * too long.
         */
        p = NULL;
        pi = NULL;
    }

    printf("A new vector with added and modified elements:\n");
    print_i64_Vec(v);

    /*
     * Sort the vector's elements in descending order. (For this, we need a
     * comparator function.)
     */
    Vec_qsort(v, i64_comparator_desc);

    printf("\nThe vector sorted in descending order:\n");
    print_i64_Vec(v);

    /*
     * Remove all odd elements. (For this, we need an "is element odd" predicate
     * function.)
     */
    Vec_remove_all_if(v, is_i64_odd);

    printf("\nThe vector with odd elements removed:\n");
    print_i64_Vec(v);

    /*
     * Multiply all elements by -1. (For this, we need a "multiply by -1"
     * function.)
     */
    Vec_apply(v, negate_i64, NULL);

    printf("\nThe vector with all elements negated:\n");
    print_i64_Vec(v);

    /*
     * Above, we passed in `NULL` for the "state pointer", which can be used to
     * retrieve data from the vector's iteration.
     *
     * Use this to find the vector's minimum element.
     */
    int64_t max = INT64_MAX;
    int64_t* min = &max; // The "state pointer", initialized to the max value

    Vec_apply(v, store_min, min);

    printf("\nThe vector's minimum element is:\n%ld\n", *min);

    // Free the vector's memory (the function sets the pointer to null, too).
    Vec_destroy(&v);
    assert(v == NULL);

    return EXIT_SUCCESS;
}
