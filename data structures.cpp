#include <bits./stdc++.h>
using namespace std;
namespace DataStructures{
    class Node{
    public:
        int data;
        Node* next;
        Node(int val) : data(val), next(nullptr) {}
    };
    class LinkedList{
        private:
            Node *head;
        public:
            LinkedList() : head(nullptr) {}
            ~LinkedList(){
                cout << "Destructor Called" << endl;
                Node *itr = head;
                while(itr){
                    Node *next = itr->next;
                    delete itr;
                    itr = next;
                }
            }
            void insertAtBegin(int val){
                Node *newNode = new Node(val);
                newNode->next = head;
                head = newNode;
            }
            void insertAtEnd(int val){
                Node *newNode = new Node(val);
                if(head == nullptr){
                    head = newNode;
                    return;
                }
                Node *itr = head;
                while(itr->next)
                    itr = itr->next;
                itr->next = newNode;
            }
            void insertAtPositionK(int k, int val){
                Node *newnode = new Node(val);
                if(head == nullptr){
                    head = newnode;
                    return;
                }
                Node *itr = head;
                int i = 1;
                while(itr->next && i < k){
                    itr = itr->next;
                    i++;
                }
                newnode->next = itr->next;
                itr->next = newnode;
            }
            void deleteAtBegin(){
                if(head == nullptr)
                    return;
                Node *temp = head;
                head = head->next;
                delete temp;
            }
            void deleteAtEnd(){
                if(head == nullptr)
                    return;
                if(head->next == nullptr){
                    delete head;
                    head = nullptr;
                }
                Node *itr = head;
                while(itr->next->next)
                    itr = itr->next;
                Node *temp = itr->next;
                itr->next = itr->next->next;
                delete temp;
            }
            void deleteValueK(int k){
                if(head == nullptr)
                    return;
                if(head->data == k){
                    Node *temp = head;
                    head = head->next;
                    delete temp;
                    return;
                }
                Node *itr = head;
                while(itr->next->data != k && itr->next)
                    itr = itr->next;
                Node *temp = itr->next;
                itr->next = itr->next->next;
                delete temp;
            }
            void print(){
                Node *itr = head;
                while(itr){
                    cout << itr->data << "->";
                    itr = itr->next;
                }
                cout << endl;
            }
    };
    // class Stack{
    //     private:
    //         int *top;
    //     public:
    //         Stack(){
    //             LinkedList *l1 = new LinkedList;
    //             top = l1->head;
    //         }
    //         ~Stack() : {}
    //         void push(int val){
    //             l1-
    //         }
    // }
}
using namespace DataStructures;
int main(){
    LinkedList *l1 = new LinkedList;
    l1->insertAtBegin(1);
    l1->print();
    l1->insertAtBegin(2);
    l1->print();
    l1->insertAtEnd(3);
    l1->print();
    l1->insertAtEnd(4);
    l1->print();
    l1->insertAtBegin(6);
    l1->print();
    l1->deleteAtBegin();
    l1->print();
    l1->deleteAtEnd();
    l1->print();
    l1->deleteValueK(2);
    l1->print();
    //delete l1;
}