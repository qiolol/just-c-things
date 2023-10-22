#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <chrono>
#include <cstdint>
#include <vector>
#include <time.h>
#include <algorithm>
extern "C"
{
    #include "../Vec.h"
}

static void print_my_vec_i64(const Vec* v, const size_t size)
{
    if (v == nullptr)
    {
        return;
    }

    std::cerr << "My vector   ";
    for (int i = 0; i < size; ++i)
    {
        const int64_t* probe = (const int64_t*) Vec_get(v, i);

        if (probe == nullptr)
        {
            std::cerr << "\nNull element pointer!\n";
            exit(EXIT_FAILURE);
        }

        std::cerr << "[" << *probe << "]";
    }
    std::cerr << "\n";
}

static void print_std_vec_i64(const std::vector<int64_t>& v)
{
    std::cerr << "std::vector ";
    for (int64_t i : v)
    {
        std::cerr << "[" << i << "]";
    }
    std::cerr << "\n";
}

static void report_times(const std::string bench_description,
                        const std::chrono::duration<double> Vec_time,
                        const std::chrono::duration<double> std_vec_time)
{
    const std::chrono::duration<double, std::milli> Vec_ms = Vec_time;
    const std::chrono::duration<double, std::milli> std_vec_ms = std_vec_time;

    std::cout << "##########################################\n";
    std::cout << bench_description;
    std::cout << "\n\t     My Vec: " << Vec_ms.count() << " ms";
    std::cout << "\n\tstd::vector: " << std_vec_ms.count() << " ms";
    std::cout << "\n\nWINNER: " << (Vec_ms.count() < std_vec_ms.count() ? "My Vec" : "std::vector") << '\n';
    std::cout << std::endl;
}




static std::chrono::duration<double> append_my_vec(const size_t n)
{
    size_t pre_append_cap = 0;
    Vec* v = Vec_new(n, sizeof(int64_t));

    assert(v != nullptr);
    pre_append_cap = Vec_capacity(v);

    auto start = std::chrono::high_resolution_clock::now(); // Start timer

    for (int64_t i = 0; (size_t) i < n; ++i)
    {
        Vec_append(v, &i, sizeof(i));
    }

    auto end = std::chrono::high_resolution_clock::now(); // End timer

    assert(Vec_capacity(v) == pre_append_cap); // Vector did not resize
    assert(Vec_count(v) == n); // All elements were added

    // Print vector contents to make sure operations weren't optimized out.
    print_my_vec_i64(v, Vec_count(v));

    Vec_destroy(&v);

    return end - start;
}

static std::chrono::duration<double> append_std_vec(const size_t n)
{
    size_t pre_append_cap = 0;
    std::vector<int64_t> v;

    v.reserve(n);
    pre_append_cap = v.capacity();

    auto start = std::chrono::high_resolution_clock::now(); // Start timer

    for (int64_t i = 0; (size_t) i < n; ++i)
    {
        v.push_back(i);
    }

    auto end = std::chrono::high_resolution_clock::now(); // End timer

    assert(v.capacity() == pre_append_cap); // Vector did not resize
    assert(v.size() == n); // All elements were added

    // Print vector contents to make sure operations weren't optimized out.
    print_std_vec_i64(v);

    return end - start;
}

static void append(const size_t n)
{
    report_times("Append",
                 append_my_vec(n),
                 append_std_vec(n));
}




static std::chrono::duration<double> append_with_resize_my_vec(const size_t n)
{
    size_t pre_resize_cap = 0;
    Vec* v = Vec_new(n / 2, sizeof(int64_t));

    assert(v != nullptr);
    pre_resize_cap = Vec_capacity(v);

    auto start = std::chrono::high_resolution_clock::now(); // Start timer

    for (int64_t i = 0; (size_t) i < n; ++i)
    {
        Vec_append(v, &i, sizeof(i));
    }

    auto end = std::chrono::high_resolution_clock::now(); // End timer

    assert(Vec_capacity(v) > pre_resize_cap); // Vector resized
    assert(Vec_count(v) == n); // All elements were added

    // Print vector contents to make sure operations weren't optimized out.
    print_my_vec_i64(v, Vec_count(v));

    Vec_destroy(&v);

    return end - start;
}

static std::chrono::duration<double> append_with_resize_std_vec(const size_t n)
{
    size_t pre_resize_cap = 0;
    std::vector<int64_t> v;

    v.reserve(n / 2);
    pre_resize_cap = v.capacity();

    auto start = std::chrono::high_resolution_clock::now(); // Start timer

    for (int64_t i = 0; (size_t) i < n; ++i)
    {
        v.push_back(i);
    }

    auto end = std::chrono::high_resolution_clock::now(); // End timer

    assert(v.capacity() > pre_resize_cap); // Vector resized
    assert(v.size() == n); // All elements were added

    // Print vector contents to make sure operations weren't optimized out.
    print_std_vec_i64(v);

    return end - start;
}

static void append_with_resize(const size_t n)
{
    report_times("Append with resize",
                 append_with_resize_my_vec(n),
                 append_with_resize_std_vec(n));
}




static Vec* Vec_of_n_seq_i64(const size_t n)
{
    Vec* v = Vec_new(n, sizeof(int64_t));

    assert(v != nullptr);

    for (int64_t i = 0; (size_t) i < n; ++i)
    {
        Vec_append(v, &i, sizeof(i));
    }

    assert(Vec_count(v) == n); // All elements were added

    return v;
}

static bool even_i64(void const* element, size_t const element_size)
{
    if (element == nullptr ||
        element_size != sizeof(int64_t))
    {
        return false; // Error; return early.
    }

    int64_t const* i = (int64_t const*) element; // Cast to the known type.

    return *i % 2 == 0;
}

static std::chrono::duration<double> remove_all_even_my_vec(const size_t n)
{
    Vec* v = Vec_of_n_seq_i64(n);

    auto start = std::chrono::high_resolution_clock::now(); // Start timer

    const size_t removed = Vec_remove_all_if(v, even_i64);

    auto end = std::chrono::high_resolution_clock::now(); // End timer

    assert(n % 2 == 0); // Assuming `n` is even...
    assert(removed == n / 2); // ...we should've removed half the elts.

    // Print result to make sure operations weren't optimized out.
    std::cerr << removed << " elements removed from my vector.\n";

    Vec_destroy(&v);

    return end - start;
}

static std::vector<int64_t> std_vec_of_n_seq_i64(const size_t n)
{
    std::vector<int64_t> v;

    for (int64_t i = 0; (size_t) i < n; ++i)
    {
        v.push_back(i);
    }

    assert(v.size() == n); // All elements were added

    return v;
}

static std::chrono::duration<double> remove_all_even_std_vec(const size_t n)
{
    std::vector<int64_t> v = std_vec_of_n_seq_i64(n);

    auto start = std::chrono::high_resolution_clock::now(); // Start timer

    v.erase(std::remove_if(v.begin(),
                           v.end(),
                           [](int64_t& i) { return i % 2 == 0; }),
            v.end());

    auto end = std::chrono::high_resolution_clock::now(); // End timer

    assert(n % 2 == 0); // Assuming `n` is even...
    assert(v.size() == n / 2); // ...we should've removed half the elts.

    // Print vector contents to make sure operations weren't optimized out.
    print_std_vec_i64(v);

    return end - start;
}

static void remove_all_even(const size_t n)
{
    report_times("Remove all even elements",
                 remove_all_even_my_vec(n),
                 remove_all_even_std_vec(n));
}




static std::chrono::duration<double> insert_my_vec(const size_t n)
{
    Vec* v = Vec_of_n_seq_i64(n);
    const int64_t element = 123;
    const size_t index = 5;

    assert(n > index); // Assuming `n` is larger than the `index`...

    auto start = std::chrono::high_resolution_clock::now(); // Start timer

    // ... `n - index` elements should shift to make room for the insertion.
    Vec_insert(v, index, &element, sizeof(element));

    auto end = std::chrono::high_resolution_clock::now(); // End timer

    // Print vector contents to make sure operations weren't optimized out.
    print_my_vec_i64(v, Vec_count(v));

    Vec_destroy(&v);

    return end - start;
}

static std::chrono::duration<double> insert_std_vec(const size_t n)
{
    std::vector<int64_t> v = std_vec_of_n_seq_i64(n);
    const int64_t element = 123;
    const size_t index = 5;

    assert(n > index); // Assuming `n` is larger than the `index`...

    auto start = std::chrono::high_resolution_clock::now(); // Start timer

    // ... `n - index` elements should shift to make room for the insertion.
    v.insert(v.begin() + index, element);

    auto end = std::chrono::high_resolution_clock::now(); // End timer

    // Print vector contents to make sure operations weren't optimized out.
    print_std_vec_i64(v);

    return end - start;
}

static void insert(const size_t n)
{
    report_times("Insertion",
                 insert_my_vec(n),
                 insert_std_vec(n));
}




static std::chrono::duration<double> find_my_vec(const size_t n,
                                                 const int64_t element)
{
    Vec* v = Vec_of_n_seq_i64(n);

    auto start = std::chrono::high_resolution_clock::now(); // Start timer

    const bool found = Vec_has(v, &element, sizeof(element));

    auto end = std::chrono::high_resolution_clock::now(); // End timer

    // Print result to make sure operations weren't optimized out.
    std::cerr << "Element found? " << found << "\n";

    Vec_destroy(&v);

    return end - start;
}

static std::chrono::duration<double> find_std_vec(const size_t n,
                                                  const int64_t element)
{
    std::vector<int64_t> v = std_vec_of_n_seq_i64(n);

    auto start = std::chrono::high_resolution_clock::now(); // Start timer

    auto it = std::find(v.begin(), v.end(), element);

    auto end = std::chrono::high_resolution_clock::now(); // End timer

    // Print result to make sure operations weren't optimized out.
    const bool found = (it != std::end(v));

    std::cerr << "Element found? " << found << "\n";

    return end - start;
}

static void find(const size_t n)
{
    /*
     * NOTE: This assumes both vectors are constructed with the sequence of
     * `int64_t` elements `[0, n)`.
     */
    srand(time(NULL));
    const int64_t mid_to_end_element = rand() % (n / 2) + (n / 2);

    report_times("Search for a random element",
                 find_my_vec(n, mid_to_end_element),
                 find_std_vec(n, mid_to_end_element));
}




#define UNUSED(x) (void)(x)

static int subtract_self(void* element, size_t const element_size, void* state)
{
    UNUSED(state);
    if (element == nullptr ||
        element_size != sizeof(int64_t))
    {
        return 1; // Error; return early.
    }

    int64_t* i = (int64_t*) element; // Cast to the known type.

    *i -= *i; // Subtract the element from itself.

    return 0; // Return success status.
}

static std::chrono::duration<double> apply_my_vec(const size_t n)
{
    Vec* v = Vec_of_n_seq_i64(n);

    auto start = std::chrono::high_resolution_clock::now(); // Start timer

    Vec_apply(v, subtract_self, nullptr);

    auto end = std::chrono::high_resolution_clock::now(); // End timer

    // All the elements were subtracted from themselves so are now zero.
    for (size_t i = 0; i < n; ++i)
    {
        int64_t* probe = (int64_t*) Vec_get(v, i);

        assert(probe != nullptr);
        assert(*probe == 0);
    }

    // Print vector contents to make sure operations weren't optimized out.
    print_my_vec_i64(v, Vec_count(v));

    Vec_destroy(&v);

    return end - start;
}

static std::chrono::duration<double> apply_std_vec(const size_t n)
{
    std::vector<int64_t> v = std_vec_of_n_seq_i64(n);

    auto start = std::chrono::high_resolution_clock::now(); // Start timer

    std::for_each(v.begin(), v.end(), [](int64_t& i) { i -= i; });

    auto end = std::chrono::high_resolution_clock::now(); // End timer

    // All the elements were subtracted from themselves so are now zero.
    for (auto it = v.cbegin(); it != v.cend(); ++it)
    {
        assert(*it == 0);
    }

    // Print vector contents to make sure operations weren't optimized out.
    print_std_vec_i64(v);

    return end - start;
}

static void apply(const size_t n)
{
    report_times("Applying an operation on all elements",
                 apply_my_vec(n),
                 apply_std_vec(n));
}

int main()
{
    append(1000000);
    append_with_resize(1000000);
    remove_all_even(1000000);
    insert(1000000);
    find(1000000);
    apply(1000000);

    return EXIT_SUCCESS;
}
