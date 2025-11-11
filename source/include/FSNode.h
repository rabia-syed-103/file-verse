#ifndef FSNODE_H
#define FSNODE_H
#include <string>
#include <iostream>
#include "odf_types.hpp"
#include "LinkedList.h"
using namespace std;

class FSNode {
public:
    FileEntry* entry;         
    LinkedList<FSNode>* children; 

    FSNode(FileEntry* e) : entry(e) {
        if (entry->getType() == EntryType::DIRECTORY)
            children = new LinkedList<FSNode>();
        else
            children = nullptr;
    }

    ~FSNode() {
        if (children) 
            delete children;
        if (entry) 
            delete entry;
    }

    void addChild(FSNode* child) {
        if (entry->getType() == EntryType::DIRECTORY)
            children->push_back(child);
    }

    void removeChild(FSNode* child) {
        if (entry->getType() == EntryType::DIRECTORY)
            children->remove(child);
    }

    FSNode* findChild(const string& name) {
        if (!children) return nullptr;
        LinkedListNode<FSNode>* node = children->getHead();
        while (node) {
            if (name == node->data->entry->name)
                return node->data;
            node = node->next;
        }
        return nullptr;
    }
};

#endif