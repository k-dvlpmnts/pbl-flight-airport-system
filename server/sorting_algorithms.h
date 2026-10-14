#pragma once

namespace Sort {

template<typename T>
void bubbleSort(T* a, int n, bool (*cmp)(const T&, const T&)) {
    for (int i = 0; i < n - 1; i++) {
        bool swapped = false;
        for (int j = 0; j < n - i - 1; j++) {
            if (cmp(a[j + 1], a[j])) {
                T t = a[j]; a[j] = a[j + 1]; a[j + 1] = t;
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}

template<typename T>
void insertionSort(T* a, int n, bool (*cmp)(const T&, const T&)) {
    for (int i = 1; i < n; i++) {
        T key = a[i];
        int j = i - 1;
        while (j >= 0 && cmp(key, a[j])) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

template<typename T>
void selectionSort(T* a, int n, bool (*cmp)(const T&, const T&)) {
    for (int i = 0; i < n - 1; i++) {
        int m = i;
        for (int j = i + 1; j < n; j++)
            if (cmp(a[j], a[m])) m = j;
        if (m != i) { T t = a[i]; a[i] = a[m]; a[m] = t; }
    }
}

template<typename T>
static void mergeArr(T* a, T* tmp, int l, int m, int r, bool (*cmp)(const T&, const T&)) {
    int i = l, j = m + 1, k = l;
    while (i <= m && j <= r) tmp[k++] = cmp(a[j], a[i]) ? a[j++] : a[i++];
    while (i <= m) tmp[k++] = a[i++];
    while (j <= r) tmp[k++] = a[j++];
    for (int x = l; x <= r; x++) a[x] = tmp[x];
}

template<typename T>
static void msort(T* a, T* tmp, int l, int r, bool (*cmp)(const T&, const T&)) {
    if (l >= r) return;
    int m = (l + r) / 2;
    msort(a, tmp, l, m, cmp);
    msort(a, tmp, m + 1, r, cmp);
    mergeArr(a, tmp, l, m, r, cmp);
}

template<typename T>
void mergeSort(T* a, int n, bool (*cmp)(const T&, const T&)) {
    if (n < 2) return;
    T* tmp = new T[n];
    msort(a, tmp, 0, n - 1, cmp);
    delete[] tmp;
}

template<typename T>
static int partition(T* a, int lo, int hi, bool (*cmp)(const T&, const T&)) {
    T pivot = a[hi];
    int i = lo - 1;
    for (int j = lo; j < hi; j++) {
        if (cmp(a[j], pivot)) {
            i++;
            T t = a[i]; a[i] = a[j]; a[j] = t;
        }
    }
    T t = a[i + 1]; a[i + 1] = a[hi]; a[hi] = t;
    return i + 1;
}

template<typename T>
static void qsortRec(T* a, int lo, int hi, bool (*cmp)(const T&, const T&)) {
    if (lo < hi) {
        int p = partition(a, lo, hi, cmp);
        qsortRec(a, lo, p - 1, cmp);
        qsortRec(a, p + 1, hi, cmp);
    }
}

template<typename T>
void quickSort(T* a, int n, bool (*cmp)(const T&, const T&)) {
    if (n > 1) qsortRec(a, 0, n - 1, cmp);
}

template<typename T>
static void heapify(T* a, int n, int i, bool (*cmp)(const T&, const T&)) {
    int l = 2 * i + 1, r = 2 * i + 2, s = i;
    if (l < n && cmp(a[l], a[s])) s = l;
    if (r < n && cmp(a[r], a[s])) s = r;
    if (s != i) {
        T t = a[i]; a[i] = a[s]; a[s] = t;
        heapify(a, n, s, cmp);
    }
}

template<typename T>
void heapSort(T* a, int n, bool (*cmp)(const T&, const T&)) {
    for (int i = n / 2 - 1; i >= 0; i--) heapify(a, n, i, cmp);
    for (int i = n - 1; i > 0; i--) {
        T t = a[0]; a[0] = a[i]; a[i] = t;
        heapify(a, i, 0, cmp);
    }
}

template<typename T>
int binarySearch(T* a, int n, const T& key, int (*cmp)(const T&, const T&)) {
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int m = lo + (hi - lo) / 2;
        int c = cmp(a[m], key);
        if (c == 0) return m;
        if (c < 0) lo = m + 1;
        else hi = m - 1;
    }
    return -1;
}

} // namespace Sort