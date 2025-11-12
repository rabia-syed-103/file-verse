#include <iostream>
#include <cstring>
#include "core/fs_core.h"
#include "core/user_manager.h"
#include "core/file_manager.h"
#include "core/dir_manager.h"
#include "core/metadata.h"

using namespace std;

const char* get_error_string(int code) {
    switch (code) {
        case 0: return "SUCCESS";
        case -1: return "ERROR_NOT_FOUND";
        case -2: return "ERROR_PERMISSION_DENIED";
        case -3: return "ERROR_IO_ERROR";
        case -4: return "ERROR_INVALID_PATH";
        case -5: return "ERROR_DIRECTORY_NOT_EMPTY";
        default: return "UNKNOWN_ERROR";
    }
}

void print_test(const char* name, int status, bool should_succeed = true) {
    cout << "[" << name << "] Status: " << status
         << " (" << get_error_string(status) << ")";
    if ((should_succeed && status == 0) || (!should_succeed && status != 0))
        cout << " ✓ PASS" << endl;
    else
        cout << " ✗ FAIL" << endl;
}

int main() {
    void* fs_instance = nullptr;
    int status;

    cout << "========================================\n";
    cout << "       METADATA + FS TESTER\n";
    cout << "========================================\n\n";

    // --------------------------
    // FS Setup
    // --------------------------
    status = fs_format("tester.omni", "default.uconf");
    print_test("FS Format", status);

    status = fs_init(&fs_instance, "tester.omni", "default.uconf");
    print_test("FS Init", status);
    if (status != 0) return -1;

    FSInstance* fs = static_cast<FSInstance*>(fs_instance);
    user_manager um(fs->users);
    file_manager fm(fs, &um);
    dir_manager dm(fs->root, &um);
    metadata md(fs);

    // --------------------------
    // Admin Login
    // --------------------------
    void* admin_session = nullptr;
    status = um.user_login(&admin_session, "admin", "admin123");
    print_test("Admin Login", status);
    if (!admin_session) return -1;

    // --------------------------
    // Admin: Create files & directories
    // --------------------------
    const char* root_file = "/f1.txt";
    const char* root_data = "Hello I am F1";
    status = fm.file_create(admin_session, root_file, root_data, strlen(root_data));
    print_test("Admin Create /f1.txt", status);

    status = dm.dir_create(admin_session, "/dir1");
    print_test("Admin Create /dir1", status);

    const char* nested_file = "/dir1/f1.txt";
    const char* nested_data = "Hello I am F1 under Dir 1";
    status = fm.file_create(admin_session, nested_file, nested_data, strlen(nested_data));
    print_test("Admin Create /dir1/f1.txt", status);

    // --------------------------
    // Metadata tests (admin)
    // --------------------------
    FileMetadata fmeta;
    status = md.get_metadata(admin_session, root_file, &fmeta);
    print_test("Get Metadata /f1.txt", status);

    FSStats stats;
    status = md.get_stats(admin_session, &stats);
    print_test("Get FS Stats", status);

    // --------------------------
    // Admin: Create home directories & normal user
    // --------------------------
    status = dm.dir_create(admin_session, "/home");
    print_test("Admin Create /home", status);

    status = dm.dir_create(admin_session, "/home/john_doe");
    print_test("Admin Create /home/john_doe", status);

    status = um.user_create(admin_session, "john_doe", "password123", UserRole::NORMAL);
    print_test("Create Normal User", status);

    // Set ownership to the normal user (admin session required)
    status = md.set_owner(admin_session, "/home/john_doe", "john_doe");
    print_test("Set Owner /home/john_doe", status);

    // Set permissions so normal user can read/write/execute
    uint32_t home_perms =
        static_cast<uint32_t>(FilePermissions::OWNER_READ) |
        static_cast<uint32_t>(FilePermissions::OWNER_WRITE) |
        static_cast<uint32_t>(FilePermissions::OWNER_EXECUTE);
    status = md.set_permissions(admin_session, "/home/john_doe", home_perms);
    print_test("Set Permissions /home/john_doe", status);

    // --------------------------
    // Normal User Login
    // --------------------------
    void* user_session = nullptr;
    status = um.user_login(&user_session, "john_doe", "password123");
    print_test("Normal User Login", status);
    if (!user_session) return -1;

    // --------------------------
    // Normal User: Create a file in home
    // --------------------------
    const char* user_file = "/home/john_doe/user_file.txt";
    const char* user_data = "Hello I am John Doe";

    status = fm.file_create(user_session, user_file, user_data, strlen(user_data));
    print_test("User Create /home/john_doe/user_file.txt", status);

    // Read it back
    char* buffer = nullptr;
    size_t size = 0;
    status = fm.file_read(user_session, user_file, &buffer, &size);
    print_test("User Read /home/john_doe/user_file.txt", status);
    if (status == 0 && buffer) {
        cout << "  Content: \"" << string(buffer, size) << "\"" << endl;
        delete[] buffer;
    }

    // Try deleting admin directory (should fail)
    status = dm.dir_delete(user_session, "/dir1");
    print_test("User Delete /dir1 (should fail)", status, false);

    // Delete own file
    status = fm.file_delete(user_session, user_file);
    print_test("User Delete /home/john_doe/user_file.txt", status);

    // --------------------------
    // Logout normal user
    // --------------------------
    um.user_logout(user_session);
    cout << "Normal user logged out." << endl;

    // --------------------------
    // Cleanup admin files
    // --------------------------
    status = fm.file_delete(admin_session, nested_file);
    print_test("Delete /dir1/f1.txt", status);

    status = dm.dir_delete(admin_session, "/dir1");
    print_test("Delete /dir1", status);

    status = fm.file_delete(admin_session, root_file);
    print_test("Delete /f1.txt", status);

    um.user_logout(admin_session);
    cout << "Admin logged out." << endl;

    fs_shutdown(fs_instance);
    cout << "FS shutdown completed." << endl;

    cout << "\n========================================" << endl;
    cout << "         METADATA + FS TEST DONE\n";
    cout << "========================================" << endl;

    return 0;
}



//g++ -std=c++17 -Isource/include -Isource/include/core source/core/*.cpp source/*.cpp test.cpp -o test_ofs -lssl -lcrypto