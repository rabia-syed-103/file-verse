#include "fs_core.h"
#include <fstream>
#include <cstring>
#include <ctime>
#include <iostream>

int fs_format(const char* omni_path, const char* config_path) {
    std::ofstream ofs(omni_path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);

    OMNIHeader header(0x00010000, 1024ULL*1024*50, sizeof(OMNIHeader), 4096);
    std::strncpy(header.magic, "OMNIFS01", sizeof(header.magic)-1);
    std::memset(header.config_hash, 0, sizeof(header.config_hash));
    header.config_timestamp = std::time(nullptr);
    header.user_table_offset = sizeof(OMNIHeader);
    header.max_users = 1024;

    ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

    UserInfo empty_user;
    for (uint32_t i = 0; i < header.max_users; ++i)
        ofs.write(reinterpret_cast<const char*>(&empty_user), sizeof(UserInfo));

    
    FileEntry root_entry("root", EntryType::DIRECTORY, 0, 0755, "admin", 0);
    ofs.write(reinterpret_cast<const char*>(&root_entry), sizeof(FileEntry));

    uint64_t total_blocks = header.total_size / header.block_size;
    FreeSpaceManager fsm(total_blocks);
    uint64_t used_blocks = (sizeof(OMNIHeader) + header.max_users*sizeof(UserInfo) + sizeof(FileEntry) + header.block_size - 1) / header.block_size;
    for (uint64_t i = 0; i < used_blocks; ++i) fsm.markUsed(i);

    ofs.close();
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int fs_init(void** instance, const char* omni_path, const char* config_path) {
    std::ifstream ifs(omni_path, std::ios::binary);
    if (!ifs.is_open()) return static_cast<int>(OFSErrorCodes::ERROR_IO_ERROR);

    FSInstance* fs = new FSInstance();
    fs->omni_path = omni_path;

    
    ifs.read(reinterpret_cast<char*>(&fs->header), sizeof(OMNIHeader));


    fs->users = new HashTable<UserInfo>(fs->header.max_users);
    ifs.seekg(fs->header.user_table_offset, std::ios::beg);
    for (uint32_t i = 0; i < fs->header.max_users; ++i) {
        UserInfo u;
        ifs.read(reinterpret_cast<char*>(&u), sizeof(UserInfo));
        if (u.is_active)
            fs->users->insert(u.username, new UserInfo(u));
    }

    FileEntry root_entry;
    ifs.read(reinterpret_cast<char*>(&root_entry), sizeof(FileEntry));
    fs->root = new FSNode(new FileEntry(root_entry));

    
    uint64_t total_blocks = fs->header.total_size / fs->header.block_size;
    fs->fsm = new FreeSpaceManager(total_blocks);

    *instance = fs;
    ifs.close();
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

void fs_shutdown(void* instance) {
    if (!instance) return;
    FSInstance* fs = static_cast<FSInstance*>(instance);

    delete fs->fsm;
    delete fs->root;
    delete fs->users;
    for (auto session : fs->sessions) delete session;
    delete fs;
}

