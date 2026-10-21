// =============================================================================
//  sorting_algorithms.cpp
//  Hand written sorting and searching routines (no STL, no qsort/bsearch).
//
//  All sorts work on DataStructures::DynArray<T> and take a comparator
//  "cmp(a, b)" that returns  <0 if a<b,  0 if equal,  >0 if a>b.
//  The comparator can be any plain function (or function object).
//
//     insertionSort  - O(n^2), best for tiny / almost sorted arrays, stable
//     selectionSort  - O(n^2), few swaps
//     bubbleSort     - O(n^2), stops early when sorted, stable
//     mergeSort      - O(n log n), stable (default for records loaded from files)
//     quickSort      - O(n log n) average, median-of-three pivot
//     heapSort       - O(n log n) worst case, in place
//
//  Searching (array MUST be sorted with the matching comparator):
//     binarySearch(arr, key, keyCmp)  -> index of a match, or -1
//     lowerBound(arr, key, keyCmp)    -> first index whose element is >= key
//     upperBound(arr, key, keyCmp)    -> first index whose element is >  key
//  keyCmp(element, key) returns <0 / 0 / >0 comparing an element with the key.
//
//  Build the self test:   g++ -DSORT_SELF_TEST sorting_algorithms.cpp -o sort_test
// =============================================================================
#ifndef SORTING_ALGORITHMS_CPP
#define SORTING_ALGORITHMS_CPP

#include "data_structures.cpp"

namespace Sorting {
using DataStructures::DynArray;

template <class T>
inline void swapItems(DynArray<T>& a, int i, int j) {
    if (i == j) return;
    T t = a[i];
    a[i] = a[j];
    a[j] = t;
}

// ---------------------------------------------------------------- insertion
template <class T, class Cmp>
void insertionSortRange(DynArray<T>& a, int lo, int hi, Cmp cmp) {   // [lo, hi]
    for (int i = lo + 1; i <= hi; i++) {
        T key = a[i];
        int j = i - 1;
        while (j >= lo && cmp(a[j], key) > 0) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}
template <class T, class Cmp>
void insertionSort(DynArray<T>& a, Cmp cmp) {
    if (a.size() > 1) insertionSortRange(a, 0, a.size() - 1, cmp);
}

// ---------------------------------------------------------------- selection
template <class T, class Cmp>
void selectionSort(DynArray<T>& a, Cmp cmp) {
    int n = a.size();
    for (int i = 0; i < n - 1; i++) {
        int m = i;
        for (int j = i + 1; j < n; j++)
            if (cmp(a[j], a[m]) < 0) m = j;
        swapItems(a, i, m);
    }
}

// ------------------------------------------------------------------- bubble
template <class T, class Cmp>
void bubbleSort(DynArray<T>& a, Cmp cmp) {
    int n = a.size();
    for (int pass = 0; pass < n - 1; pass++) {
        bool swapped = false;
        for (int j = 0; j < n - 1 - pass; j++) {
            if (cmp(a[j], a[j + 1]) > 0) {
                swapItems(a, j, j + 1);
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}

// -------------------------------------------------------------------- merge
template <class T, class Cmp>
void mergeSortRec(DynArray<T>& a, DynArray<T>& tmp, int lo, int hi, Cmp cmp) {
    if (hi - lo < 1) return;
    int mid = lo + (hi - lo) / 2;
    mergeSortRec(a, tmp, lo, mid, cmp);
    mergeSortRec(a, tmp, mid + 1, hi, cmp);
    int i = lo, j = mid + 1, k = lo;
    while (i <= mid && j <= hi) {
        if (cmp(a[j], a[i]) < 0) tmp[k++] = a[j++];       // strict: keeps it stable
        else                     tmp[k++] = a[i++];
    }
    while (i <= mid) tmp[k++] = a[i++];
    while (j <= hi)  tmp[k++] = a[j++];
    for (k = lo; k <= hi; k++) a[k] = tmp[k];
}
template <class T, class Cmp>
void mergeSort(DynArray<T>& a, Cmp cmp) {
    if (a.size() < 2) return;
    DynArray<T> tmp;
    tmp.resize(a.size());
    mergeSortRec(a, tmp, 0, a.size() - 1, cmp);
}

// -------------------------------------------------------------------- quick
template <class T, class Cmp>
void quickSortRec(DynArray<T>& a, int lo, int hi, Cmp cmp) {
    while (hi - lo > 10) {
        int mid = lo + (hi - lo) / 2;                     // median of three -> a[mid]
        if (cmp(a[mid], a[lo]) < 0) swapItems(a, mid, lo);
        if (cmp(a[hi], a[lo]) < 0)  swapItems(a, hi, lo);
        if (cmp(a[hi], a[mid]) < 0) swapItems(a, hi, mid);
        T pivot = a[mid];
        int i = lo, j = hi;
        while (i <= j) {
            while (cmp(a[i], pivot) < 0) i++;
            while (cmp(a[j], pivot) > 0) j--;
            if (i <= j) { swapItems(a, i, j); i++; j--; }
        }
        // recurse into the smaller part, loop on the larger (bounded stack depth)
        if (j - lo < hi - i) { quickSortRec(a, lo, j, cmp); lo = i; }
        else                 { quickSortRec(a, i, hi, cmp); hi = j; }
    }
    insertionSortRange(a, lo, hi, cmp);
}
template <class T, class Cmp>
void quickSort(DynArray<T>& a, Cmp cmp) {
    if (a.size() > 1) quickSortRec(a, 0, a.size() - 1, cmp);
}

// --------------------------------------------------------------------- heap
template <class T, class Cmp>
void heapSiftDown(DynArray<T>& a, int i, int n, Cmp cmp) {
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < n && cmp(a[l], a[m]) > 0) m = l;
        if (r < n && cmp(a[r], a[m]) > 0) m = r;
        if (m == i) return;
        swapItems(a, i, m);
        i = m;
    }
}
template <class T, class Cmp>
void heapSort(DynArray<T>& a, Cmp cmp) {
    int n = a.size();
    for (int i = n / 2 - 1; i >= 0; i--) heapSiftDown(a, i, n, cmp);
    for (int end = n - 1; end > 0; end--) {
        swapItems(a, 0, end);
        heapSiftDown(a, 0, end, cmp);
    }
}

// ------------------------------------------------------------------ helpers
template <class T, class Cmp>
bool isSorted(const DynArray<T>& a, Cmp cmp) {
    for (int i = 1; i < a.size(); i++)
        if (cmp(a[i - 1], a[i]) > 0) return false;
    return true;
}

// ------------------------------------------------------------ binary search
template <class T, class K, class KeyCmp>
int lowerBound(const DynArray<T>& a, const K& key, KeyCmp cmp) {
    int lo = 0, hi = a.size();                            // search in [lo, hi)
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (cmp(a[mid], key) < 0) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}
template <class T, class K, class KeyCmp>
int upperBound(const DynArray<T>& a, const K& key, KeyCmp cmp) {
    int lo = 0, hi = a.size();
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (cmp(a[mid], key) <= 0) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}
template <class T, class K, class KeyCmp>
int binarySearch(const DynArray<T>& a, const K& key, KeyCmp cmp) {
    int lo = 0, hi = a.size() - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int c = cmp(a[mid], key);
        if (c == 0) return mid;
        if (c < 0) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

} // namespace Sorting

// -----------------------------------------------------------------------------
#ifdef SORT_SELF_TEST
using namespace DataStructures;

static int intCmp(const int& a, const int& b) { return a < b ? -1 : (a > b ? 1 : 0); }
static int intKeyCmp(const int& e, const int& k) { return e < k ? -1 : (e > k ? 1 : 0); }

static unsigned int rngState = 12345;
static int nextRand() { rngState = rngState * 1103515245u + 12345u; return (int)((rngState >> 16) & 0x7fff); }

typedef void (*SortFn)(DynArray<int>&, int (*)(const int&, const int&));
static void doIns(DynArray<int>& a, int (*c)(const int&, const int&)) { Sorting::insertionSort(a, c); }
static void doSel(DynArray<int>& a, int (*c)(const int&, const int&)) { Sorting::selectionSort(a, c); }
static void doBub(DynArray<int>& a, int (*c)(const int&, const int&)) { Sorting::bubbleSort(a, c); }
static void doMrg(DynArray<int>& a, int (*c)(const int&, const int&)) { Sorting::mergeSort(a, c); }
static void doQck(DynArray<int>& a, int (*c)(const int&, const int&)) { Sorting::quickSort(a, c); }
static void doHep(DynArray<int>& a, int (*c)(const int&, const int&)) { Sorting::heapSort(a, c); }

int main() {
    const char* names[] = { "insertion", "selection", "bubble", "merge", "quick", "heap" };
    SortFn fns[] = { doIns, doSel, doBub, doMrg, doQck, doHep };
    int sizes[] = { 0, 1, 2, 5, 17, 200, 1000 };
    int failures = 0;
    for (int f = 0; f < 6; f++) {
        for (int s = 0; s < 7; s++) {
            for (int variant = 0; variant < 3; variant++) {
                DynArray<int> a;
                for (int i = 0; i < sizes[s]; i++) {
                    int v = (variant == 0) ? nextRand() % 50 : (variant == 1 ? i : sizes[s] - i);
                    a.push(v);
                }
                fns[f](a, intCmp);
                if (!Sorting::isSorted(a, intCmp)) { printf("FAIL %s n=%d variant=%d\n", names[f], sizes[s], variant); failures++; }
            }
        }
    }
    DynArray<int> a;
    for (int i = 0; i < 100; i++) a.push(i * 2);          // even numbers 0..198
    int idx = Sorting::binarySearch(a, 74, intKeyCmp);
    int none = Sorting::binarySearch(a, 75, intKeyCmp);
    DynArray<int> d;                                       // duplicates
    d.push(1); d.push(2); d.push(2); d.push(2); d.push(5);
    int lb = Sorting::lowerBound(d, 2, intKeyCmp), ub = Sorting::upperBound(d, 2, intKeyCmp);
    printf("binarySearch(74)=%d (expect 37)  binarySearch(75)=%d (expect -1)  range of 2 = [%d,%d) (expect [1,4))\n",
           idx, none, lb, ub);
    if (idx != 37 || none != -1 || lb != 1 || ub != 4) failures++;
    printf(failures ? "SORT TESTS FAILED\n" : "all sorting / searching tests passed\n");
    return failures;
}
#endif

#endif // SORTING_ALGORITHMS_CPP
