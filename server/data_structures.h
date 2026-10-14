#pragma once
#include <iostream>
#include <cstring>
#include <string>
using namespace std;

namespace DS {

// ---------- Singly Linked List ----------
template<typename T>
class SNode {
public:
    T data; SNode* next;
    SNode(const T& d) : data(d), next(nullptr) {}
};

template<typename T>
class SinglyLinkedList {
    SNode<T>* head; int sz;
public:
    SinglyLinkedList() : head(nullptr), sz(0) {}
    ~SinglyLinkedList() { clear(); }
    void clear() {
        while (head) { SNode<T>* t = head; head = head->next; delete t; }
        sz = 0;
    }
    void push_front(const T& v) {
        SNode<T>* n = new SNode<T>(v); n->next = head; head = n; sz++;
    }
    void push_back(const T& v) {
        SNode<T>* n = new SNode<T>(v);
        if (!head) { head = n; sz++; return; }
        SNode<T>* it = head; while (it->next) it = it->next;
        it->next = n; sz++;
    }
    bool remove(const T& v) {
        if (!head) return false;
        if (head->data == v) { SNode<T>* t = head; head = head->next; delete t; sz--; return true; }
        SNode<T>* it = head;
        while (it->next && !(it->next->data == v)) it = it->next;
        if (!it->next) return false;
        SNode<T>* t = it->next; it->next = t->next; delete t; sz--; return true;
    }
    T* find(const T& v) {
        SNode<T>* it = head;
        while (it) { if (it->data == v) return &it->data; it = it->next; }
        return nullptr;
    }
    int size() const { return sz; }
    SNode<T>* getHead() const { return head; }
};

// ---------- Doubly Linked List ----------
template<typename T>
class DNode {
public:
    T data; DNode* prev; DNode* next;
    DNode(const T& d) : data(d), prev(nullptr), next(nullptr) {}
};

template<typename T>
class DoublyLinkedList {
    DNode<T>* head; DNode<T>* tail; int sz;
public:
    DoublyLinkedList() : head(nullptr), tail(nullptr), sz(0) {}
    ~DoublyLinkedList() { clear(); }
    void clear() {
        DNode<T>* it = head;
        while (it) { DNode<T>* n = it->next; delete it; it = n; }
        head = tail = nullptr; sz = 0;
    }
    void push_back(const T& v) {
        DNode<T>* n = new DNode<T>(v);
        if (!head) { head = tail = n; }
        else { tail->next = n; n->prev = tail; tail = n; }
        sz++;
    }
    DNode<T>* findNode(const T& v) {
        DNode<T>* it = head;
        while (it) { if (it->data == v) return it; it = it->next; }
        return nullptr;
    }
    bool removeNode(DNode<T>* n) {
        if (!n) return false;
        if (n->prev) n->prev->next = n->next; else head = n->next;
        if (n->next) n->next->prev = n->prev; else tail = n->prev;
        delete n; sz--; return true;
    }
    int size() const { return sz; }
    DNode<T>* getHead() const { return head; }
    DNode<T>* getTail() const { return tail; }
};

// ---------- Stack ----------
template<typename T>
class Stack {
    T* arr; int cap, top;
public:
    Stack(int c = 200) : cap(c), top(-1) { arr = new T[cap]; }
    ~Stack() { delete[] arr; }
    bool push(const T& v) { if (top >= cap-1) return false; arr[++top] = v; return true; }
    bool pop(T& out) { if (top < 0) return false; out = arr[top--]; return true; }
    bool peek(T& out) const { if (top < 0) return false; out = arr[top]; return true; }
    bool isEmpty() const { return top < 0; }
    int size() const { return top+1; }
};

// ---------- Circular Queue ----------
template<typename T>
class CircularQueue {
    T* arr; int cap, front, rear, cnt;
public:
    CircularQueue(int c = 200) : cap(c), front(0), rear(-1), cnt(0) { arr = new T[cap]; }
    ~CircularQueue() { delete[] arr; }
    bool enqueue(const T& v) {
        if (cnt >= cap) return false;
        rear = (rear+1) % cap; arr[rear] = v; cnt++; return true;
    }
    bool dequeue(T& out) {
        if (cnt == 0) return false;
        out = arr[front]; front = (front+1) % cap; cnt--; return true;
    }
    bool isEmpty() const { return cnt == 0; }
    int size() const { return cnt; }
};

// ---------- Priority Queue (min-heap) ----------
template<typename T>
class PQItem { public: T data; int pri; PQItem() : pri(0) {} PQItem(const T& d, int p) : data(d), pri(p) {} };

template<typename T>
class PriorityQueue {
    PQItem<T>* arr; int cap, sz;
    void up(int i) {
        while (i > 0) {
            int p = (i-1)/2;
            if (arr[p].pri > arr[i].pri) { PQItem<T> t = arr[p]; arr[p] = arr[i]; arr[i] = t; i = p; }
            else break;
        }
    }
    void down(int i) {
        while (true) {
            int l = 2*i+1, r = 2*i+2, s = i;
            if (l < sz && arr[l].pri < arr[s].pri) s = l;
            if (r < sz && arr[r].pri < arr[s].pri) s = r;
            if (s == i) break;
            PQItem<T> t = arr[s]; arr[s] = arr[i]; arr[i] = t; i = s;
        }
    }
public:
    PriorityQueue(int c = 500) : cap(c), sz(0) { arr = new PQItem<T>[cap]; }
    ~PriorityQueue() { delete[] arr; }
    bool push(const T& d, int p) {
        if (sz >= cap) return false;
        arr[sz] = PQItem<T>(d, p); up(sz++); return true;
    }
    bool pop(T& out) {
        if (sz == 0) return false;
        out = arr[0].data; arr[0] = arr[--sz]; down(0); return true;
    }
    bool isEmpty() const { return sz == 0; }
    int size() const { return sz; }
};

// ---------- Hash Table ----------
template<typename V>
class HashEntry { public: char key[64]; V value; bool used; HashEntry() : used(false) { key[0]=0; } };

template<typename V>
class HashTable {
    HashEntry<V>* table; int cap, sz;
    unsigned int hash(const char* s) const {
        unsigned int h = 5381;
        while (*s) h = ((h << 5) + h) + (unsigned char)(*s++);
        return h % cap;
    }
public:
    HashTable(int c = 2003) : cap(c), sz(0) { table = new HashEntry<V>[cap]; }
    ~HashTable() { delete[] table; }
    bool put(const char* k, const V& v) {
        if (sz >= cap) return false;
        int idx = hash(k);
        while (table[idx].used && strcmp(table[idx].key, k) != 0) idx = (idx+1) % cap;
        if (!table[idx].used) {
            strncpy(table[idx].key, k, 63); table[idx].key[63]=0;
            table[idx].used = true; sz++;
        }
        table[idx].value = v; return true;
    }
    V* get(const char* k) {
        int idx = hash(k), start = idx;
        while (table[idx].used) {
            if (strcmp(table[idx].key, k) == 0) return &table[idx].value;
            idx = (idx+1) % cap;
            if (idx == start) break;
        }
        return nullptr;
    }
    HashEntry<V>* raw() { return table; }
    int capacity() const { return cap; }
    int size() const { return sz; }
};

// ---------- BST ----------
template<typename K, typename V>
class BSTNode { public: K key; V value; BSTNode* left; BSTNode* right;
    BSTNode(const K& k, const V& v) : key(k), value(v), left(nullptr), right(nullptr) {} };

template<typename K, typename V>
class BST {
    BSTNode<K,V>* root;
    BSTNode<K,V>* ins(BSTNode<K,V>* n, const K& k, const V& v) {
        if (!n) return new BSTNode<K,V>(k, v);
        if (k < n->key) n->left = ins(n->left, k, v);
        else if (k > n->key) n->right = ins(n->right, k, v);
        else n->value = v;
        return n;
    }
    BSTNode<K,V>* srch(BSTNode<K,V>* n, const K& k) {
        if (!n) return nullptr;
        if (k == n->key) return n;
        return k < n->key ? srch(n->left, k) : srch(n->right, k);
    }
    void inorder(BSTNode<K,V>* n, void (*cb)(const K&, const V&)) {
        if (!n) return;
        inorder(n->left, cb); cb(n->key, n->value); inorder(n->right, cb);
    }
    void destroy(BSTNode<K,V>* n) { if (!n) return; destroy(n->left); destroy(n->right); delete n; }
public:
    BST() : root(nullptr) {}
    ~BST() { destroy(root); }
    void insert(const K& k, const V& v) { root = ins(root, k, v); }
    V* find(const K& k) { auto n = srch(root, k); return n ? &n->value : nullptr; }
    void traverse(void (*cb)(const K&, const V&)) { inorder(root, cb); }
};

// ---------- Graph (adjacency list) ----------
class GraphEdge { public: int to; double weight; char flightNum[16]; GraphEdge* next;
    GraphEdge(int t, double w, const char* fn) : to(t), weight(w), next(nullptr) {
        strncpy(flightNum, fn, 15); flightNum[15]=0; } };

class Graph {
public:
    GraphEdge** adj; int numNodes;
    Graph(int n) : numNodes(n) {
        adj = new GraphEdge*[n];
        for (int i = 0; i < n; i++) adj[i] = nullptr;
    }
    ~Graph() {
        for (int i = 0; i < numNodes; i++) {
            GraphEdge* e = adj[i];
            while (e) { GraphEdge* t = e->next; delete e; e = t; }
        }
        delete[] adj;
    }
    void addEdge(int u, int v, double w, const char* fn) {
        GraphEdge* e = new GraphEdge(v, w, fn);
        e->next = adj[u]; adj[u] = e;
    }
};

} // namespace DS