#ifndef FS_CORE_H
#define FS_CORE_H

#include "odf_types.hpp"
#include "FSNode.h"
#include "HashTable.h"
#include "FreeSpaceManager.h"
#include <vector>
#include <string>

using namespace std;
struct FSInstance {
    OMNIHeader header;
    HashTable<UserInfo>* users;
    FSNode* root;                 
    FreeSpaceManager* fsm;        
    vector<SessionInfo*> sessions; 
    string omni_path;
};

// Core system functions
int fs_format(const char* omni_path, const char* config_path);
int fs_init(void** instance, const char* omni_path, const char* config_path);
void fs_shutdown(void* instance);

#endif
