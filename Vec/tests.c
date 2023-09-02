#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "Vec.h"

static void test_new(void)
{
    // Trivial case
    Vec* v = NULL;
    size_t const cap_hint = 5;

    v = Vec_new(cap_hint, sizeof(int));

    assert(v != NULL);
    // Actual capacity should be greater than or equal to the capacity hint.
    assert(Vec_capacity(v) >= cap_hint);
    assert(Vec_count(v) == 0); // Eempty
    assert(Vec_element_size(v) == sizeof(int)); // Expected element size

    Vec_destroy(&v);

    assert(v == NULL); // Destruction function nulled out the pointer

    // When the capacity hint is 0
    v = Vec_new(0, sizeof(int));
    assert(v == NULL);

    // When the element size is 0
    v = Vec_new(5, 0);
    assert(v == NULL);

    // When both of those are 0
    v = Vec_new(0, 0);
    assert(v == NULL);

    /*
     * `SIZE_MAX * 2` will overflow the total number of bytes the vector needs
     * to keep track of for its elements' memory.
     */
    v = Vec_new(SIZE_MAX, 2);
    assert(v == NULL);

    // Same with `2 * SIZE_MAX`.
    v = Vec_new(2, SIZE_MAX);
    assert(v == NULL);

    // This overflows even harder.
    v = Vec_new(SIZE_MAX, SIZE_MAX);
    assert(v == NULL);
}

static void test_destroy(void)
{
    Vec* v = Vec_new(5, sizeof(int));

    // Empty vector
    assert(v != NULL);
    Vec_destroy(&v);
    assert(v == NULL);

    // Non-empty vector
    v = Vec_new(5, sizeof(int[100]));
    assert(v != NULL);
    assert(Vec_append(v, &(int[100]){0}, sizeof((int[100]){0})) == true);
    assert(Vec_count(v) == 1);
    Vec_destroy(&v);

    // Null pointer (which should be an allowable no-op for both pointers)
    assert(v == NULL);
    assert(&v != NULL);
    Vec_destroy(&v);
    Vec_destroy(NULL);
}

static void test_where_invalid(void)
{
    Vec* v = Vec_new(5, sizeof(int));

    assert(v != NULL);

    // Since the vector is empty, the number of elements, 0, is returned.
    assert(0 == Vec_count(v));
    assert(0 == Vec_where(v, &(int){42}, sizeof(int)));

    // With a null item pointer, the number of elements is returned as well.
    assert(0 == Vec_where(v, NULL, sizeof(int))); // The number of elements is 0.
    assert(true == Vec_append(v, &(int){777}, sizeof(int))); // Add an element.
    // Now the number of elements is 1.
    assert(1 == Vec_count(v));
    assert(1 == Vec_where(v, NULL, sizeof(int)));

    // Ditto for when the item size differs from the vector's element size.
    assert(1 == Vec_where(v, &(int){42}, 0));
    assert(1 == Vec_where(v, &(int){42}, sizeof(int[5])));

    // If the vector pointer is null, 0 is always returned.
    assert(0 == Vec_where(NULL, &(int){42}, sizeof(int)));

    Vec_destroy(&v);
}

static void test_where(void)
{
    Vec* v = Vec_new(5, sizeof(int));

    assert(v != NULL);

    /*
     * Find the index of various values in the vector:
     *
     * ```
     * [5][6][6][6][7]
     *  0  1  2  3  4
     * ```
     */
    assert(true == Vec_append(v, &(int){5}, sizeof(int)));
    assert(true == Vec_append(v, &(int){6}, sizeof(int)));
    assert(true == Vec_append(v, &(int){6}, sizeof(int)));
    assert(true == Vec_append(v, &(int){6}, sizeof(int)));
    assert(true == Vec_append(v, &(int){7}, sizeof(int)));
    assert(5 == Vec_count(v));

    // `5` is at index 0.
    assert(0 == Vec_where(v, &(int){5}, sizeof(int)));

    // `7` is at index 4.
    assert(4 == Vec_where(v, &(int){7}, sizeof(int)));

    /*
     * `6` is at indices 1 through 3, but the first instance, at index 1, is
     * always the one whose index is returned.
     */
    assert(1 == Vec_where(v, &(int){6}, sizeof(int)));
    assert(1 == Vec_where(v, &(int){6}, sizeof(int)));
    assert(1 == Vec_where(v, &(int){6}, sizeof(int)));

    // The number of elements is returned when an element is not found.
    assert(Vec_count(v) == Vec_where(v, &(int){42}, sizeof(int)));

    Vec_destroy(&v);
}

static void test_where_inner_pointer(void)
{
    /*
     * Make the following vector:
     *
     * ```
     * [9][8][7]
     *  0  1  2
     * ```
     */
    Vec* v = Vec_new(3, sizeof(uint32_t));

    assert(v != NULL);

    assert(true == Vec_append(v, &(uint32_t){9}, sizeof(uint32_t)));
    assert(true == Vec_append(v, &(uint32_t){8}, sizeof(uint32_t)));
    assert(true == Vec_append(v, &(uint32_t){7}, sizeof(uint32_t)));
    assert(3 == Vec_count(v));

    // Get a pointer to the first element inside the vector.
    uint32_t* probe = (uint32_t*) Vec_get(v, 0);

    // Verify that the vector correctly reports its location.
    assert(probe != NULL);
    assert(0 == Vec_where(v, probe, sizeof(*probe)));

    // Ditto for the second element
    probe = (uint32_t*) Vec_get(v, 1);
    assert(probe != NULL);
    assert(1 == Vec_where(v, probe, sizeof(*probe)));

    // Ditto for the third element
    probe = (uint32_t*) Vec_get(v, 2);
    assert(probe != NULL);
    assert(2 == Vec_where(v, probe, sizeof(*probe)));

    Vec_destroy(&v);
}

/**
 * @brief A predicate that can be used by a vector's search or removal functions
 * to target `int`s less than 3
 * @param element An element provided by the vector when it calls this function
 * @param element_size The size of the element reported by the vector (usable as
 * a size-based "type check" to make sure the element looks like an `int`)
 */
static bool less_than_three(void const* element, size_t const element_size)
{
    /*
     * We know we're supposed to be dealing with `int`s.
     *
     * If this gets used by a vector with a different element size, the wrong
     * element size will be passed in by that vector, allowing us to return
     * early.
     */
    size_t const expected_size = sizeof(int);

    if (NULL == element ||
        element_size != expected_size)
    {
        return false; // Error; return early.
    }

    int const* i = (int const*) element; // Cast to the known type.

    return *i < 3;
}

static void test_where_if_invalid(void)
{
    Vec* v = Vec_new(5, sizeof(int16_t));

    assert(v != NULL);

    // Since the vector is empty, the number of elements, 0, is returned.
    assert(0 == Vec_count(v));
    assert(0 == Vec_where_if(v, less_than_three));

    // With a null predicate pointer, the number of elements is returned too.
    assert(0 == Vec_where_if(v, NULL)); // Number of elements is 0
    // Add an element.
    assert(true == Vec_append(v, &(int16_t){777}, sizeof(int16_t)));
    // Now the number of elements is 1.
    assert(1 == Vec_count(v));
    assert(1 == Vec_where_if(v, NULL));

    /*
     * Ditto for when the item size differs from the vector's element size.
     *
     * First, we'll need a new vector with a different declared element size.
     */
    Vec_destroy(&v);
    v = Vec_new(5, sizeof(int64_t));
    assert(v != NULL);

    // Add a few elements.
    assert(true == Vec_append(v, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){9}, sizeof(int64_t)));
    assert(3 == Vec_count(v));

    /*
     * Now search for an element of the wrong size.
     *
     * `less_than_three()` expects `sizeof(int)` but `Vec_where_if()` will give
     * it `sizeof(int64_t)`.
     *
     * The predicate will then return false for all elements. This is
     * essentially the same as never finding an element that satisfies the
     * predicate, causing the number of elements to be returned.
     */
    assert(Vec_count(v) == Vec_where_if(v, less_than_three));

    // If the vector pointer is null, 0 is always returned.
    assert(0 == Vec_where_if(NULL, less_than_three));

    Vec_destroy(&v);
}

static void test_where_if(void)
{
    Vec* v = Vec_new(5, sizeof(int));
    int* probe = NULL;

    assert(v != NULL);

    /*
     * Make the following vector:
     *
     * ```
     * [2][5][5][2][1]
     *  0  1  2  3  4
     * ```
     */
    assert(true == Vec_append(v, &(int){2}, sizeof(int)));
    assert(true == Vec_append(v, &(int){5}, sizeof(int)));
    assert(true == Vec_append(v, &(int){5}, sizeof(int)));
    assert(true == Vec_append(v, &(int){2}, sizeof(int)));
    assert(true == Vec_append(v, &(int){1}, sizeof(int)));
    assert(5 == Vec_count(v));

    // The first element less than 3 should be at index 0.
    assert(0 == Vec_where_if(v, less_than_three));
    probe = (int*) Vec_get(v, Vec_where_if(v, less_than_three));
    assert(probe != NULL);
    assert(*probe == (int){2});
    // The same index is returned across repeated calls.
    assert(0 == Vec_where_if(v, less_than_three));
    assert(0 == Vec_where_if(v, less_than_three));
    assert(0 == Vec_where_if(v, less_than_three));

    // Now remove the element at that index.
    Vec_remove(v, Vec_where_if(v, less_than_three));

    /*
     * The vector now looks like this:
     *
     * ```
     * [5][5][2][1][ ]
     *  0  1  2  3  4
     * ```
     *
     * So the first element less than 3 should now be at index 2.
     */
    assert(2 == Vec_where_if(v, less_than_three));
    probe = (int*) Vec_get(v, Vec_where_if(v, less_than_three));
    assert(probe != NULL);
    assert(*probe == (int){2});

    // Remove that element as well.
    Vec_remove(v, Vec_where_if(v, less_than_three));

    /*
     * The vector now looks like this:
     *
     * ```
     * [5][5][1][ ][ ]
     *  0  1  2  3  4
     * ```
     *
     * The first element less than 3 should still be at index 2 since it got
     * shifted to the left.
     */
    assert(2 == Vec_where_if(v, less_than_three));
    probe = (int*) Vec_get(v, Vec_where_if(v, less_than_three));
    assert(probe != NULL);
    assert(*probe == (int){1});

    // Remove that element as well.
    Vec_remove(v, Vec_where_if(v, less_than_three));

    /*
     * The vector now looks like this:
     *
     * ```
     * [5][5][ ][ ][ ]
     *  0  1  2  3  4
     * ```
     *
     * There are no more elements less than 3, so the number of elements in the
     * vector should be returned on the next query.
     */
    assert(Vec_count(v) == Vec_where_if(v, less_than_three));

    Vec_destroy(&v);
}

static void test_has_invalid(void)
{
    Vec* v = Vec_new(5, sizeof(uint16_t));
    uint16_t const element = 7;

    assert(v != NULL);
    assert(true == Vec_append(v, &element, sizeof(element)));
    assert(Vec_count(v) == 1);
    assert(true == Vec_has(v, &element, sizeof(uint16_t)));

    // When the item size disagrees with the vector's declared element size
    assert(false == Vec_has(v, &element, sizeof(uint32_t)));

    // When pointers are null
    assert(false == Vec_has(v, NULL, sizeof(uint16_t)));
    assert(false == Vec_has(NULL, &element, sizeof(element)));
    assert(false == Vec_has(NULL, NULL, sizeof(uint16_t)));

    Vec_destroy(&v);
}

static void test_has(void)
{
    /*
     * Make the following vector:
     *
     * ```
     * [3][5][5][7]
     *  0  1  2  3
     * ```
     */
    Vec* v = Vec_new(5, sizeof(int));
    int const first = 3;
    int const middle = 5;
    int const last = 7;

    // `false` is returned when searching an empty vector.
    assert(v != NULL);
    assert(false == Vec_has(v, &first, sizeof(first)));

    // Add the elements.
    assert(true == Vec_append(v, &first, sizeof(first)));
    assert(true == Vec_append(v, &middle, sizeof(middle)));
    assert(true == Vec_append(v, &middle, sizeof(middle)));
    assert(true == Vec_append(v, &last, sizeof(last)));
    assert(Vec_count(v) == 4);

    // We should find the first element.
    assert(true == Vec_has(v, &first, sizeof(first)));

    // We should find an element in the middle.
    assert(true == Vec_has(v, &middle, sizeof(middle)));

    // We should find the last element.
    assert(true == Vec_has(v, &last, sizeof(last)));

    // We should not find elements that aren't in the vector.
    assert(false == Vec_has(v, &(int){6}, sizeof(int)));
    assert(false == Vec_has(v, &(int){42}, sizeof(int)));
    assert(false == Vec_has(v, &(int){13}, sizeof(int)));

    Vec_destroy(&v);
}

static void test_has_inner_pointer(void)
{
    /*
     * Make the following vector:
     *
     * ```
     * [9][8][7]
     *  0  1  2
     * ```
     */
    Vec* v = Vec_new(3, sizeof(int32_t));

    assert(v != NULL);
    assert(true == Vec_append(v, &(int32_t){9}, sizeof(int32_t)));
    assert(true == Vec_append(v, &(int32_t){8}, sizeof(int32_t)));
    assert(true == Vec_append(v, &(int32_t){7}, sizeof(int32_t)));
    assert(3 == Vec_count(v));

    // Get a pointer to the first element inside the vector.
    int32_t* probe = (int32_t*) Vec_get(v, 0);

    // The vector should report possessing the first element.
    assert(probe != NULL);
    assert(true == Vec_has(v, probe, sizeof(*probe)));

    // Ditto for the second element
    probe = (int32_t*) Vec_get(v, 1);
    assert(probe != NULL);
    assert(true == Vec_has(v, probe, sizeof(*probe)));

    // Ditto for the third element
    probe = (int32_t*) Vec_get(v, 2);
    assert(probe != NULL);
    assert(true == Vec_has(v, probe, sizeof(*probe)));

    Vec_destroy(&v);
}

static void test_has_if_invalid(void)
{
    Vec* v = Vec_new(5, sizeof(uint16_t));
    uint16_t const element = 2;

    assert(v != NULL);
    assert(true == Vec_append(v, &element, sizeof(element)));
    assert(Vec_count(v) == 1);
    assert(true == Vec_has(v, &element, sizeof(uint16_t)));

    /*
     * Search for an element of the wrong size.
     *
     * `less_than_three()` expects `sizeof(int)` but `Vec_where_if()` will give
     * it `sizeof(uint16_t)`.
     *
     * The predicate will then return false for all elements. This is
     * essentially the same as never finding an element that satisfies the
     * predicate, so we expect to always get back false.
     */
    assert(false == Vec_has_if(v, less_than_three));

    // When pointers are null
    assert(false == Vec_has_if(v, NULL));
    assert(false == Vec_has_if(NULL, less_than_three));
    assert(false == Vec_has_if(NULL, NULL));

    Vec_destroy(&v);
}

static void test_has_if_helper(Vec* v, size_t const n, size_t const X_index)
{
    /*
     * Make a vector that looks like this
     *
     * ```
     * [X][y][y][y]
     *  0  1  2  3
     * ```
     *
     * where `X` is an element we're searching for via the `less_than_three()`
     * predicate (so, `X` is some `int` less than 3) at whatever `X_index` is
     * and `y` is any other element.
     */
    int const X = -2;

    // Add `n` elements, among them one `X` at `X_index`.
    for (size_t i = 0; i < n; ++i)
    {
        if (i == X_index)
        {
            assert(true == Vec_append(v, &X, sizeof(X)));
        }
        else
        {
            assert(true == Vec_append(v, &(int){5}, sizeof(int)));
        }
    }

    // Check vector content.
    assert(Vec_count(v) == n);
    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        int* probe = NULL;

        probe = (int*) Vec_get(v, i);
        assert(probe != NULL);

        if (i == X_index)
        {
            assert(true == less_than_three(probe, sizeof(int)));
        }
        else
        {
            assert(false == less_than_three(probe, sizeof(int)));
        }
    }
}

static void test_has_if_head(void)
{
    size_t const cap = 4;
    Vec* v = Vec_new(cap, sizeof(int));

    // `false` is returned when searching an empty vector.
    assert(v != NULL);
    assert(false == Vec_has_if(v, less_than_three));

    /*
     * Make a vector that looks like this:
     *
     * ```
     * [X][y][y][y]
     *  0  1  2  3
     * ```
     */
    size_t const X_index = 0;

    test_has_if_helper(v, cap, X_index);

    // Search for the `X`.
    assert(true == Vec_has_if(v, less_than_three));

    Vec_destroy(&v);
}

static void test_has_if_middle(void)
{
    size_t const cap = 4;
    Vec* v = Vec_new(cap, sizeof(int));

    assert(v != NULL);

    /*
     * Make a vector that looks like this:
     *
     * ```
     * [y][y][X][y]
     *  0  1  2  3
     * ```
     */
    size_t const X_index = 2;

    test_has_if_helper(v, cap, X_index);

    // Search for the `X`.
    assert(true == Vec_has_if(v, less_than_three));

    Vec_destroy(&v);
}

static void test_has_if_tail(void)
{
    size_t const cap = 4;
    Vec* v = Vec_new(cap, sizeof(int));

    assert(v != NULL);

    /*
     * Make a vector that looks like this:
     *
     * ```
     * [y][y][y][X]
     *  0  1  2  3
     * ```
     */
    size_t const X_index = 3;

    test_has_if_helper(v, cap, X_index);

    // Search for the `X`.
    assert(true == Vec_has_if(v, less_than_three));

    Vec_destroy(&v);
}

static void test_has_if_nowhere(void)
{
    size_t const cap = 4;
    Vec* v = Vec_new(cap, sizeof(int));

    assert(v != NULL);

    /*
     * Make a vector that looks like this:
     *
     * ```
     * [y][y][y][y]
     *  0  1  2  3
     * ```
     */
    assert(true == Vec_append(v, &(int){5}, sizeof(int)));
    assert(true == Vec_append(v, &(int){6}, sizeof(int)));
    assert(true == Vec_append(v, &(int){7}, sizeof(int)));
    assert(true == Vec_append(v, &(int){8}, sizeof(int)));

    // Check vector content.
    assert(Vec_count(v) == cap);
    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        int* probe = NULL;

        probe = (int*) Vec_get(v, i);
        assert(probe != NULL);
        assert(false == less_than_three(probe, sizeof(int)));
    }

    // Search for the `X`. (It shouldn't exist.)
    assert(false == Vec_has_if(v, less_than_three));

    Vec_destroy(&v);
}

static void test_get_invalid(void)
{
    size_t const cap_hint = 5;
    Vec* v = Vec_new(cap_hint, sizeof(int));

    // A null pointer is returned when searching an empty vector.
    assert(v != NULL);
    assert(NULL == Vec_get(v, 0));

    // Add some elements.
    assert(true == Vec_append(v, &(int){5}, sizeof(int)));
    assert(true == Vec_append(v, &(int){6}, sizeof(int)));
    assert(true == Vec_append(v, &(int){7}, sizeof(int)));
    assert(Vec_count(v) == 3); // The elements were added.

    // The initial capacity should be greater than the number of elements.
    assert(Vec_capacity(v) > Vec_count(v));

    /*
     * The getter function fails, and returns a null pointer, if the given index
     * is out of bounds of a vector's elements.
     *
     * ```
     * [a][b][c][ ][ ]
     *  0  1  2  3  4
     * ```
     *
     * So, for the vector above, only indices 0, 1, and 2, can be queried. The
     * bound of the vector's elements is [0, 2].
     *
     * Query an index out of the vector's bounds.
     */
    int* probe = NULL;

    probe = (int*) Vec_get(v, Vec_count(v));
    assert(probe == NULL); // Getter failed and returned a null pointer.

    probe = (int*) Vec_get(v, Vec_count(v) + 1);
    assert(probe == NULL);

    probe = (int*) Vec_get(v, Vec_capacity(v));
    assert(probe == NULL);

    Vec_destroy(&v);
}

static void test_get(void)
{
    Vec* v = Vec_new(5, sizeof(int));
    int const first = 5;
    int const second = 6;
    int const third = 7;
    int const fourth = 8;
    int const fifth = 9;

    assert(v != NULL);
    assert(true == Vec_append(v, &first, sizeof(first)));
    assert(true == Vec_append(v, &second, sizeof(second)));
    assert(true == Vec_append(v, &third, sizeof(third)));
    assert(true == Vec_append(v, &fourth, sizeof(fourth)));
    assert(true == Vec_append(v, &fifth, sizeof(fifth)));
    assert(Vec_count(v) == 5);

    // Verify the elements can be retrieved successfully.
    int* probe = NULL;

    probe = (int*) Vec_get(v, 0);
    assert(probe != NULL);
    assert(*probe == first);

    probe = (int*) Vec_get(v, 1);
    assert(probe != NULL);
    assert(*probe == second);

    probe = (int*) Vec_get(v, 2);
    assert(probe != NULL);
    assert(*probe == third);

    probe = (int*) Vec_get(v, 3);
    assert(probe != NULL);
    assert(*probe == fourth);

    probe = (int*) Vec_get(v, 4);
    assert(probe != NULL);
    assert(*probe == fifth);

    Vec_destroy(&v);
}

static void test_append_invalid(void)
{
    Vec* v = Vec_new(5, sizeof(int));

    assert(v != NULL);

    // When the item size disagrees with the vector's declared element size
    assert(Vec_append(v, &(int){2}, sizeof(int[50])) == false);
    assert(Vec_count(v) == 0);

    // When pointers are null
    assert(Vec_append(NULL, &(int){3}, sizeof((int){3})) == false);
    assert(Vec_append(v, NULL, sizeof((int){3})) == false);
    assert(Vec_append(NULL, NULL, sizeof((int){3})) == false);
    assert(Vec_count(v) == 0);

    Vec_destroy(&v);
}

static void test_append(void)
{
    Vec* v = Vec_new(5, sizeof(int));

    assert(v != NULL);

    // Trivial case
    assert(Vec_count(v) == 0);
    assert(Vec_append(v, &(int){1}, sizeof((int){1})) == true);
    assert(Vec_count(v) == 1);

    Vec_destroy(&v);

    // Automatic expansion
    size_t const least_cap_hint = 1;

    v = Vec_new(least_cap_hint, sizeof(uint64_t[4]));
    assert(v != NULL);

    size_t const initial_cap = Vec_capacity(v);

    assert(initial_cap >= least_cap_hint); // The hint is the minimum capacity.

    // Fill up the vector to its initial capacity.
    for (size_t i = 0; i < initial_cap; ++i)
    {
        uint64_t const arr[4] = {i, i, i, i};

        assert(Vec_append(v, arr, sizeof(arr)) == true);
        assert(Vec_count(v) == (i + 1));
    }
    assert(Vec_count(v) == Vec_capacity(v)); // Now, the vector should be full.
    assert(Vec_capacity(v) == initial_cap); // And it shouldn't've resized yet.

    // Add one more element.
    uint64_t arr[4] = {4, 3, 2, 1};

    assert(Vec_append(v, arr, sizeof(arr)) == true);
    assert(Vec_count(v) == (initial_cap + 1)); // Going past initial capacity
    assert(Vec_capacity(v) > initial_cap); // Should have triggered expansion

    // The elements should be in the same order as appended above.
    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        uint64_t (*stored_arr)[4] = (uint64_t (*)[4]) Vec_get(v, i);

        if (i < (Vec_count(v) - 1))
        {
            for (size_t j = 0; j < 4; ++j)
            {
                // The elements are arrays of all `i`s for the `i`th element.
                assert((*stored_arr)[j] == i);
            }
        }
        else if (i == (Vec_count(v) - 1))
        {
            // But the last element is the hardcoded `arr`.
            assert(memcmp(stored_arr, arr, sizeof(uint64_t[4])) == 0);
        }
    }

    Vec_destroy(&v);
}

static void test_append_inner_pointer(void)
{
    /*
     * Make the following vector:
     *
     * ```
     * [9][8][7][ ][ ][ ]
     *  0  1  2  3  4  5
     * ```
     */
    Vec* v = Vec_new(6, sizeof(int32_t));
    int32_t const first = 9;
    int32_t const second = 8;
    int32_t const third = 7;

    assert(v != NULL);
    assert(true == Vec_append(v, &first, sizeof(int32_t)));
    assert(true == Vec_append(v, &second, sizeof(int32_t)));
    assert(true == Vec_append(v, &third, sizeof(int32_t)));
    assert(3 == Vec_count(v));

    /*
     * Get a pointer to the last element in the vector and append it. Then the
     * vector should look like this:
     *
     * ```
     * [9][8][7][7][ ][ ]
     *  0  1  2  3  4  5
     * ```
     */
    int32_t* probe = (int32_t*) Vec_get(v, 2);
    size_t prev_cap = Vec_capacity(v);

    assert(probe != NULL);
    assert(*probe == third);
    assert(true == Vec_append(v, probe, sizeof(*probe)));
    probe = NULL; // Pointer invalidated by the append
    assert(Vec_capacity(v) == prev_cap); // No resize should have occurred.
    // A copy of the element should've be added.
    assert(4 == Vec_count(v));
    probe = (int32_t*) Vec_get(v, 3); // Get the newly-added element.
    assert(probe != NULL);
    assert(*probe == third); // Check that it's a copy of the last element.

    /*
     * Repeat with the second element to make the vector look like this:
     *
     * ```
     * [9][8][7][7][8][ ]
     *  0  1  2  3  4  5
     * ```
     */
    probe = (int32_t*) Vec_get(v, 1);
    prev_cap = Vec_capacity(v);

    assert(probe != NULL);
    assert(*probe == second);
    assert(true == Vec_append(v, probe, sizeof(*probe)));
    probe = NULL; // Pointer invalidated by the append
    assert(Vec_capacity(v) == prev_cap); // No resize should have occurred.
    // A copy of the element should've be added.
    assert(5 == Vec_count(v));
    probe = (int32_t*) Vec_get(v, 4);
    assert(probe != NULL);
    assert(*probe == second);

    /*
     * Repeat with the first element to make the vector look like this:
     *
     * ```
     * [9][8][7][7][8][9]
     *  0  1  2  3  4  5
     * ```
     */
    probe = (int32_t*) Vec_get(v, 0);
    prev_cap = Vec_capacity(v);

    assert(probe != NULL);
    assert(*probe == first);
    assert(true == Vec_append(v, probe, sizeof(*probe)));
    probe = NULL; // Pointer invalidated by the append
    assert(Vec_capacity(v) == prev_cap); // No resize should have occurred.
    // A copy of the element should've be added.
    assert(6 == Vec_count(v));
    probe = (int32_t*) Vec_get(v, 5);
    assert(probe != NULL);
    assert(*probe == first);

    Vec_destroy(&v);
}

static void test_insert_invalid(void)
{
    Vec* v = Vec_new(10, sizeof(uint64_t));

    assert(v != NULL);

    // Insert something out of bounds of the capacity.
    assert(Vec_insert(v,
                      Vec_capacity(v),
                      &(uint64_t){0},
                      sizeof((uint64_t){0})) == false);
    assert(Vec_count(v) == 0); // Insertion was rejected.

    // Insert something out of `[0, n]` bounds (`n` being the element count, 0).
    assert(Vec_insert(v,
                      1,
                      &(uint64_t){1},
                      sizeof((uint64_t){1})) == false);
    assert(Vec_count(v) == 0); // Insertion was rejected.

    // Insert something within `[0, n]` bounds.
    uint64_t const first = 1776;
    uint64_t const second = 1787;
    uint64_t* probe = NULL;

    assert(Vec_insert(v,
                      0,
                      &first,
                      sizeof((uint64_t){0})) == true);
    assert(Vec_count(v) == 1); // Insertion was accepted.
    probe = (uint64_t*) Vec_get(v, 0);
    assert(probe != NULL);
    assert(*probe == first);
    probe = NULL;

    assert(Vec_insert(v,
                      1,
                      &second,
                      sizeof((uint64_t){1})) == true);
    assert(Vec_count(v) == 2); // Insertion was accepted.
    probe = (uint64_t*) Vec_get(v, 1);
    assert(probe != NULL);
    assert(*probe == second);
    probe = NULL;

    // `n` is now 2. Insert something out of `[0, n]` bounds again, at 3.
    assert(Vec_insert(v,
                      3,
                      &(uint64_t){3},
                      sizeof((uint64_t){3})) == false);
    assert(Vec_count(v) == 2); // Insertion was rejected.

    // Insert something with an incorrect size.
    assert(Vec_insert(v,
                      0,
                      &(uint32_t){9},
                      sizeof((uint32_t){9})) == false); // A `uint32_t`
    assert(Vec_count(v) == 2); // Insertion was rejected.

    assert(Vec_insert(v,
                      0,
                      &(uint64_t){9},
                      sizeof((uint32_t){9})) == false); // Just `uint32_t` size
    assert(Vec_count(v) == 2); // Insertion was rejected.

    assert(Vec_insert(v,
                      0,
                      &(char){'a'},
                      sizeof((char){'a'})) == false); // A `char`
    assert(Vec_count(v) == 2); // Insertion was rejected.

    /*
     * Insert a different kind of element that's the same size as the expected
     * element type. This SHOULD work even though it's sort of an abuse of the
     * vector's weak typing.
     *
     * In this case, the expected element type is `uint64_t`, so let's try 8
     * `uint8_t`s!
     */
    uint8_t const diff_kind[8] = { 1, 0, 0, 0, 0, 0, 0, 0 };

    // Insert the array of `uint8_t`s as though it were a single `uint64_t`.
    assert(Vec_insert(v,
                      0,
                      &diff_kind,
                      sizeof(diff_kind)) == true);
    assert(Vec_count(v) == 3); // Insertion was accepted.

    /*
     * As a `uint64_t`, an array like `{ 1, 0, 0, 0, 0, 0, 0, 0 }` is 1 if read
     * as little-endian. Whatever it is, it should be the same when read back as
     * a `uint64_t`.
     */
    uint64_t as_a_single_number = 0;

    memcpy(&as_a_single_number, &diff_kind, sizeof(uint8_t[8]));
    probe = (uint64_t*) Vec_get(v, 0);
    assert(probe != NULL);
    assert(*probe == as_a_single_number); // Read the array as one `uint64_t`.
    probe = NULL;

    // Insert with null pointers.
    assert(Vec_insert(NULL,
                      0,
                      NULL,
                      sizeof((uint64_t){9})) == false);
    assert(Vec_count(v) == 3); // Insertion was rejected.

    assert(Vec_insert(NULL,
                      0,
                      &(uint64_t){9},
                      sizeof((uint64_t){9})) == false);
    assert(Vec_count(v) == 3); // Insertion was rejected.

    assert(Vec_insert(v,
                      0,
                      NULL,
                      sizeof((uint64_t){9})) == false);
    assert(Vec_count(v) == 3); // Insertion was rejected.

    Vec_destroy(&v);
}

static void test_insert_empty(void)
{
    size_t const initial_cap_hint = 10;
    Vec* v = Vec_new(initial_cap_hint, sizeof(char));

    assert(v != NULL);
    assert(Vec_capacity(v) >= initial_cap_hint);

    /*
     * Insert something in an empty vector.
     *
     * ```
     *  Will insert here
     *  v
     * [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]. . .
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    size_t initial_cap = Vec_capacity(v);

    assert(Vec_insert(v, 0, &(char){'x'}, sizeof(char)) == true);

    /*
     * The vector should now look like:
     *
     * ```
     *  Inserted here
     *  v
     * [x][ ][ ][ ][ ][ ][ ][ ][ ][ ]. . .
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    assert(Vec_count(v) == 1);
    assert(Vec_capacity(v) == initial_cap); // No resize should have occurred.

    char* probe = NULL;

    probe = (char*) Vec_get(v, 0);
    assert(probe != NULL);
    assert(*probe == (char){'x'});

    Vec_destroy(&v);
}

static void test_insert_slack_middle(void)
{
    Vec* v = Vec_new(5, sizeof(char));

    assert(v != NULL);

    // Add some elements to the vector.
    size_t const initial_cap = Vec_capacity(v);

    Vec_append(v, &(char){'a'}, sizeof(char)); // Index 0
    Vec_append(v, &(char){'c'}, sizeof(char)); // Index 1
    Vec_append(v, &(char){'d'}, sizeof(char)); // Index 2
    assert(Vec_count(v) == 3);
    assert(Vec_capacity(v) == initial_cap); // No resize should have occurred.

    /*
     * Insert something in the middle of some elements when the vector still has
     * slack (so it won't resize due to the insertion):
     *
     * ```
     *     Will insert here
     *     v
     * [a][c][d][ ][ ]. . .
     *  0  1  2  3  4
     * ```
     */
    assert(Vec_insert(v, 1, &(char){'b'}, sizeof(char)) == true);

    /*
     * The vector should now look like:
     *
     * ```
     *     Inserted here
     *     v
     * [a][b][c][d][ ]. . .
     *  0  1  2  3  4
     * ```
     */
    assert(Vec_count(v) == 4);
    assert(Vec_capacity(v) == initial_cap); // Still no resize has occurred.

    char* probe = NULL;

    probe = (char*) Vec_get(v, 0);
    assert(probe != NULL);
    assert(*probe == 'a');
    probe = NULL;

    probe = (char*) Vec_get(v, 1);
    assert(probe != NULL);
    assert(*probe == 'b');
    probe = NULL;

    probe = (char*) Vec_get(v, 2);
    assert(probe != NULL);
    assert(*probe == 'c');
    probe = NULL;

    probe = (char*) Vec_get(v, 3);
    assert(probe != NULL);
    assert(*probe == 'd');

    Vec_destroy(&v);
}

typedef enum Color
{
    RED,
    GREEN,
    BLUE,
    COLOR_NUM
} Color;

typedef struct Fish Fish;
struct Fish
{
    Color color;
    size_t size;
};

static void test_insert_slack_middle_struct(void)
{
    Vec* v = Vec_new(5, sizeof(Fish));

    assert(v != NULL);

    // Add some elements to the vector.
    size_t const initial_cap = Vec_capacity(v);

    Vec_append(v,
               &(Fish)
               {
                   .color = RED,
                   .size = 0
               },
               sizeof(Fish)); // Index 0
    Vec_append(v,
               &(Fish)
               {
                   .color = GREEN,
                   .size = 1
               },
               sizeof(Fish)); // Index 1
    Vec_append(v,
               &(Fish)
               {
                   .color = BLUE,
                   .size = 2
               },
               sizeof(Fish)); // Index 2
    assert(Vec_count(v) == 3);
    assert(Vec_capacity(v) == initial_cap); // No resize should have occurred.

    /*
     * Insert something in the middle of the `Fish` (denoted by their colors)
     * when the vector still has slack (so it won't resize due to the
     * insertion):
     *
     * ```
     *     Will insert here
     *     v
     * [R][G][B][ ][ ]. . .
     *  0  1  2  3  4
     * ```
     */
    assert(Vec_insert(v,
                      1,
                      &(Fish)
                      {
                          .color = BLUE,
                          .size = 10
                      },
                      sizeof(Fish)) == true);

    /*
     * The vector should now look like this (with the `Fish` denoted by their
     * colors and the inserted element by `X`):
     *
     * ```
     *     Inserted here
     *     v
     * [R][X][G][B][ ]. . .
     *  0  1  2  3  4
     * ```
     */
    assert(Vec_count(v) == 4);
    assert(Vec_capacity(v) == initial_cap); // Still no resize has occurred.

    Fish* probe = NULL;

    probe = (Fish*) Vec_get(v, 0);
    assert(probe != NULL);
    assert(probe->color == RED);
    assert(probe->size == 0);
    probe = NULL;

    probe = (Fish*) Vec_get(v, 1);
    assert(probe != NULL);
    assert(probe->color == BLUE);
    assert(probe->size == 10);
    probe = NULL;

    probe = (Fish*) Vec_get(v, 2);
    assert(probe != NULL);
    assert(probe->color == GREEN);
    assert(probe->size == 1);
    probe = NULL;

    probe = (Fish*) Vec_get(v, 3);
    assert(probe != NULL);
    assert(probe->color == BLUE);
    assert(probe->size == 2);
    probe = NULL;

    Vec_destroy(&v);
}

static void test_insert_slack_tail(void)
{
    Vec* v = Vec_new(5, sizeof(char));

    assert(v != NULL);

    // Add some elements to the vector.
    size_t const initial_cap = Vec_capacity(v);

    Vec_append(v, &(char){'a'}, sizeof(char)); // Index 0
    Vec_append(v, &(char){'b'}, sizeof(char)); // Index 1
    Vec_append(v, &(char){'d'}, sizeof(char)); // Index 2
    assert(Vec_count(v) == 3);
    assert(Vec_capacity(v) == initial_cap); // No resize should have occurred.

    /*
     * Insert something at the tail end of the vector's elements when the vector
     * still has slack.
     *
     * ```
     *        Will insert here
     *        v
     * [a][b][d][ ][ ]. . .
     *  0  1  2  3  4
     * ```
     */
    assert(Vec_insert(v, 2, &(char){'c'}, sizeof(char)) == true);

    /*
     * The vector should now look like:
     *
     * ```
     *        Inserted here
     *        v
     * [a][b][c][d][ ]. . .
     *  0  1  2  3  4
     * ```
     */
    assert(Vec_count(v) == 4);
    assert(Vec_capacity(v) == initial_cap); // Still no resize has occurred.

    char* probe = NULL;

    probe = (char*) Vec_get(v, 0);
    assert(probe != NULL);
    assert(*probe == 'a');
    probe = NULL;

    probe = (char*) Vec_get(v, 1);
    assert(probe != NULL);
    assert(*probe == 'b');
    probe = NULL;

    probe = (char*) Vec_get(v, 2);
    assert(probe != NULL);
    assert(*probe == 'c');
    probe = NULL;

    probe = (char*) Vec_get(v, 3);
    assert(probe != NULL);
    assert(*probe == 'd');

    Vec_destroy(&v);
}

static void test_insert_slack_past_tail(void)
{
    Vec* v = Vec_new(5, sizeof(char));

    assert(v != NULL);

    // Add some elements to the vector.
    size_t const initial_cap = Vec_capacity(v);

    Vec_append(v, &(char){'a'}, sizeof(char)); // Index 0
    Vec_append(v, &(char){'b'}, sizeof(char)); // Index 1
    Vec_append(v, &(char){'c'}, sizeof(char)); // Index 2
    assert(Vec_count(v) == 3);
    assert(Vec_capacity(v) == initial_cap); // No resize should have occurred.

    /*
     * Insert something at one past the last index of the vector's elements when
     * the vector still has slack.
     *
     * ```
     *           Will insert here
     *           v
     * [a][b][c][ ][ ]. . .
     *  0  1  2  3  4
     * ```
     */
    assert(Vec_insert(v, 3, &(char){'d'}, sizeof(char)) == true);

    /*
     * The vector should now look like:
     *
     * ```
     *           Inserted here
     *           v
     * [a][b][c][d][ ]. . .
     *  0  1  2  3  4
     * ```
     */
    assert(Vec_count(v) == 4);
    assert(Vec_capacity(v) == initial_cap); // Still no resize has occurred.

    char* probe = NULL;

    probe = (char*) Vec_get(v, 0);
    assert(probe != NULL);
    assert(*probe == 'a');
    probe = NULL;

    probe = (char*) Vec_get(v, 1);
    assert(probe != NULL);
    assert(*probe == 'b');
    probe = NULL;

    probe = (char*) Vec_get(v, 2);
    assert(probe != NULL);
    assert(*probe == 'c');
    probe = NULL;

    probe = (char*) Vec_get(v, 3);
    assert(probe != NULL);
    assert(*probe == 'd');

    Vec_destroy(&v);
}

static void test_insert_slack_head(void)
{
    Vec* v = Vec_new(5, sizeof(char));

    assert(v != NULL);

    // Add some elements to the vector.
    size_t const initial_cap = Vec_capacity(v);

    Vec_append(v, &(char){'b'}, sizeof(char)); // Index 0
    Vec_append(v, &(char){'c'}, sizeof(char)); // Index 1
    Vec_append(v, &(char){'d'}, sizeof(char)); // Index 2
    assert(Vec_count(v) == 3);
    assert(Vec_capacity(v) == initial_cap); // No resize should have occurred.

    /*
     Insert something at the beginning of the vector when the vector has slack.
     *
     * ```
     *  Will insert here
     *  v
     * [b][c][d][ ][ ]. . .
     *  0  1  2  3  4
     * ```
     */
    assert(Vec_insert(v, 0, &(char){'a'}, sizeof(char)) == true);

    /*
     * The vector should now look like:
     *
     * ```
     *  Inserted here
     *  v
     * [a][b][c][d][ ]. . .
     *  0  1  2  3  4
     * ```
     */
    assert(Vec_count(v) == 4);
    assert(Vec_capacity(v) == initial_cap); // Still no resize has occurred.

    char* probe = NULL;

    probe = (char*) Vec_get(v, 0);
    assert(probe != NULL);
    assert(*probe == 'a');
    probe = NULL;

    probe = (char*) Vec_get(v, 1);
    assert(probe != NULL);
    assert(*probe == 'b');
    probe = NULL;

    probe = (char*) Vec_get(v, 2);
    assert(probe != NULL);
    assert(*probe == 'c');
    probe = NULL;

    probe = (char*) Vec_get(v, 3);
    assert(probe != NULL);
    assert(*probe == 'd');

    Vec_destroy(&v);
}

static void test_insert_until_full(void)
{
    Vec* v = Vec_new(5, sizeof(uint64_t));

    assert(v != NULL);

    /*
     * Fill the vector to capacity using inserts.
     *
     * ```
     *  Will insert at every index
     *  v  v  v  v  v  v . . . v
     * [ ][ ][ ][ ][ ][ ]. . .[ ]
     *  0  1  2  3  4  5       ^
     *                         Current capacity
     * ```
     */
    size_t curr_cap = Vec_capacity(v);

    for (size_t i = 0; i < curr_cap; ++i)
    {
        assert(Vec_insert(v,
                          i,
                          &(uint64_t){i},
                          sizeof(uint64_t)) == true);
    }
    assert(Vec_count(v) == curr_cap); // Vector should be full.
    assert(Vec_capacity(v) == curr_cap); // But it hasn't resized yet.

    /*
     * The vector should now look like:
     *
     * ```
     *  Inserted at every index
     *  v  v  v  v  v  v . . . v
     * [0][1][2][3][4][5]. . .[x]
     *  0  1  2  3  4  5       ^
     *                         Current capacity
     * ```
     */
    for (size_t i = 0; i < curr_cap; ++i)
    {
        uint64_t* probe = NULL;

        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);
        assert(*probe == (uint64_t){i});
    }

    Vec_destroy(&v);
}

/**
 * @param initial_cap_hint The vector's minimum initial capacity
 * @returns A new vector sized according to the given capacity hint, full to
 * capacity with different `uint64_t` elements
 */
static Vec* new_full_u64_vec(size_t const initial_cap_hint)
{
    Vec* v = Vec_new(initial_cap_hint, sizeof(uint64_t));

    assert(v != NULL);

    // Fill the vector to capacity.
    size_t const initial_cap = Vec_capacity(v);

    for (size_t i = 0; i < initial_cap; ++i)
    {
        Vec_append(v, &(uint64_t){i}, sizeof((uint64_t){i}));
    }
    assert(Vec_count(v) == initial_cap); // The vector's full.
    assert(Vec_capacity(v) == initial_cap); // The vector didn't expand yet.

    // Every element should be equal to its index.
    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        uint64_t* probe = NULL;

        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);
        assert(*probe == (uint64_t){i});
    }

    return v;
}

static void test_insert_full_middle(void)
{
    Vec* v = new_full_u64_vec(10); // Get a full vector.

    /*
     * Insert something in the middle the vector when it's full.
     *
     * ```
     *              Will insert here
     *              v
     * [0][1][2][3][4][5][6][7][8][9]. . .[x]
     *  0  1  2  3  4  5  6  7  8  9       ^
     *                                     Initial capacity
     * ```
     */
    size_t initial_cap = Vec_capacity(v);
    size_t const inserted_element_index = 4;
    uint64_t const inserted_element = 9;

    assert(Vec_insert(v,
                      inserted_element_index,
                      &inserted_element,
                      sizeof(uint64_t)) == true);

    /*
     * The vector should now look like:
     *
     * ```
     *              Inserted here
     *              v
     * [0][1][2][3][9][4][5][6][7][8]. . .[x - 1][x]. . .[ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9         ^                  ^
     *                                       Initial capacity   New capacity
     * ```
     */
    assert(Vec_capacity(v) > initial_cap); // Resize should have occurred.
    assert(Vec_count(v) == (initial_cap + 1));

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        uint64_t* probe = NULL;

        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i < inserted_element_index)
        {
            // The elements before the insertion site are still `i`.
            assert(*probe == i);
        }
        else if (i == inserted_element_index)
        {
            // The insertion site contains the inserted element.
            assert(*probe == inserted_element);
        }
        else
        {
            // The elements after the insertion site are `i - 1`.
            assert(*probe == (i - 1));
        }
    }

    Vec_destroy(&v);
}

/**
 * @param i An index
 * @return A `Fish` whose color and size is based on the given index
 */
static Fish index_to_Fish(size_t const i)
{
    return (Fish)
    {
        .color = i % COLOR_NUM,
        .size = i
    };
}

/**
 * @param initial_cap_hint The vector's minimum initial capacity
 * @returns A new vector sized according to the given capacity hint, full to
 * capacity with different `Fish` elements
 */
static Vec* new_full_Fish_vec(size_t const initial_cap_hint)
{
    Vec* v = Vec_new(initial_cap_hint, sizeof(Fish));

    assert(v != NULL);

    // Fill the vector to capacity.
    size_t const initial_cap = Vec_capacity(v);

    for (size_t i = 0; i < initial_cap; ++i)
    {
        Fish const f = index_to_Fish(i);

        Vec_append(v, &f, sizeof(f));
    }
    assert(Vec_count(v) == initial_cap); // The vector's full.
    assert(Vec_capacity(v) == initial_cap); // The vector didn't expand yet.

    // Every element should be equal to its index.
    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        Fish* probe = NULL;

        probe = (Fish*) Vec_get(v, i);
        assert(probe != NULL);

        Fish const f = index_to_Fish(i);

        assert(probe->color == f.color);
        assert(probe->size == f.size);
    }

    return v;
}

static void test_insert_full_middle_struct(void)
{
    Vec* v = new_full_Fish_vec(10); // Get a full vector.

    /*
     * Insert something in the middle the vector when it's full.
     *
     * ```
     *              Will insert here
     *              v
     * [0][1][2][3][4][5][6][7][8][9]. . .[x]
     *  0  1  2  3  4  5  6  7  8  9       ^
     *                                     Initial capacity
     * ```
     */
    size_t initial_cap = Vec_capacity(v);
    size_t const inserted_element_index = 4;
    Fish const inserted_element =
    {
        .color = BLUE,
        .size = 123456
    };

    assert(Vec_insert(v,
                      inserted_element_index,
                      &inserted_element,
                      sizeof(Fish)) == true);

    /*
     * The vector should now look like this (with the inserted element denoted
     * by `X`):
     *
     * ```
     *              Inserted here
     *              v
     * [0][1][2][3][X][4][5][6][7][8]. . .[x - 1][x]. . .[ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9         ^                  ^
     *                                       Initial capacity   New capacity
     * ```
     */
    assert(Vec_capacity(v) > initial_cap); // Resize should have occurred.
    assert(Vec_count(v) == (initial_cap + 1));

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        Fish* probe = NULL;
        Fish expected;

        probe = (Fish*) Vec_get(v, i);
        assert(probe != NULL);

        if (i < inserted_element_index)
        {
            // The elements before the insertion site still correspond to `i`.
            expected = index_to_Fish(i);
            assert(probe->color == expected.color);
            assert(probe->size == expected.size);
        }
        else if (i == inserted_element_index)
        {
            // The insertion site contains the inserted element.
            assert(probe->color == inserted_element.color);
            assert(probe->size == inserted_element.size);
        }
        else
        {
            // The elements after the insertion site correspond to `i - 1`.
            expected = index_to_Fish(i - 1);
            assert(probe->color == expected.color);
            assert(probe->size == expected.size);
        }
    }

    Vec_destroy(&v);
}

static void test_insert_full_tail(void)
{
    Vec* v = new_full_u64_vec(10); // Get a full vector.

    /*
     * Insert something at the last index.
     *
     * ```
     *                                     Will insert here
     *                                     v
     * [0][1][2][3][4][5][6][7][8][9]. . .[x]
     *  0  1  2  3  4  5  6  7  8  9       ^
     *                                     Initial capacity
     * ```
     */
    size_t const initial_cap = Vec_capacity(v);
    size_t const inserted_element_index = Vec_count(v) - 1; // The last index
    uint64_t const inserted_element = 42;
    uint64_t const shifted_element = Vec_count(v) - 1; // `x` will be shifted.

    assert(Vec_insert(v,
                      inserted_element_index,
                      &inserted_element,
                      sizeof(inserted_element)) == true);

    /*
     * We expect that:
     *     - The vector was automatically resized
     *     - The element that occupied the last index before the insertion was
     *       shifted into the newly-expanded region of the vector
     *
     * If the element we just inserted is `I` and the element that occupied the
     * last index before the insertion is `P`, the vector should look like this:
     *
     * ```
     *                         Inserted here
     *                         v
     * [0][1][2][3][4][5]. . .[I][P][ ]. . .[ ][ ][ ][ ]
     *  0  1  2  3  4  5       ^                      ^
     *                         Initial capacity       Current capacity
     * ```
     */
    assert(Vec_capacity(v) > initial_cap); // Resize should have occurred.
    assert(Vec_count(v) == (initial_cap + 1));

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        uint64_t* probe = NULL;

        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i < inserted_element_index)
        {
            // The elements before the insertion site are still `i`.
            assert(*probe == i);
        }
        else if (i == inserted_element_index)
        {
            // The insertion site contains the inserted element.
            assert(*probe == inserted_element);
        }
        else if (i == (inserted_element_index + 1))
        {
            // The element after the insertion site is the shifted element.
            assert(*probe == shifted_element);
        }
    }

    Vec_destroy(&v);
}

static void test_insert_full_past_tail(void)
{
    Vec* v = new_full_u64_vec(10); // Get a full vector.

    /*
     * Insert something at one past the last index.
     *
     * ```
     *                                        Will insert here
     *                                        v
     * [0][1][2][3][4][5][6][7][8][9]. . .[x] _
     *  0  1  2  3  4  5  6  7  8  9       ^
     *                                     Initial capacity
     * ```
     */
    size_t const initial_cap = Vec_capacity(v);
    size_t const inserted_element_index = Vec_count(v); // One past last index
    uint64_t const inserted_element = 777;

    assert(Vec_insert(v,
                      inserted_element_index,
                      &inserted_element,
                      sizeof(inserted_element)) == true);

    /*
     * The vector should have expanded and, if the element we just inserted is
     * `I`, look like this:
     *
     * ```
     *                            Inserted here
     *                            v
     * [0][1][2][3][4][5]. . .[x][I][ ]. . .[ ][ ][ ][ ]
     *  0  1  2  3  4  5       ^                      ^
     *                         Initial capacity       Current capacity
     * ```
     */
    assert(Vec_capacity(v) > initial_cap); // Resize should have occurred.
    assert(Vec_count(v) == (initial_cap + 1));

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        uint64_t* probe = NULL;

        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i < inserted_element_index)
        {
            // The elements before the insertion site are still `i`.
            assert(*probe == i);
        }
        else if (i == inserted_element_index)
        {
            // The insertion site contains the inserted element.
            assert(*probe == inserted_element);
        }
    }

    Vec_destroy(&v);
}

static void test_insert_full_head(void)
{
    Vec* v = new_full_u64_vec(10); // Get a full vector.

    /*
     * Insert something at the head of the vector.
     *
     * ```
     *  Will insert here
     *  v
     * [0][1][2][3][4][5][6][7][8][9]. . .[x]
     *  0  1  2  3  4  5  6  7  8  9       ^
     *                                     Initial capacity
     * ```
     */
    size_t const initial_cap = Vec_capacity(v);
    size_t const inserted_element_index = 0; // Head index
    uint64_t const inserted_element = 2023;

    assert(Vec_insert(v,
                      inserted_element_index,
                      &inserted_element,
                      sizeof(inserted_element)) == true);

    /*
     * The vector should have expanded and, if the element we just inserted is
     * `I`, look like this:
     *
     * ```
     *  Inserted here
     *  v
     * [I][0][1][2][3][4][5]. . .[x][I][ ]. . .[ ][ ][ ][ ]
     *  0  1  2  3  4  5          ^                      ^
     *                            Initial capacity       Current capacity
     * ```
     */
    assert(Vec_capacity(v) > initial_cap); // Resize should have occurred.
    assert(Vec_count(v) == (initial_cap + 1));

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        uint64_t* probe = NULL;

        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i > inserted_element_index)
        {
            // The elements after the insertion site shifted, so are `i - 1`.
            assert(*probe == (i - 1));
        }
        else
        {
            // The insertion site contains the inserted element.
            assert(*probe == inserted_element);
        }
    }

    Vec_destroy(&v);
}

static Vec* insert_inner_pointer_input_helper(void)
{
    /*
     * Make a non-full vector of unique elements:
     *
     * ```
     * [9][8][7][6][5][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    Vec* v = Vec_new(10, sizeof(int64_t));

    assert(v != NULL);
    assert(true == Vec_append(v, &(int64_t){9}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){6}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){5}, sizeof(int64_t)));
    assert(5 == Vec_count(v));
    assert(Vec_capacity(v) > Vec_count(v)); // The vector's not full.

    return v;
}

static void test_insert_inner_pointer_head_before_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *        target
     *        |  Will insert here
     *        v  v
     *       [9][8][7][6][5][ ][ ][ ][ ][ ]
     *        0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = insert_inner_pointer_input_helper();

    // Get a pointer to the element.
    int64_t const element = 9;
    size_t const target_index = 0;
    size_t const insertion_index = 1;
    int64_t* target = (int64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *     Inserted here
     *     v
     * [9][9][8][7][6][5][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    Vec* expected = Vec_new(10, sizeof(int64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t))); // New
    assert(true == Vec_append(expected, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){6}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t)));
    assert(Vec_count(expected) == prev_count + 1);

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_insert_inner_pointer_head_at_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     * target   Will insert here
     *       \ /
     *        |
     *        v
     *       [9][8][7][6][5][ ][ ][ ][ ][ ]
     *        0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = insert_inner_pointer_input_helper();

    // Get a pointer to the element.
    int64_t const element = 9;
    size_t const target_index = 0;
    size_t const insertion_index = 0;
    int64_t* target = (int64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *  Inserted here
     *  v
     * [9][9][8][7][6][5][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    Vec* expected = Vec_new(10, sizeof(int64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t))); // New
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){6}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t)));
    assert(Vec_count(expected) == prev_count + 1);

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_insert_inner_pointer_head_after_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *        Will insert here
     *        |  target
     *        v  v
     *       [9][8][7][6][5][ ][ ][ ][ ][ ]
     *        0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = insert_inner_pointer_input_helper();

    // Get a pointer to the element.
    int64_t const element = 8;
    size_t const target_index = 1;
    size_t const insertion_index = 0;
    int64_t* target = (int64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *  Inserted here
     *  v
     * [8][9][8][7][6][5][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    Vec* expected = Vec_new(10, sizeof(int64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &element, sizeof(int64_t))); // New
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){6}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t)));
    assert(Vec_count(expected) == prev_count + 1);

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_insert_inner_pointer_middle_before_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *        target
     *        |  Will insert here
     *        v  v
     * [9][8][7][6][5][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = insert_inner_pointer_input_helper();

    // Get a pointer to the element.
    int64_t const element = 7;
    size_t const target_index = 2;
    size_t const insertion_index = 3;
    int64_t* target = (int64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *           Inserted here
     *           v
     * [9][8][7][7][6][5][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    Vec* expected = Vec_new(10, sizeof(int64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){7}, sizeof(int64_t))); // New
    assert(true == Vec_append(expected, &(int64_t){6}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t)));
    assert(Vec_count(expected) == prev_count + 1);

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_insert_inner_pointer_middle_at_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *    target   Will insert here
     *          \ /
     *           |
     *           v
     * [9][8][7][6][5][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = insert_inner_pointer_input_helper();

    // Get a pointer to the element.
    int64_t const element = 6;
    size_t const target_index = 3;
    size_t const insertion_index = 3;
    int64_t* target = (int64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *           Inserted here
     *           v
     * [9][8][7][6][6][5][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    Vec* expected = Vec_new(10, sizeof(int64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){6}, sizeof(int64_t))); // New
    assert(true == Vec_append(expected, &(int64_t){6}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t)));
    assert(Vec_count(expected) == prev_count + 1);

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_insert_inner_pointer_middle_after_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *        Will insert here
     *        |  target
     *        v  v
     * [9][8][7][6][5][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = insert_inner_pointer_input_helper();

    // Get a pointer to the element.
    int64_t const element = 6;
    size_t const target_index = 3;
    size_t const insertion_index = 2;
    int64_t* target = (int64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *        Inserted here
     *        v
     * [9][8][6][7][6][5][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    Vec* expected = Vec_new(10, sizeof(int64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){6}, sizeof(int64_t))); // New
    assert(true == Vec_append(expected, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){6}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t)));
    assert(Vec_count(expected) == prev_count + 1);

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

/**
 * @brief A `Fish` comparator to be used by a vector's equality function that
 * looks only at fish size
 * @param fish_a A `Fish`
 * @param fish_b Another `Fish`
 * @return 1 if there was an input problem; 0 if the given `Fish` have the same
 * size; 2 if they have different sizes
 */
static int fish_size_equality_comparator(void const* fish_a, void const* fish_b)
{
    if (fish_a == NULL ||
        fish_b == NULL)
    {
        return 1;
    }

    // We know we're dealing with `Fish`; cast to the known type.
    Fish const* a = (Fish const*) fish_a;
    Fish const* b = (Fish const*) fish_b;

    if (a->size == b->size)
    {
        return 0;
    }

    return 2;
}

static void test_insert_inner_pointer_middle_after_insertion_struct(void)
{
    /*
     * Using a pointer, `target`, to a `Fish` inside the not-full vector
     *
     * ```
     *     Will insert here
     *     |     target
     *     v     v
     * [9][8][7][6][5][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * insert a copy of that `Fish` in somwhere among the others (denoted by
     * their sizes).
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = Vec_new(10, sizeof(Fish));

    assert(v != NULL);

    // Add some unique elements to the vector.
    size_t const initial_cap = Vec_capacity(v);

    Vec_append(v,
               &(Fish)
               {
                   .color = RED,
                   .size = 9
               },
               sizeof(Fish)); // Index 0
    Vec_append(v,
               &(Fish)
               {
                   .color = GREEN,
                   .size = 8
               },
               sizeof(Fish)); // Index 1
    Vec_append(v,
               &(Fish)
               {
                   .color = BLUE,
                   .size = 7
               },
               sizeof(Fish)); // Index 2
    Vec_append(v,
               &(Fish)
               {
                   .color = RED,
                   .size = 6
               },
               sizeof(Fish)); // Index 3
    Vec_append(v,
               &(Fish)
               {
                   .color = GREEN,
                   .size = 5
               },
               sizeof(Fish)); // Index 4
    assert(Vec_count(v) == 5);
    assert(Vec_capacity(v) == initial_cap); // No resize should have occurred.

    // Get a pointer to the element.
    Fish const element =
    {
        .color = RED,
        .size = 6
    };
    size_t const target_index = 3;
    size_t const insertion_index = 1;
    Fish* target = (Fish*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(target->color == element.color);
    assert(target->size == element.size);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *     Inserted here
     *     v
     * [9][6][8][7][6][5][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    Vec* expected = Vec_new(10, sizeof(Fish));

    assert(expected != NULL);
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 9
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 6
                              },
                              sizeof(Fish))); // Inserted
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 8
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = BLUE,
                                  .size = 7
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 6
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 5
                              },
                              sizeof(Fish)));
    assert(Vec_count(expected) == prev_count + 1);

    assert(true == Vec_equal(v, expected, fish_size_equality_comparator));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_insert_inner_pointer_tail_before_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *              target
     *              |  Will insert here
     *              v  v
     * [9][8][7][6][5][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = insert_inner_pointer_input_helper();

    // Get a pointer to the element.
    int64_t const element = 5;
    size_t const target_index = 4;
    size_t const insertion_index = 5;
    int64_t* target = (int64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *                 Inserted here
     *                 v
     * [9][8][7][6][5][5][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    Vec* expected = Vec_new(10, sizeof(int64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){6}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t))); // New
    assert(Vec_count(expected) == prev_count + 1);

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_insert_inner_pointer_tail_at_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *       target   Will insert here
     *             \ /
     *              |
     *              v
     * [9][8][7][6][5][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = insert_inner_pointer_input_helper();

    // Get a pointer to the element.
    int64_t const element = 5;
    size_t const target_index = 4;
    size_t const insertion_index = 4;
    int64_t* target = (int64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *              Inserted here
     *              v
     * [9][8][7][6][5][5][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    Vec* expected = Vec_new(10, sizeof(int64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){6}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t))); // New
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t)));
    assert(Vec_count(expected) == prev_count + 1);

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_insert_inner_pointer_tail_after_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *           Will insert here
     *           |  target
     *           v  v
     * [9][8][7][6][5][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = insert_inner_pointer_input_helper();

    // Get a pointer to the element.
    int64_t const element = 5;
    size_t const target_index = 4;
    size_t const insertion_index = 3;
    int64_t* target = (int64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *           Inserted here
     *           v
     * [9][8][7][5][6][5][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    Vec* expected = Vec_new(10, sizeof(int64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t))); // New
    assert(true == Vec_append(expected, &(int64_t){6}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t)));
    assert(Vec_count(expected) == prev_count + 1);

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_insert_full_inner_pointer_head_before_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *        target
     *        |  Will insert here
     *        v  v
     *       [0][1][2]. . .[8][9][a]. . .[d][e][f]
     *        0  1  2       8  9  a       d  e  f
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = new_full_u64_vec(16);

    // Get a pointer to the element.
    uint64_t const element = 0;
    size_t const target_index = 0;
    size_t const insertion_index = 1;
    uint64_t* target = (uint64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);
    size_t const prev_cap = Vec_capacity(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.
    assert(Vec_capacity(v) > prev_cap); // The vector was full, so it expanded.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *     Inserted here
     *     v
     * [0][0][1]. . .[7][8][9]. . .[c][d][e][f][ ][ ][ ]. . .
     *  0  1  2       8  9  a       d  e  f (Expanded space)
     * ```
     */
    uint64_t* probe = NULL;

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i == 0 || i == 1)
        {
            assert(*probe == element); // Inserted
        }
        else
        {
            assert(*probe == (uint64_t){i - 1}); // Shifted
        }
    }

    Vec_destroy(&v);
}

static void test_insert_full_inner_pointer_head_at_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     * target   Will insert here
     *       \ /
     *        |
     *        v
     *       [0][1][2]. . .[8][9][a]. . .[d][e][f]
     *        0  1  2       8  9  a       d  e  f
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = new_full_u64_vec(16);

    // Get a pointer to the element.
    uint64_t const element = 0;
    size_t const target_index = 0;
    size_t const insertion_index = 0;
    uint64_t* target = (uint64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);
    size_t const prev_cap = Vec_capacity(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.
    assert(Vec_capacity(v) > prev_cap); // The vector was full, so it expanded.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *  Inserted here
     *  v
     * [0][0][1]. . .[7][8][9]. . .[c][d][e][f][ ][ ][ ]. . .
     *  0  1  2       8  9  a       d  e  f (Expanded space)
     * ```
     */
    uint64_t* probe = NULL;

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i == 0 || i == 1)
        {
            assert(*probe == element); // Inserted
        }
        else
        {
            assert(*probe == (uint64_t){i - 1}); // Shifted
        }
    }

    Vec_destroy(&v);
}

static void test_insert_full_inner_pointer_head_after_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *        Will insert here
     *        |  target
     *        v  v
     *       [0][1][2]. . .[8][9][a]. . .[d][e][f]
     *        0  1  2       8  9  a       d  e  f
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = new_full_u64_vec(16);

    // Get a pointer to the element.
    uint64_t const element = 1;
    size_t const target_index = 1;
    size_t const insertion_index = 0;
    uint64_t* target = (uint64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);
    size_t const prev_cap = Vec_capacity(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.
    assert(Vec_capacity(v) > prev_cap); // The vector was full, so it expanded.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *  Inserted here
     *  v
     * [1][0][1]. . .[7][8][9]. . .[c][d][e][f][ ][ ][ ]. . .
     *  0  1  2       8  9  a       d  e  f (Expanded space)
     * ```
     */
    uint64_t* probe = NULL;

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i == 0)
        {
            assert(*probe == element); // Inserted
        }
        else
        {
            assert(*probe == (uint64_t){i - 1}); // Shifted
        }
    }

    Vec_destroy(&v);
}

static void test_insert_full_inner_pointer_middle_before_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *                   target
     *                   |  Will insert here
     *                   v  v
     * [0][1][2]. . .[7][8][9]. . .[d][e][f]
     *  0  1  2       7  8  9       d  e  f
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = new_full_u64_vec(16);

    // Get a pointer to the element.
    uint64_t const element = Vec_count(v) / 2; // Assuming elements are indices
    size_t const target_index = Vec_count(v) / 2;
    size_t const insertion_index = target_index + 1;
    uint64_t* target = (uint64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);
    size_t const prev_cap = Vec_capacity(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.
    assert(Vec_capacity(v) > prev_cap); // The vector was full, so it expanded.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *                      Inserted here
     *                      |  Shifted elements begin here
     *                      v  v
     * [0][1][2]. . .[7][8][8][9]. . .[c][d][e][f][ ][ ][ ]. . .
     *  0  1  2       7  8  9  a       d  e  f (Expanded space)
     * ```
     */
    uint64_t* probe = NULL;

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i < insertion_index)
        {
            assert(*probe == (uint64_t){i}); // Unchanged
        }
        else if (i == insertion_index)
        {
            assert(*probe == element); // Inserted
        }
        else
        {
            assert(*probe == (uint64_t){i - 1}); // Shifted
        }
    }

    Vec_destroy(&v);
}

static void test_insert_full_inner_pointer_middle_at_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *            target   Will insert here
     *                  \ /
     *                   |
     *                   v
     * [0][1][2]. . .[7][8][9]. . .[d][e][f]
     *  0  1  2       7  8  9       d  e  f
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = new_full_u64_vec(16);

    // Get a pointer to the element.
    uint64_t const element = Vec_count(v) / 2; // Assuming elements are indices
    size_t const target_index = Vec_count(v) / 2;
    size_t const insertion_index = target_index;
    uint64_t* target = (uint64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);
    size_t const prev_cap = Vec_capacity(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.
    assert(Vec_capacity(v) > prev_cap); // The vector was full, so it expanded.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *                   Inserted here
     *                   |  Shifted elements begin here
     *                   v  v
     * [0][1][2]. . .[7][8][8]. . .[c][d][e][f][ ][ ][ ]. . .
     *  0  1  2       7  8  9       d  e  f (Expanded space)
     * ```
     */
    uint64_t* probe = NULL;

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i < insertion_index)
        {
            assert(*probe == (uint64_t){i}); // Unchanged
        }
        else if (i == insertion_index)
        {
            assert(*probe == element); // Inserted
        }
        else
        {
            assert(*probe == (uint64_t){i - 1}); // Shifted
        }
    }

    Vec_destroy(&v);
}

static void test_insert_full_inner_pointer_middle_after_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *                Will insert here
     *                |  target
     *                v  v
     * [0][1][2]. . .[7][8][9]. . .[d][e][f]
     *  0  1  2       7  8  9       d  e  f
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = new_full_u64_vec(16);

    // Get a pointer to the element.
    uint64_t const element = Vec_count(v) / 2; // Assuming elements are indices
    size_t const target_index = Vec_count(v) / 2;
    size_t const insertion_index = target_index - 1;
    uint64_t* target = (uint64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);
    size_t const prev_cap = Vec_capacity(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.
    assert(Vec_capacity(v) > prev_cap); // The vector was full, so it expanded.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *                Inserted here
     *                |  Shifted elements begin here
     *                v  v
     * [0][1][2]. . .[8][7][8]. . .[c][d][e][f][ ][ ][ ]. . .
     *  0  1  2       7  8  9       d  e  f (Expanded space)
     * ```
     */
    uint64_t* probe = NULL;

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i < insertion_index)
        {
            assert(*probe == (uint64_t){i}); // Unchanged
        }
        else if (i == insertion_index)
        {
            assert(*probe == element); // Inserted
        }
        else
        {
            assert(*probe == (uint64_t){i - 1}); // Shifted
        }
    }

    Vec_destroy(&v);
}

static void test_insert_full_inner_pointer_middle_after_insertion_struct(void)
{
    /*
     * Using a pointer, `target`, to a `Fish` inside the not-full vector
     *
     * ```
     *                Will insert here
     *                |     target
     *                v     v
     * [0][1][2]. . .[7][8][9]. . .[d][e][f]
     *  0  1  2       7  8  9       d  e  f
     * ```
     *
     * insert a copy of that `Fish` among the others (denoted by their sizes).
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = new_full_Fish_vec(16);

    // Get a pointer to the element.
    size_t const target_index = (Vec_count(v) / 2) + 1;
    size_t const insertion_index = target_index - 2;
    Fish const element = index_to_Fish(target_index);
    Fish* target = (Fish*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(target->color == element.color);
    assert(target->size == element.size);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);
    size_t const prev_cap = Vec_capacity(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.
    assert(Vec_capacity(v) > prev_cap); // The vector was full, so it expanded.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *                Inserted here
     *                |  Shifted elements begin here
     *                v  v
     * [0][1][2]. . .[9][7][8]. . .[c][d][e][f][ ][ ][ ]. . .
     *  0  1  2       7  8  9       d  e  f (Expanded space)
     * ```
     */
    Fish* probe = NULL;

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        probe = (Fish*) Vec_get(v, i);
        assert(probe != NULL);

        if (i < insertion_index)
        {
            // Unchanged
            Fish const f = index_to_Fish(i);

            assert(probe->color == f.color);
            assert(probe->size == f.size);
        }
        else if (i == insertion_index)
        {
            // Inserted
            assert(probe->color == element.color);
            assert(probe->size == element.size);
        }
        else
        {
            // Shifted
            Fish const f = index_to_Fish(i - 1);

            assert(probe->color == f.color);
            assert(probe->size == f.size);
        }
    }

    Vec_destroy(&v);
}

static void test_insert_full_inner_pointer_tail_before_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *                                    target
     *                                    |  Will insert here
     *                                    v  v
     * [0][1][2]. . .[8][9][a]. . .[d][e][f][ ]
     *  0  1  2       8  9  a       d  e  f  ^
     *                                       One past the end
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = new_full_u64_vec(16);

    // Get a pointer to the element.
    uint64_t const element = Vec_count(v) - 1; // Assuming elements are indices
    size_t const target_index = Vec_count(v) - 1;
    size_t const insertion_index = target_index + 1;
    uint64_t* target = (uint64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);
    size_t const prev_cap = Vec_capacity(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.
    assert(Vec_capacity(v) > prev_cap); // The vector was full, so it expanded.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *                                       Inserted here
     *                                       v
     * [0][1][2]. . .[8][9][a]. . .[d][e][f][f][ ][ ][ ]. . .
     *  0  1  2       8  9  a       d  e  f (Expanded space)
     * ```
     */
    uint64_t* probe = NULL;

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i < insertion_index)
        {
            assert(*probe == (uint64_t){i}); // Unchanged
        }
        else if (i == insertion_index)
        {
            assert(*probe == element); // Inserted
        }
        else
        {
            assert(false); // There should be no elements past the insertion.
        }
    }

    Vec_destroy(&v);
}

static void test_insert_full_inner_pointer_tail_at_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *                             target   Will insert here
     *                                   \ /
     *                                    |
     *                                    v
     * [0][1][2]. . .[8][9][a]. . .[d][e][f]
     *  0  1  2       8  9  a       d  e  f
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = new_full_u64_vec(16);

    // Get a pointer to the element.
    uint64_t const element = Vec_count(v) - 1; // Assuming elements are indices
    size_t const target_index = Vec_count(v) - 1;
    size_t const insertion_index = target_index;
    uint64_t* target = (uint64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);
    size_t const prev_cap = Vec_capacity(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.
    assert(Vec_capacity(v) > prev_cap); // The vector was full, so it expanded.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *                                    Inserted here
     *                                    v
     * [0][1][2]. . .[8][9][a]. . .[d][e][f][f][ ][ ][ ]. . .
     *  0  1  2       8  9  a       d  e  f (Expanded space)
     * ```
     */
    uint64_t* probe = NULL;

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i < insertion_index)
        {
            assert(*probe == (uint64_t){i}); // Unchanged
        }
        else if (i == insertion_index)
        {
            assert(*probe == element); // Inserted
        }
        else if (i == insertion_index + 1)
        {
            assert(*probe == element); // Shifted original element
        }
        else
        {
            // There should be no elements past the shifted original element.
            assert(false);
        }
    }

    Vec_destroy(&v);
}

static void test_insert_full_inner_pointer_tail_after_insertion(void)
{
    /*
     * Using a pointer, `target`, to an element inside the not-full vector
     *
     * ```
     *                                 Will insert here
     *                                 |  target
     *                                 v  v
     * [0][1][2]. . .[8][9][a]. . .[d][e][f]
     *  0  1  2       8  9  a       d  e  f
     * ```
     *
     * insert a copy of that element.
     *
     * Before insertion, all the elements of the vector are unique. This is so
     * that, should the memory behind `target` shift before insertion, this test
     * verifies that the correct element (the element initially behind `target`)
     * gets inserted.
     */
    Vec* v = new_full_u64_vec(16);

    // Get a pointer to the element.
    uint64_t const element = Vec_count(v) - 1; // Assuming elements are indices
    size_t const target_index = Vec_count(v) - 1;
    size_t const insertion_index = target_index - 1;
    uint64_t* target = (uint64_t*) Vec_get(v, target_index);

    assert(target != NULL);
    assert(*target == element);

    // Insert a copy of the element via a pointer to itself.
    size_t const prev_count = Vec_count(v);
    size_t const prev_cap = Vec_capacity(v);

    assert(true == Vec_insert(v, insertion_index, target, sizeof(*target)));
    target = NULL; // Pointer invalidated by insertion
    assert(Vec_count(v) == prev_count + 1); // An element was inserted.
    assert(Vec_capacity(v) > prev_cap); // The vector was full, so it expanded.

    /*
     * After insertion, the vector should look like this:
     *
     * ```
     *                                 Inserted here
     *                                 v
     * [0][1][2]. . .[8][9][a]. . .[d][f][e][f][ ][ ][ ]. . .
     *  0  1  2       8  9  a       d  e  f (Expanded space)
     * ```
     */
    uint64_t* probe = NULL;

    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);

        if (i < insertion_index)
        {
            assert(*probe == (uint64_t){i}); // Unchanged
        }
        else if (i == insertion_index)
        {
            assert(*probe == element); // Inserted
        }
        else if (i == insertion_index + 1)
        {
            assert(*probe == element - 1); // Shifted penultimate element (`e`)
        }
        else if (i == insertion_index + 2)
        {
            // Shifted original tail element (`f`)
            assert(*probe == element);
        }
        else
        {
            // There should be no elements past the shifted tail element.
            assert(false);
        }
    }

    Vec_destroy(&v);
}

static void test_remove_invalid(void)
{
    Vec* v = Vec_new(5, sizeof(uint64_t));

    assert(v != NULL);

    /*
     * Remove from an out of bounds index.
     *
     * Since there is no element at that index, we should get back the index to
     * one-past the last element. Since there are NO elements at all, the
     * one-past the last index is 0.
     */
    assert(Vec_capacity(v) < 12345);
    assert(Vec_remove(v, 12345) == 0);

    /*
     * Remove from an in bounds index where there's no element. Since the vector
     * is empty, that's any in bounds index.
     */
    assert(Vec_count(v) == 0);
    assert(Vec_remove(v, 0) == 0);
    assert(Vec_capacity(v) > 3);
    assert(Vec_remove(v, 1) == 0);
    assert(Vec_remove(v, 2) == 0);
    assert(Vec_remove(v, 3) == 0);

    // Remove from a null vector.
    assert(Vec_remove(NULL, 0) == 0);

    Vec_destroy(&v);
}

static void test_remove_middle(void)
{
    Vec* v = new_full_u64_vec(10); // Get a full vector.

    /*
     * Remove something from the middle of the vector.
     *
     * ```
     *                 Will remove from here
     *                 v
     * [0][1][2][3][4][5][6][7][8][9]. . .[x]
     *  0  1  2  3  4  5  6  7  8  9       ^
     *                                     Initial capacity
     * ```
     */
    size_t const initial_count = Vec_count(v);
    size_t removed_element_index = 5;

    // First, note the element following the one we're about to remove.
    uint64_t following_pre_removal = 0;

    {
        /*
         * Also note that we have to store it as a value, briefly using a
         * pointer in this nested scope for null checking when fetching the
         * value.
         *
         * The pointer will be invalidated after we do the removal since the
         * removal will cause elements to shift, changing what the pointer's
         * pointing at!
         */
        uint64_t* following_pre_ptr = NULL; // WARNING: Will be invalidated!

        following_pre_ptr = (uint64_t*) Vec_get(v, (removed_element_index + 1));
        assert(following_pre_ptr != NULL);
        following_pre_removal = *following_pre_ptr;
    }

    /*
     * Now remove the element, receiving the index to the element after the one
     * we removed.
     */
    size_t const following_index = Vec_remove(v, removed_element_index);

    assert(Vec_count(v) == (initial_count - 1)); // An element was removed.

    // The received index corresponds to the following element we noted earlier.
    uint64_t* following_post_removal = NULL;

    following_post_removal = (uint64_t*) Vec_get(v, following_index);
    assert(following_post_removal != NULL);
    assert(*following_post_removal == following_pre_removal);

    /*
     * The vector now looks like this:
     *
     * ```
     *                 Removed from here
     *                 v
     * [0][1][2][3][4][6][7][8][9]. . .[x][ ]
     *  0  1  2  3  4  5  6  7  8          ^
     *                                     Initial capacity
     * ```
     */
    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        uint64_t* probe = NULL;

        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);
        if (i < removed_element_index)
        {
            // The elements before the removal site are still `i`.
            assert(*probe == i);
        }
        else
        {
            // Those at and past the removal site are now `i + 1`.
            assert(*probe == (i + 1));
        }
    }

    Vec_destroy(&v);
}

static void test_remove_tail(void)
{
    Vec* v = new_full_u64_vec(10); // Get a full vector.

    /*
     * Remove the last element of the vector.
     *
     * ```
     *                                            Will remove from here
     *                                            v
     * [0][1][2][3][4][5][6][7][8][9]. . .[x - 1][x]
     *  0  1  2  3  4  5  6  7  8  9              ^
     *                                            Initial capacity
     * ```
     */
    size_t const initial_count = Vec_count(v);
    size_t last_index_pre_removal = initial_count - 1;

    // Remove the element, receiving the index one past the last index.
    assert(Vec_remove(v, last_index_pre_removal) == Vec_count(v));
    assert(Vec_count(v) == (initial_count - 1)); // An element was removed.

    /*
     * The vector now looks like this:
     *
     * ```
     *                                         Removed from here
     *                                         v
     * [0][1][2][3][4][6][7][8][9]. . .[x - 1][ ]
     *  0  1  2  3  4  5  6  7  8              ^
     *                                         Initial capacity
     * ```
     */
    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        uint64_t* probe = NULL;

        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);
        // The elements before the removal site are still `i`.
        assert(*probe == i);
        // And we never even reach the removal site since it's past the end!
        assert(i < last_index_pre_removal);
    }

    Vec_destroy(&v);
}

static void test_remove_head(void)
{
    Vec* v = new_full_u64_vec(10); // Get a full vector.

    /*
     * Remove something from the middle of the vector.
     *
     * ```
     *  Will remove from here
     *  v
     * [0][1][2][3][4][5][6][7][8][9]. . .[x]
     *  0  1  2  3  4  5  6  7  8  9       ^
     *                                     Initial capacity
     * ```
     */
    size_t const initial_count = Vec_count(v);
    size_t removed_element_index = 0;

    /*
     * Note the element following the one we're about to remove.
     *
     * We have to store the element as a value because the pointer will be
     * invalidated after we do the removal since the removal will cause elements
     * to shift, changing what the pointer's pointing at!
     */
    uint64_t following_pre_removal = 0;
    uint64_t* probe = (uint64_t*) Vec_get(v, (removed_element_index + 1));

    assert(probe != NULL);
    following_pre_removal = *probe;

    /*
     * Now remove the element, receiving the index to the element after the one
     * we removed.
     */
    size_t const following_index = Vec_remove(v, removed_element_index);

    probe = NULL; // Pointer invalidated by the removal
    assert(0 == following_index); // The element after should be the new head.
    assert(Vec_count(v) == (initial_count - 1)); // An element was removed.

    // The received index corresponds to the following element we noted earlier.
    probe = (uint64_t*) Vec_get(v, following_index);
    assert(probe != NULL);
    assert(*probe == following_pre_removal);

    /*
     * The vector now looks like this:
     *
     * ```
     *  Removed from here
     *  v
     * [1][2][3][4][5][6][7][8][9]. . .[x][ ]
     *  0  1  2  3  4  5  6  7  8          ^
     *                                     Initial capacity
     * ```
     */
    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        uint64_t* probe = NULL;

        probe = (uint64_t*) Vec_get(v, i);
        assert(probe != NULL);
        // The elements are all now `i + 1`.
        assert(*probe == (i + 1));
    }

    Vec_destroy(&v);
}

static void test_remove_until_empty(void)
{
    Vec* v = new_full_u64_vec(10); // Get a full vector.

    // Starting at the first index, remove an element until none remain.
    size_t i = 0;

    while (i < Vec_count(v))
    {
        /*
         * `i` will be set to the index of the next element to remove.
         *
         * Since we started at the first index, `i` will be equal to the number
         * of elements when there are no elements left.
         */
        i = Vec_remove(v, i);
    }
    assert(Vec_count(v) == 0); // The vector is empty.

    Vec_destroy(&v);
}

/**
 * @brief A `Fish` comparator to be used by a vector's equality function that
 * looks only at fish color
 * @param fish_a A `Fish`
 * @param fish_b Another `Fish`
 * @return 1 if there was an input problem; 0 if the given `Fish` have the same
 * color; 2 if they have different colors
 */
static int fish_color_comparator(void const* fish_a, void const* fish_b)
{
    if (fish_a == NULL ||
        fish_b == NULL)
    {
        return 1;
    }

    // We know we're dealing with `Fish`; cast to the known type.
    Fish const* a = (Fish const*) fish_a;
    Fish const* b = (Fish const*) fish_b;

    if (a->color == b->color)
    {
        return 0;
    }

    return 2;
}

static void test_equal_invalid(void)
{
    Vec* v = Vec_new(5, sizeof(Fish));

    assert(v != NULL);

    /*
     * The equality function fails and returns false if either vector pointer is
     * null (but the comparator pointer is optional).
     */
    assert(false == Vec_equal(v, NULL, fish_color_comparator));
    assert(false == Vec_equal(v, NULL, NULL));

    assert(false == Vec_equal(NULL, v, fish_color_comparator));
    assert(false == Vec_equal(NULL, v, NULL));

    assert(false == Vec_equal(NULL, NULL, fish_color_comparator));
    assert(false == Vec_equal(NULL, NULL, NULL));

    Vec_destroy(&v);
}

static void test_equal_unmodified_default_comparator(void)
{
    // Make two identical vectors.
    Vec* v1 = Vec_new(15, sizeof(uint16_t));
    Vec* v2 = Vec_new(15, sizeof(uint16_t));

    assert(v1 != NULL);
    assert(v2 != NULL);

    assert(true == Vec_append(v1, &(uint16_t){42}, sizeof(uint16_t)));
    assert(true == Vec_append(v1, &(uint16_t){43}, sizeof(uint16_t)));
    assert(true == Vec_append(v1, &(uint16_t){44}, sizeof(uint16_t)));

    assert(true == Vec_append(v2, &(uint16_t){42}, sizeof(uint16_t)));
    assert(true == Vec_append(v2, &(uint16_t){43}, sizeof(uint16_t)));
    assert(true == Vec_append(v2, &(uint16_t){44}, sizeof(uint16_t)));

    assert(Vec_count(v1) == 3);
    assert(Vec_count(v2) == 3);
    assert(Vec_capacity(v1) == Vec_capacity(v2));

    // Verify that they are considered equal.
    assert(true == Vec_equal(v1, v2, NULL));

    Vec_destroy(&v1);
    Vec_destroy(&v2);
}

static void test_equal_unmodified_custom_comparator(void)
{
    // Make two vectors that have the same elements but different capacities.
    Vec* v1 = Vec_new(1, sizeof(Fish));
    Vec* v2 = Vec_new(1000, sizeof(Fish));

    assert(v1 != NULL);
    assert(v2 != NULL);

    assert(true == Vec_append(v1,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 1
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(v1,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 1
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(v1,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 2
                              },
                              sizeof(Fish)));

    assert(true == Vec_append(v2,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 1
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(v2,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 1
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(v2,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 2
                              },
                              sizeof(Fish)));

    assert(Vec_count(v1) == 3);
    assert(Vec_count(v2) == 3);
    assert(Vec_capacity(v1) != Vec_capacity(v2));

    // Verify they're are considered equal (since they have the same elements).
    assert(true == Vec_equal(v1, v2, fish_color_comparator));

    Vec_destroy(&v1);
    Vec_destroy(&v2);
}

static void test_equal_modified(void)
{
    // Make two vectors that have all but one element in common.
    Vec* v1 = Vec_new(3, sizeof(Fish));
    Vec* v2 = Vec_new(3, sizeof(Fish));

    assert(v1 != NULL);
    assert(v2 != NULL);

    // Two empty vectors with the same element size are considered equal.
    assert(true == Vec_equal(v1, v2, NULL));
    assert(true == Vec_equal(v1, v2, fish_color_comparator));

    Fish const f1 =
    {
        .color = RED,
        .size = 1
    };
    Fish const f2 =
    {
        .color = GREEN,
        .size = 2
    };
    Fish const f3 =
    {
        .color = BLUE,
        .size = 3
    };

    Vec_append(v1, &f1, sizeof(f1));
    Vec_append(v1, &f2, sizeof(f2));
    Vec_append(v1, &f3, sizeof(f3));

    Vec_append(v2, &f1, sizeof(f1));
    Vec_append(v2, &f2, sizeof(f2));
    // Last element missing!

    assert(Vec_count(v1) > Vec_count(v2));
    assert(false == Vec_equal(v1, v2, fish_color_comparator));

    // After adding the missing element, the vectors should report as equal.
    Vec_append(v2, &f3, sizeof(f3));

    assert(Vec_count(v1) == Vec_count(v2));
    assert(true == Vec_equal(v1, v2, fish_color_comparator));

    // After inserting a new element into one vector, they should be unequal.
    size_t const index = 0;
    Fish inserted =
    {
        .color = RED,
        .size = 2
    };

    assert(true == Vec_insert(v1, index, &inserted, sizeof(inserted)));

    assert(Vec_count(v1) > Vec_count(v2));
    assert(false == Vec_equal(v1, v2, fish_color_comparator));

    /*
     * After inserting the element in the same place in the other vector, they
     * should be equal.
     */
    assert(true == Vec_insert(v2, index, &inserted, sizeof(inserted)));

    assert(Vec_count(v1) == Vec_count(v2));
    assert(true == Vec_equal(v1, v2, fish_color_comparator));

    // Removing an element from one vector makes the vectors unequal.
    size_t const last_index = Vec_count(v1) - 1;

    Vec_remove(v1, last_index);

    assert(Vec_count(v1) < Vec_count(v2));
    assert(false == Vec_equal(v1, v2, fish_color_comparator));

    // Removing the same element in the other vector, they should be equal.
    Vec_remove(v2, last_index);

    assert(Vec_count(v1) == Vec_count(v2));
    assert(true == Vec_equal(v1, v2, fish_color_comparator));

    // Add elements to the first vector until it resizes.
    size_t old_cap = Vec_capacity(v1);
    Fish const filler =
    {
        .color = GREEN,
        .size = 1
    };

    while (Vec_capacity(v1) == old_cap)
    {
        assert(true == Vec_append(v1, &filler, sizeof(filler)));
    }
    assert(false == Vec_equal(v1, v2, fish_color_comparator)); // Now unequal.

    // Add the same elements to the other vector until it resizes.
    old_cap = Vec_capacity(v2);

    while (Vec_capacity(v2) == old_cap)
    {
        assert(true == Vec_append(v2, &filler, sizeof(filler)));
    }
    assert(true == Vec_equal(v1, v2, fish_color_comparator)); // Now equal.

    // Remove elements from the first vector until it's empty.
    while (Vec_count(v1) > 0)
    {
        Vec_remove(v1, 0);
    }
    assert(Vec_count(v1) == 0);
    assert(false == Vec_equal(v1, v2, fish_color_comparator)); // Now unequal.

    // Remove elements from the other vector until it's empty.
    while (Vec_count(v2) > 0)
    {
        Vec_remove(v2, 0);
    }
    assert(Vec_count(v2) == 0);
    assert(true == Vec_equal(v1, v2, fish_color_comparator)); // Now equal.

    Vec_destroy(&v1);
    Vec_destroy(&v2);
}

static void test_equal_empty_same_element_size(void)
{
    // Make two empty vectors that take elements of the same size (`Fish`).
    Vec* v1 = Vec_new(1, sizeof(Fish));
    Vec* v2 = Vec_new(1, sizeof(Fish));

    assert(v1 != NULL);
    assert(v2 != NULL);

    /*
     * The vectors should be empty, and, since both take elements of the same
     * size, they should be equal (regardless of what comparator is used).
     */
    assert(Vec_count(v1) == 0);
    assert(Vec_count(v2) == Vec_count(v1));
    assert(true == Vec_equal(v1, v2, NULL));
    assert(true == Vec_equal(v1, v2, fish_color_comparator));

    // Add an element to one of the vectors.
    Fish const dory = (Fish)
    {
        .color = BLUE,
        .size = 2
    };

    assert(true == Vec_append(v1,
                              &dory,
                              sizeof(Fish)));

    // The vectors are no longer equal.
    assert(Vec_count(v1) == 1);
    assert(Vec_count(v2) != Vec_count(v1));
    assert(false == Vec_equal(v1, v2, NULL));
    assert(false == Vec_equal(v1, v2, fish_color_comparator));

    // Remove the element.
    assert(Vec_get(v1, 0) != NULL);
    assert(Vec_remove(v1, 0) == 0);

    // Both vectors should be empty and equal again.
    assert(Vec_count(v1) == 0);
    assert(Vec_count(v2) == Vec_count(v1));
    assert(true == Vec_equal(v1, v2, NULL));
    assert(true == Vec_equal(v1, v2, fish_color_comparator));

    Vec_destroy(&v1);
    Vec_destroy(&v2);
}

static void test_equal_empty_diff_element_size(void)
{
    // Make two empty vectors that take elements of the different sizes.
    Vec* v1 = Vec_new(1, sizeof(uint16_t));
    Vec* v2 = Vec_new(1, sizeof(uint32_t));

    assert(v1 != NULL);
    assert(v2 != NULL);

    /*
     * The vectors should be empty, and, since both take elements of different
     * sizes, they should NOT be equal (regardless of what comparator is used).
     */
    assert(Vec_count(v1) == 0);
    assert(Vec_count(v2) == Vec_count(v1));
    assert(false == Vec_equal(v1, v2, NULL));
    assert(false == Vec_equal(v1, v2, fish_color_comparator));

    Vec_destroy(&v1);
    Vec_destroy(&v2);
}

static void test_remove_all_invalid(void)
{
    Vec* v = Vec_new(10, sizeof(uint8_t));

    assert(v != NULL);

    // Try removing an element not matching the size expected by the vector.
    assert(0 == Vec_remove_all(v, &(uint8_t){5}, sizeof(uint8_t)));
    assert(0 == Vec_remove_all(v, &(uint8_t){5}, 0));

    // Try removing with null pointers.
    assert(0 == Vec_remove_all(NULL, &(uint8_t){5}, sizeof(uint8_t)));
    assert(0 == Vec_remove_all(v, NULL, sizeof(uint8_t)));
    assert(0 == Vec_remove_all(NULL, NULL, sizeof(uint8_t)));

    Vec_destroy(&v);
}

static void test_remove_all_none(void)
{
    /*
     * Remove an element that doesn't match any element in the vector. So from
     *
     * ```
     * [y][y][y]
     *  0  1  2
     * ```
     *
     * we'll try removing all instances of `X` (of which there are none).
     */
    Vec* v = Vec_new(10, sizeof(int));

    // Removing all from an empty vector is fine but does nothing.
    assert(v != NULL);
    assert(0 == Vec_remove_all(v, &(uint8_t){5}, sizeof(uint8_t)));
    assert(0 == Vec_count(v));

    // So add some positive integers...
    assert(true == Vec_append(v, &(int){0}, sizeof(int)));
    assert(true == Vec_append(v, &(int){1}, sizeof(int)));
    assert(true == Vec_append(v, &(int){2}, sizeof(int)));
    assert(Vec_count(v) == 3);

    // ...and try to remove all integers that are -2 (when there aren't any).
    assert(0 == Vec_remove_all(v, &(int){-2}, sizeof(int)));
    assert(Vec_count(v) == 3); // No elements were removed.

    Vec_destroy(&v);
}

static void test_remove_all_one(void)
{
    /*
     * Remove an element that only matches one element in the vector. So from
     *
     * ```
     * [9][8][7][6]
     *  0  1  2  3
     * ```
     *
     * we'll try removing all instances of `9` (of which there is one, at the
     * very beginning).
     */
    Vec* v = Vec_new(5, sizeof(uint64_t));
    Vec* expected = NULL;

    assert(v != NULL);
    assert(true == Vec_append(v, &(uint64_t){9}, sizeof(uint64_t)));
    assert(true == Vec_append(v, &(uint64_t){8}, sizeof(uint64_t)));
    assert(true == Vec_append(v, &(uint64_t){7}, sizeof(uint64_t)));
    assert(true == Vec_append(v, &(uint64_t){6}, sizeof(uint64_t)));
    assert(Vec_count(v) == 4);

    // Try to remove all elements that match `9`.
    assert(1 == Vec_remove_all(v, &(uint64_t){9}, sizeof(uint64_t)));
    assert(Vec_count(v) == 3); // One element was removed.

    /*
     * The vector now looks like this:
     *
     * ```
     * [8][7][6][ ]
     *  0  1  2  3
     * ```
     */
    expected = Vec_new(3, sizeof(uint64_t));
    assert(expected != NULL);
    assert(true == Vec_append(expected, &(uint64_t){8}, sizeof(uint64_t)));
    assert(true == Vec_append(expected, &(uint64_t){7}, sizeof(uint64_t)));
    assert(true == Vec_append(expected, &(uint64_t){6}, sizeof(uint64_t)));
    assert(Vec_count(expected) == 3);

    assert(true == Vec_equal(v, expected, NULL));
    Vec_destroy(&expected);

    /*
     * Next, remove one element from the middle, making the vector look like:
     *
     * ```
     * [8][6][ ][ ]
     *  0  1  2  3
     * ```
     */
    assert(1 == Vec_remove_all(v, &(uint64_t){7}, sizeof(uint64_t)));
    assert(Vec_count(v) == 2); // One element was removed.

    expected = Vec_new(2, sizeof(uint64_t));
    assert(expected != NULL);
    assert(true == Vec_append(expected, &(uint64_t){8}, sizeof(uint64_t)));
    assert(true == Vec_append(expected, &(uint64_t){6}, sizeof(uint64_t)));
    assert(Vec_count(expected) == 2);

    assert(true == Vec_equal(v, expected, NULL));
    Vec_destroy(&expected);

    /*
     * Next, remove one element from the end, making the vector look like:
     *
     * ```
     * [8][ ][ ][ ]
     *  0  1  2  3
     * ```
     */
    assert(1 == Vec_remove_all(v, &(uint64_t){6}, sizeof(uint64_t)));
    assert(Vec_count(v) == 1); // One element was removed.

    expected = Vec_new(1, sizeof(uint64_t));
    assert(expected != NULL);
    assert(true == Vec_append(expected, &(uint64_t){8}, sizeof(uint64_t)));
    assert(Vec_count(expected) == 1);

    assert(true == Vec_equal(v, expected, NULL));
    Vec_destroy(&expected);

    Vec_destroy(&v);
}

static void test_remove_all_partial(void)
{
    /*
     * Remove an element that matches several elements in the vector. So from
     *
     * ```
     * [X][X][a][X][X][X][b][X][c][X]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * we'll try removing all instances of `X`.
     */
    Vec* v = Vec_new(10, sizeof(uint64_t));
    uint64_t const a = 1;
    uint64_t const b = 2;
    uint64_t const c = 3;
    uint64_t const X = 4;

    assert(v != NULL);
    assert(true == Vec_append(v, &X, sizeof(X)));
    assert(true == Vec_append(v, &X, sizeof(X)));
    assert(true == Vec_append(v, &a, sizeof(a))); // a
    assert(true == Vec_append(v, &X, sizeof(X)));
    assert(true == Vec_append(v, &X, sizeof(X)));
    assert(true == Vec_append(v, &X, sizeof(X)));
    assert(true == Vec_append(v, &b, sizeof(b))); // b
    assert(true == Vec_append(v, &X, sizeof(X)));
    assert(true == Vec_append(v, &c, sizeof(c))); // c
    assert(true == Vec_append(v, &X, sizeof(X)));
    assert(Vec_count(v) == 10);

    uint64_t* probe = Vec_get(v, 0);

    assert(probe != NULL);
    assert(*probe == X);

    // Remove all elements that match `X`.
    assert(7 == Vec_remove_all(v, probe, sizeof(*probe)));
    probe = NULL; // Pointer invalidated by the removal
    assert(Vec_count(v) == 3); // No `X`s remain.

    /*
     * The vector should now look like this:
     *
     * ```
     * [a][b][c][ ][ ][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    Vec* expected = Vec_new(3, sizeof(uint64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &a, sizeof(a)));
    assert(true == Vec_append(expected, &b, sizeof(b)));
    assert(true == Vec_append(expected, &c, sizeof(c)));
    assert(Vec_count(expected) == 3);

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_remove_all_except_one(void)
{
    /*
     * Remove all elements except one. So from
     *
     * ```
     * [X][X][X][X][a]
     *  0  1  2  3  4
     * ```
     *
     * we'll try removing all instances of `X`.
     */
    Vec* v = Vec_new(5, sizeof(uint64_t));
    uint64_t const X = 1;
    uint64_t const a = 2;

    assert(v != NULL);
    for (int i = 0; i < 4; ++i)
    {
        assert(true == Vec_append(v, &X, sizeof(X)));
    }
    assert(true == Vec_append(v, &a, sizeof(a)));
    assert(5 == Vec_count(v));

    // Remove all `X`s.
    assert(4 == Vec_remove_all(v, &X, sizeof(X)));

    // No `X`s remain, leaving only one element.
    assert(1 == Vec_count(v));

    uint64_t* probe = (uint64_t*) Vec_get(v, 0);

    assert(probe != NULL);
    assert(*probe == a);

    Vec_destroy(&v);
}

static void test_remove_all_total(void)
{
    /*
     * Remove all elements. So from
     *
     * ```
     * [X][X][X][X][X][X][X][X][X][X]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * we'll try removing all instances of `X` (i.e., all the elements).
     */
    Vec* v = Vec_new(10, sizeof(uint64_t));

    for (int i = 0; i < 10; ++i)
    {
        assert(true == Vec_append(v, &(uint64_t){6}, sizeof(uint64_t)));
    }
    assert(10 == Vec_count(v));

    // Remove all `X`s.
    assert(10 == Vec_remove_all(v, &(uint64_t){6}, sizeof(uint64_t)));

    // No `X`s remain.
    assert(0 == Vec_count(v));

    Vec_destroy(&v);
}

static Vec* remove_all_partial_inner_pointer_input_helper(int64_t const X,
                                                          int64_t const y)
{
    /*
     * Create the vector
     *
     * ```
     * [X][X][y][X][X][X][y][y][X][X]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * using the given `X` and `y` parameters.
     */
    Vec* v = Vec_new(10, sizeof(int64_t));

    assert(v != NULL);
    assert(true == Vec_append(v, &X, sizeof(X))); // 0
    assert(true == Vec_append(v, &X, sizeof(X))); // 1
    assert(true == Vec_append(v, &y, sizeof(y))); // 2
    assert(true == Vec_append(v, &X, sizeof(X))); // 3
    assert(true == Vec_append(v, &X, sizeof(X))); // 4
    assert(true == Vec_append(v, &X, sizeof(X))); // 5
    assert(true == Vec_append(v, &y, sizeof(y))); // 6
    assert(true == Vec_append(v, &y, sizeof(y))); // 7
    assert(true == Vec_append(v, &X, sizeof(X))); // 8
    assert(true == Vec_append(v, &X, sizeof(X))); // 9
    assert(10 == Vec_count(v));

    return v;
}

static Vec* remove_all_partial_inner_pointer_expected_helper(int64_t const y)
{
    /*
     * Create the vector
     *
     * ```
     * [y][y][y][ ][ ][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * using the given `y` parameter.
     */
    Vec* expected = Vec_new(10, sizeof(int64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &y, sizeof(y))); // 0
    assert(true == Vec_append(expected, &y, sizeof(y))); // 1
    assert(true == Vec_append(expected, &y, sizeof(y))); // 2
    assert(3 == Vec_count(expected));

    return expected;
}

static void test_remove_all_partial_inner_pointer_to_head(void)
{
    /*
     * Remove all instances of `X` using a pointer, `p`, to the first element
     * inside the vector:
     *
     * ```
     *  p
     *  v
     * [X][X][y][X][X][X][y][y][X][X]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    int64_t const X = 6;
    int64_t const y = 7;
    Vec* v = remove_all_partial_inner_pointer_input_helper(X, y);

    // Get a pointer to the first element.
    int64_t* p = (int64_t*) Vec_get(v, 0);

    assert(p != NULL);
    assert(*p == X);
    assert(7 == Vec_remove_all(v, p, sizeof(*p)));
    p = NULL; // Pointer invalidated by the removal
    assert(3 == Vec_count(v));

    Vec* expected = remove_all_partial_inner_pointer_expected_helper(y);

    /*
     * With the `X`s removed, the vector should now look like this:
     *
     * ```
     * [y][y][y][ ][ ][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_remove_all_partial_inner_pointer_to_middle(void)
{
    /*
     * Remove all instances of `X` using a pointer, `p`, to an element in the
     * middle of the vector:
     *
     * ```
     *              p
     *              v
     * [X][X][y][X][X][X][y][y][X][X]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    int64_t const X = 6;
    int64_t const y = 7;
    Vec* v = remove_all_partial_inner_pointer_input_helper(X, y);

    // Get a pointer to a middle element.
    int64_t* p = (int64_t*) Vec_get(v, 4);

    assert(p != NULL);
    assert(*p == X);
    assert(7 == Vec_remove_all(v, p, sizeof(*p)));
    p = NULL; // Pointer invalidated by the removal
    assert(3 == Vec_count(v));

    Vec* expected = remove_all_partial_inner_pointer_expected_helper(y);

    /*
     * With the `X`s removed, the vector should now look like this:
     *
     * ```
     * [y][y][y][ ][ ][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_remove_all_partial_inner_pointer_to_tail(void)
{
    /*
     * Remove all instances of `X` using a pointer, `p`, to the last element in
     * the vector:
     *
     * ```
     *                             p
     *                             v
     * [X][X][y][X][X][X][y][y][X][X]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    int64_t const X = 6;
    int64_t const y = 7;
    Vec* v = remove_all_partial_inner_pointer_input_helper(X, y);

    // Get a pointer to the last element.
    int64_t* p = (int64_t*) Vec_get(v, 9);

    assert(p != NULL);
    assert(*p == X);
    assert(7 == Vec_remove_all(v, p, sizeof(*p)));
    p = NULL; // Pointer invalidated by the removal
    assert(3 == Vec_count(v));

    Vec* expected = remove_all_partial_inner_pointer_expected_helper(y);

    /*
     * With the `X`s removed, the vector should now look like this:
     *
     * ```
     * [y][y][y][ ][ ][ ][ ][ ][ ][ ]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     */
    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static Vec* new_full_i64_vec_same_element(size_t const initial_cap_hint,
                                          int64_t const X)
{
    /*
     * Create the vector
     *
     * ```
     * [X][X][X][X][X][. . .
     *  0  1  2  3  4. . . .
     * ```
     *
     * full of elements matching the given `X` parameter and of a capacity
     * greater than or equal to the given capacity hint parameter.
     */
    Vec* v = Vec_new(initial_cap_hint, sizeof(int64_t));

    assert(v != NULL);

    // Fill the vector to capacity.
    size_t const initial_cap = Vec_capacity(v);

    for (size_t i = 0; i < initial_cap; ++i)
    {
        Vec_append(v, &X, sizeof(X));
    }
    assert(Vec_count(v) == initial_cap); // The vector's full.
    assert(Vec_capacity(v) == initial_cap); // The vector didn't expand yet.

    // Every element should be equal to `X`.
    for (size_t i = 0; i < Vec_count(v); ++i)
    {
        int64_t* probe = NULL;

        probe = (int64_t*) Vec_get(v, i);
        assert(probe != NULL);
        assert(*probe == X);
    }

    return v;
}

static void test_remove_all_total_inner_pointer_to_head(void)
{
    /*
     * Remove all instances of `X` using a pointer, `p`, to the first element in
     * a vector full of `X`s:
     *
     * ```
     *  p
     *  v
     * [X][X][X][X][X]
     *  0  1  2  3  4
     * ```
     */
    size_t const cap_hint = 5;
    int64_t const X = 6;
    Vec* v = new_full_i64_vec_same_element(cap_hint, X);

    // Get a pointer to the first element.
    int64_t* p = (int64_t*) Vec_get(v, 0);
    size_t const count = Vec_count(v);

    assert(p != NULL);
    assert(*p == X);
    assert(count == Vec_remove_all(v, p, sizeof(*p)));
    p = NULL; // Pointer invalidated by the removal
    assert(0 == Vec_count(v)); // The vector's now empty.

    // The vector should compare equal against an empty vector.
    Vec* expected = Vec_new(cap_hint, sizeof(int64_t));

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_remove_all_total_inner_pointer_to_middle(void)
{
    /*
     * Remove all instances of `X` using a pointer, `p`, to an element in the
     * middle of a vector full of `X`s:
     *
     * ```
     *        p
     *        v
     * [X][X][X][X][X]
     *  0  1  2  3  4
     * ```
     */
    size_t const cap_hint = 5;
    int64_t const X = 6;
    Vec* v = new_full_i64_vec_same_element(cap_hint, X);

    // Get a pointer to a middle element.
    int64_t* p = (int64_t*) Vec_get(v, 2);
    size_t const count = Vec_count(v);

    assert(p != NULL);
    assert(*p == X);
    assert(count == Vec_remove_all(v, p, sizeof(*p)));
    p = NULL; // Pointer invalidated by the removal
    assert(0 == Vec_count(v)); // The vector's now empty.

    // The vector should compare equal against an empty vector.
    Vec* expected = Vec_new(cap_hint, sizeof(int64_t));

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

static void test_remove_all_total_inner_pointer_to_tail(void)
{
    /*
     * Remove all instances of `X` using a pointer, `p`, to the last element in
     * a vector full of `X`s:
     *
     * ```
     *              p
     *              v
     * [X][X][X][X][X]
     *  0  1  2  3  4
     * ```
     */
    size_t const cap_hint = 5;
    int64_t const X = 6;
    Vec* v = new_full_i64_vec_same_element(cap_hint, X);
    size_t const count = Vec_count(v);

    // Get a pointer to the last element.
    int64_t* p = (int64_t*) Vec_get(v, count - 1);

    assert(p != NULL);
    assert(*p == X);
    assert(count == Vec_remove_all(v, p, sizeof(*p)));
    p = NULL; // Pointer invalidated by the removal
    assert(0 == Vec_count(v)); // The vector's now empty.

    // The vector should compare equal against an empty vector.
    Vec* expected = Vec_new(cap_hint, sizeof(int64_t));

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&v);
    Vec_destroy(&expected);
}

/**
 * @brief A predicate that can be used by a vector's search or removal functions
 * to target non-negative `int64_t`s
 * @param element An element provided by the vector when it calls this function
 * @param element_size The size of the element reported by the vector (usable as
 * a size-based "type check" to make sure the element looks like an `int64_t`)
 */
static bool non_negative(void const* element, size_t const element_size)
{
    /*
     * We know we're supposed to be dealing with `int64_t`s.
     *
     * If this gets used by a vector with a different element size, the wrong
     * element size will be passed in by that vector, allowing us to return
     * early.
     */
    size_t const expected_size = sizeof(int64_t);

    if (NULL == element ||
        element_size != expected_size)
    {
        return false; // Error; return early.
    }

    int64_t const* i = (int64_t const*) element; // Cast to the known type.

    return *i >= 0;
}

static void test_remove_all_if_invalid(void)
{
    Vec* v = Vec_new(5, sizeof(int64_t));

    assert(v != NULL);
    assert(true == Vec_append(v, &(int64_t){-42}, sizeof(int64_t)));
    assert(1 == Vec_count(v));

    // Null pointers should remove no elements.
    assert(0 == Vec_remove_all_if(v, NULL));
    assert(0 == Vec_remove_all_if(NULL, non_negative));
    assert(0 == Vec_remove_all_if(NULL, NULL));
    assert(1 == Vec_count(v)); // No element was removed.

    Vec_destroy(&v);
}

static void test_remove_all_if_none(void)
{
    // Make a vector of all negative elements.
    Vec* v = Vec_new(5, sizeof(int64_t));

    /*
     * Removing all with a predicate from an empty vector is fine but does nothing.
     */
    assert(v != NULL);
    assert(0 == Vec_remove_all_if(v, non_negative));
    assert(0 == Vec_count(v));

    assert(true == Vec_append(v, &(int64_t){-1}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-2}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-3}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-4}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-5}, sizeof(int64_t)));
    assert(5 == Vec_count(v));

    // Remove all non-negative elements (of which there are none).
    assert(0 == Vec_remove_all_if(v, non_negative));
    assert(5 == Vec_count(v)); // No element was removed.

    // Compare with expected vector.
    Vec* expected = Vec_new(5, sizeof(int64_t));

    assert(true == Vec_append(expected, &(int64_t){-1}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){-2}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){-3}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){-4}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){-5}, sizeof(int64_t)));
    assert(5 == Vec_count(expected));

    assert(Vec_equal(v, expected, NULL));

    Vec_destroy(&expected);
    Vec_destroy(&v);
}

static void test_remove_all_if_one(void)
{
    // Make a vector with exactly 1 non-negative element.
    Vec* v = Vec_new(5, sizeof(int64_t));

    assert(v != NULL);
    assert(true == Vec_append(v, &(int64_t){-1}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-2}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-3}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-4}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){5}, sizeof(int64_t))); // Non-neg
    assert(5 == Vec_count(v));

    // Remove all non-negative elements (of which there is exactly one).
    assert(1 == Vec_remove_all_if(v, non_negative));
    assert(4 == Vec_count(v)); // One element was removed.

    // Compare with expected vector.
    Vec* expected = Vec_new(5, sizeof(int64_t));

    assert(true == Vec_append(expected, &(int64_t){-1}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){-2}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){-3}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){-4}, sizeof(int64_t)));
    assert(Vec_count(expected) == Vec_count(v));

    assert(Vec_equal(v, expected, NULL));

    Vec_destroy(&expected);
    Vec_destroy(&v);
}

static void test_remove_all_if_some(void)
{
    /*
     * Remove an element that matches several elements in the vector.
     *
     * So from
     *
     * ```
     * [X][X][a][X][X][X][b][X][c][X][d][X][X]
     *  0  1  2  3  4  5  6  7  8  9 10 11 12
     * ```
     *
     * we'll try removing all instances of `X`, leaving 4 elements.
     */
    size_t const total = 13;
    size_t const remaining = 4;
    Vec* v = Vec_new(total, sizeof(int64_t));

    assert(v != NULL);
    // The `X` will be any non-negative element.
    assert(true == Vec_append(v, &(int64_t){100}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){200}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-200}, sizeof(int64_t))); // `a`
    assert(true == Vec_append(v, &(int64_t){300}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){400}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){500}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-1}, sizeof(int64_t))); // `b`
    assert(true == Vec_append(v, &(int64_t){600}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-42}, sizeof(int64_t))); // `c`
    assert(true == Vec_append(v, &(int64_t){700}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-800}, sizeof(int64_t))); // `d`
    assert(true == Vec_append(v, &(int64_t){800}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){900}, sizeof(int64_t)));
    assert(total == Vec_count(v));

    // Remove all `X`s.
    size_t const removed = total - remaining;

    assert(removed == Vec_remove_all_if(v, non_negative));
    assert(Vec_count(v) == remaining); // Only non-`X`s remain.

    // Compare with expected vector.
    Vec* expected = Vec_new(total, sizeof(int64_t));

    assert(true == Vec_append(expected, &(int64_t){-200}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){-1}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){-42}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){-800}, sizeof(int64_t)));
    assert(remaining == Vec_count(expected));

    assert(Vec_equal(v, expected, NULL));

    Vec_destroy(&expected);
    Vec_destroy(&v);
}

static void test_remove_all_if_all(void)
{
    /*
     * Remove all elements.
     *
     * So from
     *
     * ```
     * [X][X][X][X][X][X][X][X][X][X]
     *  0  1  2  3  4  5  6  7  8  9
     * ```
     *
     * remove all instances of `X` (i.e., all the elements).
     */
    size_t const cap_hint = 9000;
    int64_t const X = 1;
    Vec* v = new_full_i64_vec_same_element(cap_hint, X);

    assert(v != NULL);

    // Remove all `X`s.
    size_t const total = Vec_count(v);

    assert(total >= cap_hint);
    assert(total == Vec_remove_all_if(v, non_negative));
    assert(0 == Vec_count(v)); // All elements removed

    Vec_destroy(&v);
}

/**
 * @brief An integer comparator usable for sorting elements in ascending order
 * @param a The first integer
 * @param b The second integer
 * @return A positive `int` if the first integer is bigger than the second, a
 * zero `int` when it's the same size as the second, or a negative `int` when
 * it's smaller than the second or when either integer pointer is null
 */
static int int64_comparator(void const* int64_a, void const* int64_b)
{
    if (int64_a == NULL ||
        int64_b == NULL)
    {
        return -1;
    }

    // We know we're dealing with `int64_t`s; cast to the known type.
    int64_t const* a = (int64_t const*) int64_a;
    int64_t const* b = (int64_t const*) int64_b;

    return (int) (*a - *b);
}

static void test_qsort_invalid(void)
{
    size_t const count = 3;
    Vec* v = Vec_new(count, sizeof(int64_t));
    Vec* expected = Vec_new(count, sizeof(int64_t));

    assert(v != NULL);
    assert(true == Vec_append(v, &(int64_t){5}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-42}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){0}, sizeof(int64_t)));
    assert(count == Vec_count(v));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &(int64_t){5}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){-42}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){0}, sizeof(int64_t)));
    assert(count == Vec_count(expected));

    // Null pointers should not permit sorting.
    assert(false == Vec_qsort(v, NULL));
    assert(false == Vec_qsort(NULL, int64_comparator));
    assert(false == Vec_qsort(NULL, NULL));
    // The vector should be unsorted.
    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&expected);
    Vec_destroy(&v);

    v = Vec_new(count, sizeof(int64_t));

    assert(v != NULL);
    assert(0 == Vec_count(v));

    // Empty vectors should not permit sorting.
    assert(false == Vec_qsort(v, int64_comparator));

    Vec_destroy(&v);
}

static void test_qsort_scalar(void)
{
    size_t const count = 5;
    Vec* v = Vec_new(count, sizeof(int64_t));
    Vec* expected = Vec_new(count, sizeof(int64_t));

    // Sorting empty vectors is fine but does nothing, always returning `false`.
    assert(false == Vec_qsort(v, int64_comparator));

    // Add unsorted elements.
    assert(true == Vec_append(v, &(int64_t){2077}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-666}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){1962}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-5}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){1945}, sizeof(int64_t)));
    assert(Vec_count(v) == count);

    // Add sorted elements.
    assert(expected != NULL);
    assert(true == Vec_append(expected, &(int64_t){-666}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){-5}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){1945}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){1962}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){2077}, sizeof(int64_t)));
    assert(Vec_count(expected) == count);

    assert(false == Vec_equal(v, expected, NULL)); // The elements are unsorted.
    assert(true == Vec_qsort(v, int64_comparator)); // Sort the elements.
    assert(true == Vec_equal(v, expected, NULL)); // The elements are sorted.

    Vec_destroy(&expected);
    Vec_destroy(&v);
}

/**
 * @brief A `Fish` comparator usable for sorting `Fish` by size in ascending
 * order
 * @param fish_a The first `Fish`
 * @param fish_b The second `Fish`
 * @return A positive `int` if the first `Fish` is bigger than the second, a
 * zero `int` when it's the same size as the second, or a negative `int` when
 * it's smaller than the second or when either `Fish` pointer is null
 */
static int fish_size_comparator(void const* fish_a, void const* fish_b)
{
    if (fish_a == NULL ||
        fish_b == NULL)
    {
        return -1;
    }

    // We know we're dealing with `Fish`; cast to the known type.
    Fish const* a = (Fish const*) fish_a;
    Fish const* b = (Fish const*) fish_b;

    return (int) (a->size - b->size);
}

static void test_qsort_struct(void)
{
    size_t const count = 5;
    Vec* v = Vec_new(count, sizeof(Fish));
    Vec* expected = Vec_new(count, sizeof(Fish));

    // Add (size-wise) unsorted elements.
    assert(v != NULL);
    assert(true == Vec_append(v,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 6
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(v,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 1
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(v,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 4
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(v,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 1
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(v,
                              &(Fish)
                              {
                                  .color = BLUE,
                                  .size = 2
                              },
                              sizeof(Fish)));
    assert(Vec_count(v) == count);

    // Add (size-wise) sorted elements.
    assert(expected != NULL);
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 1
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 1
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = BLUE,
                                  .size = 2
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 4
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 6
                              },
                              sizeof(Fish)));
    assert(Vec_count(expected) == count);

    // The elements are unsorted.
    assert(false == Vec_equal(v, expected, fish_size_comparator));
    assert(true == Vec_qsort(v, fish_size_comparator)); // Sort the elements.
    // The elements are sorted.
    assert(true == Vec_equal(v, expected, fish_size_comparator));

    Vec_destroy(&expected);
    Vec_destroy(&v);
}

#define UNUSED(x) (void)(x)

/**
 * @brief Increments the given element by one
 * @param element A pointer to an `int64_t`
 * @param element_size For size-based type checking
 * @param state An unused pointer
 * @return 1 if there was an input problem; 0 otherwise
 */
static int add_one(void* element, size_t const element_size, void* state)
{
    UNUSED(state);
    if (element == NULL ||
        sizeof(int64_t) != element_size) // Explicit size check
    {
        return 1; // Input error
    }

    int64_t* i = (int64_t*) element;

    *i += 1;

    return 0;
}

static void test_apply_invalid(void)
{
    Vec* v = Vec_new(5, sizeof(int));

    assert(v != NULL);

    // Null pointers should cause the function to fail...
    int const failure_flag = 1;

    assert(failure_flag == Vec_apply(v, NULL, NULL));
    assert(failure_flag == Vec_apply(NULL, add_one, NULL));
    assert(failure_flag == Vec_apply(NULL, NULL, NULL));
    // ...regardless of the caller state pointer supplied.
    assert(failure_flag == Vec_apply(v, NULL, &v));
    assert(failure_flag == Vec_apply(NULL, add_one, &v));
    assert(failure_flag == Vec_apply(NULL, NULL, &v));

    Vec_destroy(&v);
}

static void test_apply_modify_scalar(void)
{
    // Make a vector with some scalar elements.
    size_t const count = 5;
    Vec* v = Vec_new(count, sizeof(int64_t));

    /*
     * Applying something on an empty vector is fine but does nothing, returning
     * `1`.
     */
    assert(v != NULL);
    assert(1 == Vec_apply(v, add_one, NULL));
    assert(0 == Vec_count(v));

    assert(true == Vec_append(v, &(int64_t){5}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){6}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){9}, sizeof(int64_t)));
    assert(count == Vec_count(v));

    // Increment all elements by one.
    int const success_flag = 0;

    assert(success_flag == Vec_apply(v, add_one, NULL));

    // Make a vector with the same scalar elements incremented by one.
    Vec* expected = Vec_new(count, sizeof(int64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &(int64_t){6}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){7}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){8}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){10}, sizeof(int64_t)));
    assert(count == Vec_count(expected));

    // The vectors should match.
    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&expected);

    /*
     * If we increment by one three more times (regardless of caller state
     * pointer supplied)...
     */
    assert(success_flag == Vec_apply(v, add_one, NULL));
    assert(success_flag == Vec_apply(v, add_one, &expected));
    assert(success_flag == Vec_apply(v, add_one, &(int){666}));

    /*
     * ...the vector should match one with the original elements incremented by
     * four.
     */
    expected = Vec_new(count, sizeof(int64_t));

    assert(expected != NULL);
    assert(true == Vec_append(expected, &(int64_t){9}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){10}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){11}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){12}, sizeof(int64_t)));
    assert(true == Vec_append(expected, &(int64_t){13}, sizeof(int64_t)));
    assert(count == Vec_count(expected));

    assert(true == Vec_equal(v, expected, NULL));

    Vec_destroy(&expected);
    Vec_destroy(&v);
}

/**
 * @brief Increases the size of the given `Fish` if it's a green one
 * @param element A `Fish`
 * @param element_size For size-based type checking
 * @param state An unused pointer
 * @return 1 if there was an input problem; 0 otherwise
 */
static int grow_green_fish(void* element,
                           size_t const element_size,
                           void* state)
{
    UNUSED(state);
    if (element == NULL ||
        sizeof(Fish) != element_size) // Explicit size check
    {
        return 1; // Input error
    }

    Fish* fish = (Fish*) element;

    if (fish->color == GREEN)
    {
        fish->size += 1;
    }

    return 0;
}

/**
 * @brief A `Fish` comparator to be used by a vector's equality function that
 * considers all the members of a `Fish`
 * @param fish_a A `Fish`
 * @param fish_b Another `Fish`
 * @return 1 if there was an input problem; 0 if the given `Fish` have the same
 * members; 2 if their members differ
 */
static int fish_precise_comparator(void const* fish_a, void const* fish_b)
{
    if (fish_a == NULL ||
        fish_b == NULL)
    {
        return 1;
    }

    // We know we're dealing with `Fish`; cast to the known type.
    Fish const* a = (Fish const*) fish_a;
    Fish const* b = (Fish const*) fish_b;

    if ((a->color == b->color) &&
        (a->size == b->size))
    {
        return 0;
    }

    return 2;
}

static void test_apply_modify_struct(void)
{
    // Make a vector of `Fish`.
    size_t const count = 5;
    Vec* v = Vec_new(count, sizeof(Fish));

    assert(v != NULL);
    assert(true == Vec_append(v,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 6
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(v,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 3
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(v,
                              &(Fish)
                              {
                                  .color = BLUE,
                                  .size = 4
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(v,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 10
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(v,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 41
                              },
                              sizeof(Fish)));
    assert(count == Vec_count(v));

    // Increment the size of all the green `Fish` by one.
    int const success_flag = 0;

    assert(success_flag == Vec_apply(v, grow_green_fish, NULL));

    /*
     * Make a vector of the same `Fish`, with the size of the green ones
     * incremented by one.
     */
    Vec* expected = Vec_new(count, sizeof(Fish));

    assert(expected != NULL);
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 7
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 3
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = BLUE,
                                  .size = 4
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 11
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 42
                              },
                              sizeof(Fish)));
    assert(count == Vec_count(expected));

    // The vectors should match.
    assert(true == Vec_equal(v, expected, fish_precise_comparator));

    Vec_destroy(&expected);

    /*
     * If we increment by one three more times (regardless of caller state
     * pointer supplied)...
     */
    assert(success_flag == Vec_apply(v, grow_green_fish, NULL));
    assert(success_flag == Vec_apply(v, grow_green_fish, &expected));
    assert(success_flag == Vec_apply(v, grow_green_fish, &(int){666}));

    /*
     * ...the vector should match one with the original elements incremented by
     * four.
     */
    expected = Vec_new(count, sizeof(Fish));

    assert(expected != NULL);
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 10
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = RED,
                                  .size = 3
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = BLUE,
                                  .size = 4
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 14
                              },
                              sizeof(Fish)));
    assert(true == Vec_append(expected,
                              &(Fish)
                              {
                                  .color = GREEN,
                                  .size = 45
                              },
                              sizeof(Fish)));
    assert(count == Vec_count(expected));

    assert(true == Vec_equal(v, expected, fish_precise_comparator));

    Vec_destroy(&expected);
    Vec_destroy(&v);
}

/**
 * @brief Records the largest element seen so far
 * @param element An `int64_t`
 * @param element_size For size-based type checking
 * @param state Used to update the largest element seen so far
 * @return 1 if there was an input problem; 0 otherwise
 */
static int store_max(void* element, size_t const element_size, void* state)
{
    if (element == NULL ||
        state == NULL ||
        sizeof(int64_t) != element_size) // Explicit size check
    {
        return 1; // Input error
    }

    int64_t const* i = (int64_t const*) element;
    int64_t* curr_max = (int64_t*) state;

    if (*i > *curr_max)
    {
        *curr_max = *i; // Update largest element so far
    }

    return 0;
}

static void test_apply_state_scalar(void)
{
    // Make a vector with one maximum element.
    int64_t const max_actual = 43;
    Vec* v = Vec_new(10, sizeof(int64_t));

    assert(v != NULL);
    assert(true == Vec_append(v, &(int64_t){5}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-4}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){42}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){1}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){1}, sizeof(int64_t)));
    assert(true == Vec_append(v, &max_actual, sizeof(int64_t))); // Max
    assert(true == Vec_append(v, &(int64_t){0}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){-777}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){39}, sizeof(int64_t)));
    assert(true == Vec_append(v, &(int64_t){4}, sizeof(int64_t)));
    assert(10 == Vec_count(v));

    // Find the maximum value in the vector.
    int64_t min = INT64_MIN;
    int64_t* max = &min; // Initialize to the smallest value to find max
    int const success = 0; // Processed all elements

    assert(success == Vec_apply(v, store_max, max));
    assert(*max == max_actual); // The max value should have been found.

    Vec_destroy(&v);
}

/**
 * @brief Records the number of green `Fish` seen so far
 * @param element A `Fish`
 * @param element_size For size-based type checking
 * @param state Used to update the number of green `Fish` seen so far
 * @return 1 if there was an input problem; 0 otherwise
 */
static int count_green_fish(void* element,
                            size_t const element_size,
                            void* state)
{
    if (element == NULL ||
        state == NULL ||
        sizeof(Fish) != element_size) // Explicit size check
    {
        return 1; // Input error
    }

    Fish const* f = (Fish const*) element;
    size_t* green_fish_count = (size_t*) state;

    if (f->color == GREEN)
    {
        *green_fish_count += 1;
    }

    return 0;
}

static void test_apply_state_struct(void)
{
    // Make a vector with some green `Fish`.
    size_t greens_added = 0;
    size_t const total = 100;
    Vec* v = Vec_new(10, sizeof(Fish)); // Smaller than `total` for resizing

    assert(v != NULL);
    // Just for fun, let's have a random number of green ones.
    srand(time(NULL));
    for (size_t i = 0; i < total; ++i)
    {
        int const die = rand() % COLOR_NUM;

        switch(die)
        {
            case 0: // Red
                assert(true == Vec_append(v,
                                          &(Fish)
                                          {
                                              .color = RED,
                                              .size = 1
                                          },
                                          sizeof(Fish)));
                break;
            case 1: // Green
                assert(true == Vec_append(v,
                                          &(Fish)
                                          {
                                              .color = GREEN,
                                              .size = 1
                                          },
                                          sizeof(Fish)));
                greens_added++; // Note that we added a green.
                break;
            default: // Blue
                assert(true == Vec_append(v,
                                          &(Fish)
                                          {
                                              .color = BLUE,
                                              .size = 1
                                          },
                                          sizeof(Fish)));
        }
    }

    size_t greens_counted = 0;
    size_t* greens_counted_ptr = &greens_counted;
    int const success = 0; // Processed all elements

    assert(success == Vec_apply(v, count_green_fish, greens_counted_ptr));
    assert(*greens_counted_ptr == greens_added); // The count is correct

    Vec_destroy(&v);
}

/**
 * @brief Counts the number of blue `Fish` seen so far, requesting termination
 * when seeing a red or a green `Fish`
 * @param element A `Fish`
 * @param element_size For size-based type checking
 * @param state Used to update the number of blue `Fish` seen so far
 * @return 1 if there was an input problem; 0 if a blue `Fish` was seen and
 * counting should proceed; 2 if a red `Fish` was seen and counting should stop;
 * 3 if a green `Fish` was seen and counting should stop
 */
static int count_to_red_and_green_fish(void* element,
                                       size_t const element_size,
                                       void* state)
{
    if (element == NULL ||
        state == NULL ||
        sizeof(Fish) != element_size)
    {
        return 1; // Input error
    }

    Fish const* f = (Fish const*) element;
    size_t* non_red_count = (size_t*) state;

    if (f->color == RED)
    {
        return 2; // Red `Fish` seen; stop the count.
    }
    else if (f->color == GREEN)
    {
        return 3; // Green `Fish` seen; stop the count.
    }
    else
    {
        // Non-red `Fish` seen; update the count and proceed.
        *non_red_count += 1;

        return 0;
    }
}

static void test_apply_early_return_head(void)
{
    // Make a vector with a stop-worthy element at the beginning.
    Fish const stop = (Fish)
    {
        .color = RED,
        .size = 1
    };
    Fish const go = (Fish)
    {
        .color = BLUE,
        .size = 1
    };
    int const stop_error = 2;
    size_t const count = 7;
    Vec* v = Vec_new(count, sizeof(Fish));

    assert(v != NULL);
    assert(true == Vec_append(v, &stop, sizeof(Fish))); // Stop here
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(count == Vec_count(v));

    /*
     * We should iterate the correct number of times before hitting the
     * stop-worthy element.
     */
    size_t const iterations_expected = 0;
    size_t iterations = 0;
    size_t* p = &iterations;

    assert(stop_error == Vec_apply(v, count_to_red_and_green_fish, p));
    assert(*p == iterations_expected);

    Vec_destroy(&v);
}

static void test_apply_early_return_middle(void)
{
    // Make a vector with a stop-worthy element at the middle.
    Fish const stop = (Fish)
    {
        .color = RED,
        .size = 1
    };
    Fish const go = (Fish)
    {
        .color = BLUE,
        .size = 1
    };
    int const stop_error = 2;
    size_t const count = 7;
    Vec* v = Vec_new(count, sizeof(Fish));

    assert(v != NULL);
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &stop, sizeof(Fish))); // Stop here
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(count == Vec_count(v));

    /*
     * We should iterate the correct number of times before hitting the
     * stop-worthy element.
     */
    size_t const iterations_expected = 3;
    size_t iterations = 0;
    size_t* p = &iterations;

    assert(stop_error == Vec_apply(v, count_to_red_and_green_fish, p));
    assert(*p == iterations_expected);

    Vec_destroy(&v);
}

static void test_apply_early_return_tail(void)
{
    // Make a vector with a stop-worthy element at the end.
    Fish const stop = (Fish)
    {
        .color = RED,
        .size = 1
    };
    Fish const go = (Fish)
    {
        .color = BLUE,
        .size = 1
    };
    int const stop_error = 2;
    size_t const count = 7;
    Vec* v = Vec_new(count, sizeof(Fish));

    assert(v != NULL);
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &stop, sizeof(Fish))); // Stop here
    assert(count == Vec_count(v));

    /*
     * We should iterate the correct number of times before hitting the
     * stop-worthy element.
     */
    size_t const iterations_expected = 6;
    size_t iterations = 0;
    size_t* p = &iterations;

    assert(stop_error == Vec_apply(v, count_to_red_and_green_fish, p));
    assert(*p == iterations_expected);

    Vec_destroy(&v);
}

static void test_apply_early_return_different_error_codes(void)
{
    // Make a vector with a stop-worthy element somewhere.
    Fish const stop = (Fish)
    {
        .color = RED,
        .size = 1
    };
    Fish const stop_different = (Fish)
    {
        .color = GREEN,
        .size = 1
    };
    Fish const go = (Fish)
    {
        .color = BLUE,
        .size = 1
    };
    int const stop_error = 2;
    int const stop_different_error = 3;
    size_t const count = 7;
    Vec* v = Vec_new(count, sizeof(Fish));

    assert(v != NULL);
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &stop, sizeof(Fish))); // Stop here
    assert(true == Vec_append(v, &stop_different, sizeof(Fish))); // Unreachable
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &stop_different, sizeof(Fish))); // Unreachable
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &stop, sizeof(Fish))); // Unreachable
    assert(count == Vec_count(v));

    /*
     * We should iterate the correct number of times before hitting the
     * stop-worthy element, and we should get the correct error code (that of
     * the FIRST kind of stop-worthy element).
     */
    size_t iterations_expected = 1;
    size_t iterations = 0;
    size_t* p = &iterations;

    assert(stop_error == Vec_apply(v, count_to_red_and_green_fish, p));
    assert(*p == iterations_expected);

    Vec_destroy(&v);

    // Then make a vector with a different stop-worthy element somewhere.
    v = Vec_new(count, sizeof(Fish));

    assert(v != NULL);
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &stop_different, sizeof(Fish))); // Stop here
    assert(true == Vec_append(v, &stop, sizeof(Fish))); // Unreachable
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &stop_different, sizeof(Fish))); // Unreachable
    assert(true == Vec_append(v, &go, sizeof(Fish)));
    assert(true == Vec_append(v, &stop, sizeof(Fish))); // Unreachable
    assert(count == Vec_count(v));

    /*
     * We should iterate the correct number of times before hitting the
     * stop-worthy element, and we should get the correct error code (that of
     * the SECOND kind of stop-worthy element).
     */
    iterations_expected = 1;
    iterations = 0;
    p = &iterations;

    assert(stop_different_error == Vec_apply(v,
                                             count_to_red_and_green_fish,
                                             p));
    assert(*p == iterations_expected);

    Vec_destroy(&v);
}

int main(void)
{
    test_new();

    test_destroy();

    test_where_invalid();
    test_where();
    test_where_inner_pointer();

    test_where_if_invalid();
    test_where_if();

    test_has_invalid();
    test_has();
    test_has_inner_pointer();

    test_has_if_invalid();
    test_has_if_head();
    test_has_if_middle();
    test_has_if_tail();
    test_has_if_nowhere();

    test_get_invalid();
    test_get();

    test_append_invalid();
    test_append();
    test_append_inner_pointer();

    test_insert_invalid();
    test_insert_empty();
    test_insert_slack_middle();
    test_insert_slack_middle_struct();
    test_insert_slack_tail();
    test_insert_slack_past_tail();
    test_insert_slack_head();
    test_insert_until_full();
    test_insert_full_middle();
    test_insert_full_middle_struct();
    test_insert_full_tail();
    test_insert_full_past_tail();
    test_insert_full_head();
    test_insert_inner_pointer_head_before_insertion();
    test_insert_inner_pointer_head_at_insertion();
    test_insert_inner_pointer_head_after_insertion();
    test_insert_inner_pointer_middle_before_insertion();
    test_insert_inner_pointer_middle_at_insertion();
    test_insert_inner_pointer_middle_after_insertion();
    test_insert_inner_pointer_middle_after_insertion_struct();
    test_insert_inner_pointer_tail_before_insertion();
    test_insert_inner_pointer_tail_at_insertion();
    test_insert_inner_pointer_tail_after_insertion();
    test_insert_full_inner_pointer_head_before_insertion();
    test_insert_full_inner_pointer_head_at_insertion();
    test_insert_full_inner_pointer_head_after_insertion();
    test_insert_full_inner_pointer_middle_before_insertion();
    test_insert_full_inner_pointer_middle_at_insertion();
    test_insert_full_inner_pointer_middle_after_insertion();
    test_insert_full_inner_pointer_middle_after_insertion_struct();
    test_insert_full_inner_pointer_tail_before_insertion();
    test_insert_full_inner_pointer_tail_at_insertion();
    test_insert_full_inner_pointer_tail_after_insertion();

    test_remove_invalid();
    test_remove_middle();
    test_remove_tail();
    test_remove_head();
    test_remove_until_empty();

    test_equal_invalid();
    test_equal_unmodified_default_comparator();
    test_equal_unmodified_custom_comparator();
    test_equal_modified();
    test_equal_empty_same_element_size();
    test_equal_empty_diff_element_size();

    test_remove_all_invalid();
    test_remove_all_none();
    test_remove_all_one();
    test_remove_all_partial();
    test_remove_all_except_one();
    test_remove_all_total();
    test_remove_all_partial_inner_pointer_to_head();
    test_remove_all_partial_inner_pointer_to_middle();
    test_remove_all_partial_inner_pointer_to_tail();
    test_remove_all_total_inner_pointer_to_head();
    test_remove_all_total_inner_pointer_to_middle();
    test_remove_all_total_inner_pointer_to_tail();

    test_remove_all_if_invalid();
    test_remove_all_if_none();
    test_remove_all_if_one();
    test_remove_all_if_some();
    test_remove_all_if_all();

    test_qsort_invalid();
    test_qsort_scalar();
    test_qsort_struct();

    test_apply_invalid();
    test_apply_modify_scalar();
    test_apply_modify_struct();
    test_apply_state_scalar();
    test_apply_state_struct();
    test_apply_early_return_head();
    test_apply_early_return_middle();
    test_apply_early_return_tail();
    test_apply_early_return_different_error_codes();

    return EXIT_SUCCESS;
}
