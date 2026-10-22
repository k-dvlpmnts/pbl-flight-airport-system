#ifndef DATA_STRUCTURES_CPP
#define DATA_STRUCTURES_CPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
namespace DataStructures {

class Node {
public:
    int data;
    Node* next;
    Node(int val) : data(val), next(0) {}
};

class LinkedList {
private:
    Node* head;
public:
    LinkedList() : head(0) {}
    ~LinkedList() {
#ifdef DS_SELF_TEST
        printf("Destructor Called\n");
#endif
        Node* itr = head;
        while (itr) {
            Node* nx = itr->next;
            delete itr;
            itr = nx;
        }
    }
    void insertAtBegin(int val) {
        Node* newNode = new Node(val);
        newNode->next = head;
        head = newNode;
    }
    void insertAtEnd(int val) {
        Node* newNode = new Node(val);
        if (head == 0) { head = newNode; return; }
        Node* itr = head;
        while (itr->next) itr = itr->next;
        itr->next = newNode;
    }
    // inserts after the k-th node (1 based); k <= 0 inserts at the beginning
    void insertAtPositionK(int k, int val) {
        if (head == 0 || k <= 0) { insertAtBegin(val); return; }
        Node* newnode = new Node(val);
        Node* itr = head;
        int i = 1;
        while (itr->next && i < k) { itr = itr->next; i++; }
        newnode->next = itr->next;
        itr->next = newnode;
    }
    void deleteAtBegin() {
        if (head == 0) return;
        Node* temp = head;
        head = head->next;
        delete temp;
    }
    void deleteAtEnd() {
        if (head == 0) return;
        if (head->next == 0) { delete head; head = 0; return; }
        Node* itr = head;
        while (itr->next->next) itr = itr->next;
        delete itr->next;
        itr->next = 0;
    }
    void deleteValueK(int k) {
        if (head == 0) return;
        if (head->data == k) {
            Node* temp = head;
            head = head->next;
            delete temp;
            return;
        }
        Node* itr = head;
        while (itr->next && itr->next->data != k) itr = itr->next;
        if (itr->next == 0) return;               // value not present
        Node* temp = itr->next;
        itr->next = temp->next;
        delete temp;
    }
    int size() const {
        int n = 0;
        for (Node* i = head; i; i = i->next) n++;
        return n;
    }
    bool contains(int v) const {
        for (Node* i = head; i; i = i->next) if (i->data == v) return true;
        return false;
    }
    void print() const {
        for (Node* i = head; i; i = i->next) printf("%d->", i->data);
        printf("\n");
    }
};

// 2. DynArray<T>  - growable contiguous array
template <class T>
class DynArray {
private:
    T* items;
    int count;
    int cap;

    void reserve(int need) {
        if (need <= cap) return;
        int ncap = (cap == 0) ? 8 : cap;
        while (ncap < need) ncap *= 2;
        T* nitems = new T[ncap];
        for (int i = 0; i < count; i++) nitems[i] = items[i];
        delete[] items;
        items = nitems;
        cap = ncap;
    }
    void copyFrom(const DynArray& o) {
        reserve(o.count);
        for (int i = 0; i < o.count; i++) items[i] = o.items[i];
        count = o.count;
    }
    void check(int i) const {
        if (i < 0 || i >= count) {
            fprintf(stderr, "DynArray: index %d out of range (size %d)\n", i, count);
            abort();
        }
    }
public:
    DynArray() : items(0), count(0), cap(0) {}
    DynArray(const DynArray& o) : items(0), count(0), cap(0) { copyFrom(o); }
    DynArray& operator=(const DynArray& o) {
        if (this != &o) { count = 0; copyFrom(o); }
        return *this;
    }
    ~DynArray() { delete[] items; }

    int size() const { return count; }
    bool empty() const { return count == 0; }
    void clear() { count = 0; }

    void push(const T& v) {
        T copy = v;                       // v may live inside this array
        reserve(count + 1);
        items[count++] = copy;
    }
    T& operator[](int i) { check(i); return items[i]; }
    const T& operator[](int i) const { check(i); return items[i]; }
    T& last() { check(count - 1); return items[count - 1]; }

    void insertAt(int idx, const T& v) {
        if (idx < 0 || idx > count) idx = count;
        T copy = v;
        reserve(count + 1);
        for (int i = count; i > idx; i--) items[i] = items[i - 1];
        items[idx] = copy;
        count++;
    }
    void removeAt(int idx) {
        check(idx);
        for (int i = idx; i < count - 1; i++) items[i] = items[i + 1];
        count--;
    }
    // grow (with default constructed items) or shrink to exactly n items
    void resize(int n) {
        if (n < 0) n = 0;
        reserve(n);
        for (int i = count; i < n; i++) items[i] = T();
        count = n;
    }
};

// 3. String 
class String {
private:
    char* buf;
    int len;
    int cap;

    void init(const char* s, int n) {
        if (!s) { s = ""; n = 0; }
        len = n;
        cap = (n + 1 < 16) ? 16 : n + 1;
        buf = (char*)malloc(cap);
        memcpy(buf, s, n);
        buf[n] = 0;
    }
    void reserve(int needLen) {            // makes room for needLen chars + null
        if (needLen + 1 <= cap) return;
        int ncap = cap;
        while (ncap < needLen + 1) ncap *= 2;
        buf = (char*)realloc(buf, ncap);
        cap = ncap;
    }
public:
    String() { init("", 0); }
    String(const char* s) { init(s, s ? (int)strlen(s) : 0); }
    String(const char* s, int n) { init(s, n); }
    String(const String& o) { init(o.buf, o.len); }
    ~String() { free(buf); }

    String& operator=(const String& o) {
        if (this != &o) {
            reserve(o.len);
            memcpy(buf, o.buf, o.len);
            len = o.len;
            buf[len] = 0;
        }
        return *this;
    }
    String& operator=(const char* s) { String t(s); return *this = t; }

    int length() const { return len; }
    bool empty() const { return len == 0; }
    const char* c_str() const { return buf; }
    char operator[](int i) const { return (i < 0 || i >= len) ? 0 : buf[i]; }
    void clear() { len = 0; buf[0] = 0; }

    String& append(const char* s, int n) {
        if (!s || n <= 0) return *this;
        if (s >= buf && s <= buf + len) {          // appending part of itself
            String tmp(s, n);
            return append(tmp.buf, n);
        }
        reserve(len + n);
        memcpy(buf + len, s, n);
        len += n;
        buf[len] = 0;
        return *this;
    }
    String& append(const char* s) { return append(s, s ? (int)strlen(s) : 0); }
    String& append(const String& o) { return append(o.buf, o.len); }
    String& appendChar(char c) { return append(&c, 1); }
    String& appendInt(long long v) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%lld", v);
        return append(tmp);
    }
    String& appendDouble(double v, int decimals) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "%.*f", decimals, v);
        return append(tmp);
    }
    String& operator+=(const String& o) { return append(o); }
    String& operator+=(const char* s) { return append(s); }
    String& operator+=(char c) { return appendChar(c); }

    int compare(const String& o) const { return strcmp(buf, o.buf); }
    bool equals(const char* s) const { return strcmp(buf, s ? s : "") == 0; }
    bool startsWith(const char* p) const { return strncmp(buf, p, strlen(p)) == 0; }

    int indexOf(char c, int from = 0) const {
        for (int i = (from < 0 ? 0 : from); i < len; i++) if (buf[i] == c) return i;
        return -1;
    }
    String substr(int start, int n = -1) const {
        if (start < 0) start = 0;
        if (start >= len) return String();
        if (n < 0 || start + n > len) n = len - start;
        return String(buf + start, n);
    }
    String trimmed() const {
        int a = 0, b = len;
        while (a < b && (buf[a] == ' ' || buf[a] == '\t' || buf[a] == '\r' || buf[a] == '\n')) a++;
        while (b > a && (buf[b - 1] == ' ' || buf[b - 1] == '\t' || buf[b - 1] == '\r' || buf[b - 1] == '\n')) b--;
        return String(buf + a, b - a);
    }
    String upper() const {
        String r(*this);
        for (int i = 0; i < r.len; i++) if (r.buf[i] >= 'a' && r.buf[i] <= 'z') r.buf[i] -= 32;
        return r;
    }
    // replaces every occurrence of a character
    String replaced(char from, char to) const {
        String r(*this);
        for (int i = 0; i < r.len; i++) if (r.buf[i] == from) r.buf[i] = to;
        return r;
    }
    long long toLong(bool* ok = 0) const {
        int i = 0;
        bool neg = false;
        long long v = 0;
        if (ok) *ok = false;
        if (i < len && (buf[i] == '-' || buf[i] == '+')) { neg = (buf[i] == '-'); i++; }
        if (i >= len) return 0;
        for (; i < len; i++) {
            if (buf[i] < '0' || buf[i] > '9') return 0;
            v = v * 10 + (buf[i] - '0');
        }
        if (ok) *ok = true;
        return neg ? -v : v;
    }
    double toDouble(bool* ok = 0) const {
        int i = 0;
        bool neg = false;
        double v = 0, scale = 1;
        bool seenDot = false, digits = false;
        if (ok) *ok = false;
        if (i < len && (buf[i] == '-' || buf[i] == '+')) { neg = (buf[i] == '-'); i++; }
        for (; i < len; i++) {
            if (buf[i] == '.' && !seenDot) { seenDot = true; continue; }
            if (buf[i] < '0' || buf[i] > '9') return 0;
            digits = true;
            if (seenDot) { scale /= 10; v += (buf[i] - '0') * scale; }
            else v = v * 10 + (buf[i] - '0');
        }
        if (!digits) return 0;
        if (ok) *ok = true;
        return neg ? -v : v;
    }
    static String fromInt(long long v) { String s; s.appendInt(v); return s; }
    static String fromDouble(double v, int decimals) { String s; s.appendDouble(v, decimals); return s; }

    // splits on a single character, keeps empty fields; returns number of parts
    static int split(const String& s, char delim, DynArray<String>& out) {
        out.clear();
        int start = 0;
        for (int i = 0; i <= s.len; i++) {
            if (i == s.len || s.buf[i] == delim) {
                out.push(String(s.buf + start, i - start));
                start = i + 1;
            }
        }
        return out.size();
    }
};

inline String operator+(const String& a, const String& b) { String r(a); r.append(b); return r; }
inline bool operator==(const String& a, const String& b) { return a.compare(b) == 0; }
inline bool operator!=(const String& a, const String& b) { return a.compare(b) != 0; }
inline bool operator<(const String& a, const String& b) { return a.compare(b) < 0; }
inline bool operator>(const String& a, const String& b) { return a.compare(b) > 0; }

// 4. Stack<T> and Queue<T> (singly linked)
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
    Stack(const Stack&);               // not copyable
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

template <class T>
struct QueueNode {
    T data;
    QueueNode* next;
    QueueNode(const T& d) : data(d), next(0) {}
};

template <class T>
class Queue {
private:
    QueueNode<T>* headNode;
    QueueNode<T>* tailNode;
    int count;
    Queue(const Queue&);
    Queue& operator=(const Queue&);
public:
    Queue() : headNode(0), tailNode(0), count(0) {}
    ~Queue() { while (!empty()) dequeue(); }
    void enqueue(const T& v) {
        QueueNode<T>* n = new QueueNode<T>(v);
        if (tailNode) tailNode->next = n; else headNode = n;
        tailNode = n;
        count++;
    }
    T dequeue() {
        T v = headNode->data;
        QueueNode<T>* n = headNode;
        headNode = headNode->next;
        if (!headNode) tailNode = 0;
        delete n;
        count--;
        return v;
    }
    T& front() { return headNode->data; }
    bool empty() const { return headNode == 0; }
    int size() const { return count; }
};

// 5. Doubly linked list
template <class T>
struct DNode {
    T data;
    DNode* prev;
    DNode* next;
    DNode(const T& d) : data(d), prev(0), next(0) {}
};

template <class T>
class DoublyLinkedList {
private:
    DNode<T>* headNode;
    DNode<T>* tailNode;
    int count;
    DoublyLinkedList(const DoublyLinkedList&);
    DoublyLinkedList& operator=(const DoublyLinkedList&);
public:
    DoublyLinkedList() : headNode(0), tailNode(0), count(0) {}
    ~DoublyLinkedList() { clear(); }

    DNode<T>* head() const { return headNode; }
    DNode<T>* tail() const { return tailNode; }
    int size() const { return count; }
    bool empty() const { return count == 0; }

    DNode<T>* pushBack(const T& v) {
        DNode<T>* n = new DNode<T>(v);
        n->prev = tailNode;
        if (tailNode) tailNode->next = n; else headNode = n;
        tailNode = n;
        count++;
        return n;
    }
    DNode<T>* pushFront(const T& v) {
        DNode<T>* n = new DNode<T>(v);
        n->next = headNode;
        if (headNode) headNode->prev = n; else tailNode = n;
        headNode = n;
        count++;
        return n;
    }
    DNode<T>* insertAfter(DNode<T>* pos, const T& v) {
        if (!pos) return pushFront(v);
        if (pos == tailNode) return pushBack(v);
        DNode<T>* n = new DNode<T>(v);
        n->prev = pos;
        n->next = pos->next;
        pos->next->prev = n;
        pos->next = n;
        count++;
        return n;
    }
    void remove(DNode<T>* n) {
        if (!n) return;
        if (n->prev) n->prev->next = n->next; else headNode = n->next;
        if (n->next) n->next->prev = n->prev; else tailNode = n->prev;
        delete n;
        count--;
    }
    void clear() { while (headNode) remove(headNode); }
};

// 6. MinHeap - binary heap of (key, value) pairs, smallest key on top
struct HeapItem {
    long long key;
    int value;
    HeapItem() : key(0), value(0) {}
    HeapItem(long long k, int v) : key(k), value(v) {}
};

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

// 7. Graph (adjacency lists) + Dijkstra "earliest arrival" search.
class EdgeModel {
public:
    virtual ~EdgeModel() {}
    // timeAtFrom : time we are standing at the origin node
    // isStart    : origin node is the search source
    // returns false if the edge cannot be used, else sets *arrivalAtTo
    virtual bool traverse(int edgeId, int fromNode, long long timeAtFrom,
                          bool isStart, long long* arrivalAtTo) = 0;
};

struct GraphEdge {
    int to;
    int id;                              // index of the record (e.g. flight) behind the edge
    GraphEdge() : to(0), id(0) {}
    GraphEdge(int t, int i) : to(t), id(i) {}
};

class Graph {
private:
    int n;
    DynArray<GraphEdge>* adj;
    Graph(const Graph&);
    Graph& operator=(const Graph&);
public:
    explicit Graph(int nodes) : n(nodes) { adj = new DynArray<GraphEdge>[n > 0 ? n : 1]; }
    ~Graph() { delete[] adj; }
    int nodeCount() const { return n; }
    void addEdge(int u, int v, int id) { adj[u].push(GraphEdge(v, id)); }
    const DynArray<GraphEdge>& edges(int u) const { return adj[u]; }

    // Dijkstra. arrival[], prevNode[], prevEdge[] must hold nodeCount() entries.
    bool earliestArrival(int src, int dst, long long startTime, EdgeModel* model,
                         int* prevNode, int* prevEdge, long long* arrival) const {
        const long long INF = 4000000000000000000LL;
        for (int i = 0; i < n; i++) { arrival[i] = INF; prevNode[i] = -1; prevEdge[i] = -1; }
        arrival[src] = startTime;
        MinHeap heap;
        heap.push(startTime, src);
        while (!heap.empty()) {
            HeapItem it = heap.pop();
            int u = it.value;
            if (it.key > arrival[u]) continue;           // stale entry
            if (u == dst) break;
            const DynArray<GraphEdge>& es = adj[u];
            for (int i = 0; i < es.size(); i++) {
                GraphEdge e = es[i];
                long long arr;
                if (!model->traverse(e.id, u, it.key, u == src, &arr)) continue;
                if (arr < arrival[e.to]) {
                    arrival[e.to] = arr;
                    prevNode[e.to] = u;
                    prevEdge[e.to] = e.id;
                    heap.push(arr, e.to);
                }
            }
        }
        return arrival[dst] < INF;
    }
};

}

#endif // DATA_STRUCTURES_CPP
