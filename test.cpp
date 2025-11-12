#include <iostream>
#include <cstring>
#include "core/fs_core.h"
#include "core/user_manager.h"
#include "core/file_manager.h"

const char* get_error_string(int code) {
    switch(code) {
        case 0: return "SUCCESS";
        case -1: return "ERROR_INVALID_OPERATION";
        case -2: return "ERROR_PERMISSION_DENIED";
        case -3: return "ERROR_NOT_FOUND";
        case -4: return "ERROR_FILE_EXISTS";
        default: return "UNKNOWN_ERROR";
    }
}

void print_test(const char* test_name, int status, bool should_succeed = true) {
    std::cout << "[" << test_name << "] Status: " << status 
              << " (" << get_error_string(status) << ")";
    if ((should_succeed && status == 0) || (!should_succeed && status != 0)) {
        std::cout << " ✓ PASS" << std::endl;
    } else {
        std::cout << " ✗ FAIL" << std::endl;
    }
}

int main() {
    void* fs_instance = nullptr;
    int status;

    std::cout << "========================================" << std::endl;
    std::cout << "  FILE MANAGER TEST SUITE" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;

    // --------------------------
    // 0. Format File System
    // --------------------------
    std::cout << "--- PHASE 1: File System Initialization ---" << std::endl;
    status = fs_format("student_test.omni", "default.uconf");
    print_test("FS Format", status);

    // --------------------------
    // 1. Initialize File System
    // --------------------------
    status = fs_init(&fs_instance, "student_test.omni", "default.uconf");
    print_test("FS Init", status);
    
    if (status != 0) {
        std::cerr << "Cannot continue without FS init." << std::endl;
        return -1;
    }

    // --------------------------
    // 2. Setup Users
    // --------------------------
    std::cout << "\n--- PHASE 2: User Setup ---" << std::endl;
    FSInstance* fs = static_cast<FSInstance*>(fs_instance);
    user_manager um(fs->users);
    
    void* admin_session = nullptr;
    status = um.user_login(&admin_session, "admin", "admin123");
    print_test("Admin Login", status);

    // Create a normal user
    void* user_session = nullptr;
    um.user_create(admin_session, "john_doe", "password123", UserRole::NORMAL);
    status = um.user_login(&user_session, "john_doe", "password123");
    print_test("User Login", status);

    // --------------------------
    // 3. File Manager Tests
    // --------------------------
    file_manager fm(fs, &um);
    
    std::cout << "\n--- PHASE 3: File Creation Tests ---" << std::endl;
    
    // Test 1: Create a file in root directory (using admin session)
    const char* file1 = "/test1.txt";
    const char* data1 = "Hello, OFS!";
    status = fm.file_create(admin_session, file1, data1, std::strlen(data1));
    print_test("Create /test1.txt", status);

    // Test 2: Create another file
    const char* file2 = "/document.txt";
    const char* data2 = "This is a test document.";
    status = fm.file_create(admin_session, file2, data2, std::strlen(data2));
    print_test("Create /document.txt", status);

    // Test 3: Try to create duplicate (should fail)
    status = fm.file_create(admin_session, file1, data1, std::strlen(data1));
    print_test("Create duplicate /test1.txt (should fail)", status, false);

    // --------------------------
    // 4. File Read Tests
    // --------------------------
    std::cout << "\n--- PHASE 4: File Read Tests ---" << std::endl;
    
    char* buffer = nullptr;
    size_t size = 0;
    status = fm.file_read(admin_session, file1, &buffer, &size);
    print_test("Read /test1.txt", status);
    if (status == 0 && buffer) {
        std::cout << "  Content: \"" << std::string(buffer, size) << "\"" << std::endl;
        std::cout << "  Size: " << size << " bytes" << std::endl;
        delete[] buffer;
    }

    // --------------------------
    // 5. File Exists Tests
    // --------------------------
    std::cout << "\n--- PHASE 5: File Exists Tests ---" << std::endl;
    
    status = fm.file_exists(admin_session, file1);
    print_test("Check /test1.txt exists", status);

    status = fm.file_exists(admin_session, "/nonexistent.txt");
    print_test("Check /nonexistent.txt exists (should fail)", status, false);

    // --------------------------
    // 6. File Edit Tests
    // --------------------------
    std::cout << "\n--- PHASE 6: File Edit Tests ---" << std::endl;
    
    // Edit file at index 7 (replace "OFS!" with "World!")
    const char* new_data = "World!";
    status = fm.file_edit(admin_session, file1, new_data, std::strlen(new_data), 7);
    print_test("Edit /test1.txt at index 7", status);

    // Read again to verify edit
    buffer = nullptr;
    size = 0;
    status = fm.file_read(admin_session, file1, &buffer, &size);
    if (status == 0 && buffer) {
        std::cout << "  After edit: \"" << std::string(buffer, size) << "\"" << std::endl;
        delete[] buffer;
    }

    // --------------------------
    // 7. File Rename Tests
    // --------------------------
    std::cout << "\n--- PHASE 7: File Rename Tests ---" << std::endl;
    
    const char* new_name = "/test1_renamed.txt";
    status = fm.file_rename(admin_session, file1, new_name);
    print_test("Rename /test1.txt to /test1_renamed.txt", status);

    // Verify old name doesn't exist
    status = fm.file_exists(admin_session, file1);
    print_test("Check old name /test1.txt (should not exist)", status, false);

    // Verify new name exists
    status = fm.file_exists(admin_session, new_name);
    print_test("Check new name /test1_renamed.txt exists", status);

    // --------------------------
    // 8. File Truncate Tests
    // --------------------------
    std::cout << "\n--- PHASE 8: File Truncate Tests ---" << std::endl;
    
    status = fm.file_truncate(admin_session, file2);
    print_test("Truncate /document.txt", status);

    // Read to verify truncation
    buffer = nullptr;
    size = 0;
    status = fm.file_read(admin_session, file2, &buffer, &size);
    if (status == 0) {
        std::cout << "  After truncate, size: " << size << " bytes" << std::endl;
        if (buffer) delete[] buffer;
    }

    // --------------------------
    // 9. File Delete Tests
    // --------------------------
    std::cout << "\n--- PHASE 9: File Delete Tests ---" << std::endl;
    
    status = fm.file_delete(admin_session, new_name);
    print_test("Delete /test1_renamed.txt", status);

    // Verify file no longer exists
    status = fm.file_exists(admin_session, new_name);
    print_test("Check deleted file (should not exist)", status, false);

    status = fm.file_delete(admin_session, file2);
    print_test("Delete /document.txt", status);

    // --------------------------
    // 10. Edge Case Tests
    // --------------------------
    std::cout << "\n--- PHASE 10: Edge Case Tests ---" << std::endl;
    
    // Try to read non-existent file
    buffer = nullptr;
    status = fm.file_read(admin_session, "/nonexistent.txt", &buffer, &size);
    print_test("Read non-existent file (should fail)", status, false);

    // Try to delete non-existent file
    status = fm.file_delete(admin_session, "/nonexistent.txt");
    print_test("Delete non-existent file (should fail)", status, false);

    // Create empty file
    status = fm.file_create(admin_session, "/empty.txt", nullptr, 0);
    print_test("Create empty file", status);

    // Read empty file
    buffer = nullptr;
    size = 0;
    status = fm.file_read(admin_session, "/empty.txt", &buffer, &size);
    if (status == 0) {
        std::cout << "  Empty file size: " << size << " bytes" << std::endl;
        if (buffer) delete[] buffer;
    }

    // --------------------------
    // 11. Cleanup
    // --------------------------
    std::cout << "\n--- PHASE 11: Cleanup ---" << std::endl;
    
    um.user_logout(user_session);
    std::cout << "User logged out." << std::endl;
    
    um.user_logout(admin_session);
    std::cout << "Admin logged out." << std::endl;
    
    fs_shutdown(fs_instance);
    std::cout << "FS shutdown completed." << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "  TEST SUITE COMPLETED" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}