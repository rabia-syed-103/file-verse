#include "file_manager.h"
#include <cstring>
#include <iostream>

using namespace std;

file_manager::file_manager(FSInstance* fs, user_manager* user_mgr)
    : fs_instance(fs), um(user_mgr) {}

file_manager::~file_manager() = default;

// ------------------ Utility ------------------

FSNode* file_manager::resolve_path(const std::string& path) {
    if (path.empty() || path[0] != '/') return nullptr;
    FSNode* node = fs_instance->root;
    size_t start = 1;
    while (start < path.size()) {
        size_t end = path.find('/', start);
        string token = path.substr(start, end - start);
        if (token.empty()) break;
        node = node->getChild(token);
        if (!node) return nullptr;
        if (end == string::npos) break;
        start = end + 1;
    }
    return node;
}

bool file_manager::check_permissions(void* session, FSNode* node) {
    if (!node || !session) return false;
    SessionInfo info;
    if (um->get_session_info(session, &info) != 0) return false;
    // Admin or owner can modify
    return info.user.role == UserRole::ADMIN || info.user.username == node->entry.owner;
}

// ------------------ File Operations ------------------

int file_manager::file_create(void* session, const char* path, const char* data, size_t size) {
    if (!session || !path) return static_cast<int>(OFSErrorCodes::ERROR_INVALID_OPERATION);
    string str_path(path);
    FSNode* parent = resolve_path(str_path.substr(0, str_path.find_last_of('/')));
    if (!parent) return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    if (!check_permissions(session, parent)) return static_cast<int>(OFSErrorCodes::ERROR_PERMISSION_DENIED);

    string filename = str_path.substr(str_path.find_last_of('/') + 1);
    if (parent->getChild(filename)) return static_cast<int>(OFSErrorCodes::ERROR_FILE_EXISTS);

    FileEntry entry(filename.c_str(), EntryType::FILE, fs_instance->next_file_index++, 0644,
                    "unknown", 0);
    FSNode* new_node = new FSNode(&entry, parent);
    parent->addChild(new_node);

    if (data && size > 0) {
        new_node->data.assign(data, data + size);
    }

    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_manager::file_read(void* session, const char* path, char** buffer, size_t* size) {
    FSNode* node = resolve_path(path);
    if (!node || node->entry.type != EntryType::FILE) return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
    *size = node->data.size();
    *buffer = new char[*size];
    memcpy(*buffer, node->data.data(), *size);
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_manager::file_edit(void* session, const char* path, const char* data, size_t size, uint index) {
    FSNode* node = resolve_path(path);
    if (!node || !check_permissions(session, node)) return static_cast<int>(OFSErrorCodes::ERROR_PERMISSION_DENIED);
    if (index > node->data.size()) return static_cast<int>(OFSErrorCodes::ERROR_INVALID_OPERATION);

    if (index + size > node->data.size())
        node->data.resize(index + size);
    memcpy(node->data.data() + index, data, size);

    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_manager::file_delete(void* session, const char* path) {
    FSNode* node = resolve_path(path);
    if (!node || !check_permissions(session, node)) return static_cast<int>(OFSErrorCodes::ERROR_PERMISSION_DENIED);

    FSNode* parent = node->parent;
    if (!parent) return static_cast<int>(OFSErrorCodes::ERROR_INVALID_OPERATION);

    parent->removeChild(node->entry.name);
    delete node;
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_manager::file_truncate(void* session, const char* path) {
    FSNode* node = resolve_path(path);
    if (!node || !check_permissions(session, node)) return static_cast<int>(OFSErrorCodes::ERROR_PERMISSION_DENIED);
    node->data.assign(node->data.size(), 's'); // fill file with 's'
    return static_cast<int>(OFSErrorCodes::SUCCESS);
}

int file_manager::file_exists(void* session, const char* path) {
    FSNode* node = resolve_path(path);
    return node ? static_cast<int>(OFSErrorCodes::SUCCESS) : static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);
}

int file_manager::file_rename(void* session, const char* old_path, const char* new_path) {
    FSNode* node = resolve_path(old_path);
    if (!node || !check_permissions(session, node)) return static_cast<int>(OFSErrorCodes::ERROR_PERMISSION_DENIED);

    FSNode* new_parent = resolve_path(string(new_path).substr(0, string(new_path).find_last_of('/')));
    if (!new_parent) return static_cast<int>(OFSErrorCodes::ERROR_NOT_FOUND);

    string new_name = string(new_path).substr(string(new_path).find_last_of('/') + 1);
    node->parent->removeChild(node->entry.name);
    node->entry.name = new_name;
    new_parent->addChild(node);
    node->parent = new_parent;

    return static_cast<int>(OFSErrorCodes::SUCCESS);
}
