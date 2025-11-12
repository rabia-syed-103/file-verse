#include <iostream>
#include <vector>
#include <cstring>
#include "core/fs_core.h"
#include "core/user_manager.h"

int main() {
    void* fs_instance = nullptr;

    // --------------------------
    // 0. Format File System
    // --------------------------
    int status = fs_format("student_test.omni", "default.uconf");
    if (status != 0) {
        std::cerr << "FS Format failed with code: " << status << std::endl;
        return -1;
    }
    std::cout << "FS formatted successfully.\n";

    // --------------------------
    // 1. Initialize File System
    // --------------------------
    status = fs_init(&fs_instance, "student_test.omni", "default.uconf");
    if (status != 0) {
        std::cerr << "FS Init failed with code: " << status << std::endl;
        return -1;
    }
    std::cout << "FS Initialized Successfully.\n";

    // --------------------------
    // 2. Create User Manager Object
    // --------------------------
    FSInstance* fs = static_cast<FSInstance*>(fs_instance);
user_manager um(fs->users);
    
    
    void* admin_session = nullptr;

status = um.user_login(&admin_session, "admin", "admin123");
std::cout << "Login status code: " << status << std::endl;
std::cout << "SUCCESS enum value: " << static_cast<int>(OFSErrorCodes::SUCCESS) << std::endl;
    // --------------------------
    // 3. Admin User Login
    // --------------------------
    status = um.user_login(&admin_session, "admin", "admin123");
    if (status != 0) {
       std::cerr << "Admin login failed.\n";
    return -1;
    }
    std::cout << "Admin logged in.\n";

    // --------------------------
    // 4. Create a Normal User
    // --------------------------
    status = um.user_create(admin_session, "john_doe", "password123", UserRole::NORMAL);
    if (status == 0) std::cout << "User john_doe created.\n";
    else std::cerr << "User creation failed: " << status << std::endl;

    // --------------------------
    // 5. List Users
    // --------------------------
    UserInfo* users = nullptr;
    int count = 0;
    status = um.user_list(admin_session, &users, &count);
    if (status == 0) {
        std::cout << "Users in system (" << count << "):\n";
        for (int i = 0; i < count; ++i)
            std::cout << " - " << users[i].username << "\n";
    }

    // --------------------------
    // 6. User Login
    // --------------------------
    void* user_session = nullptr;
    status = um.user_login(&user_session, "john_doe", "password123");
    if (status == 0) std::cout << "User john_doe logged in successfully.\n";
    else std::cerr << "User login failed: " << status << std::endl;

    // --------------------------
    // 7. Get Session Info
    // --------------------------
    SessionInfo info;
    status = um.get_session_info(user_session, &info);
    if (status == 0) {
        std::cout << "Session info for user: " << info.user.username
                  << ", login time: " << info.login_time << "\n";
    }

    // --------------------------
    // 8. Delete User
    // --------------------------
    status = um.user_delete(admin_session, "john_doe");
    if (status == 0) std::cout << "User john_doe deleted.\n";
    else std::cerr << "User deletion failed: " << status << std::endl;

    // --------------------------
    // 9. Logout Sessions
    // --------------------------
    um.user_logout(user_session);
    std::cout << "User john_doe logged out.\n";
    um.user_logout(admin_session);
    std::cout << "Admin logged out.\n";

    // --------------------------
    // 10. Shutdown File System
    // --------------------------
    fs_shutdown(fs_instance);
    std::cout << "FS shutdown completed.\n";

    return 0;
}
