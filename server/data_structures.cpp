#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class MinHeap {
private:
    DynArray<HeapItem> a;
    void siftUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (a[p].key <= a[i].key) break;
            HeapItem t = a[p]; a[p] = a[i]; a[i] = t;
            i = p;
        }
    }
    void siftDown(int i) {
        int n = a.size();
        for (;;) {
            int l = 2 * i + 1, r = l + 1, m = i;
            if (l < n && a[l].key < a[m].key) m = l;
            if (r < n && a[r].key < a[m].key) m = r;
            if (m == i) break;
            HeapItem t = a[m]; a[m] = a[i]; a[i] = t;
            i = m;
        }
    }
public:
    bool empty() const { return a.size() == 0; }
    int size() const { return a.size(); }
    void push(long long key, int value) {
        a.push(HeapItem(key, value));
        siftUp(a.size() - 1);
    }
    HeapItem pop() {
        HeapItem top = a[0];
        a[0] = a[a.size() - 1];
        a.removeAt(a.size() - 1);
        if (a.size() > 0) siftDown(0);
        return top;
    }
};


template <class T>
struct StackNode {
    T data;
    StackNode* next;
    StackNode(const T& d) : data(d), next(0) {}
};

template <class T>
class Stack {
private:
    StackNode<T>* topNode;
    int count;
    Stack(const Stack&);               
    Stack& operator=(const Stack&);
public:
    Stack() : topNode(0), count(0) {}
    ~Stack() { while (!empty()) pop(); }
    void push(const T& v) {
        StackNode<T>* n = new StackNode<T>(v);
        n->next = topNode;
        topNode = n;
        count++;
    }
    T pop() {
        T v = topNode->data;
        StackNode<T>* n = topNode;
        topNode = topNode->next;
        delete n;
        count--;
        return v;
    }
    T& top() { return topNode->data; }
    bool empty() const { return topNode == 0; }
    int size() const { return count; }
};