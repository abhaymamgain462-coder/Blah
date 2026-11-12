#include "headers.h"

static void print_banner() {
    printf("\n");
    printf("  ********************************************************************\n");
    printf("  *                                                                  *\n");
    printf("  *                    C I N E M A   S Y S T E M                     *\n");
    printf("  *              Movie Booking & Management Console                 *\n");
    printf("  *                                                                  *\n");
    printf("  ********************************************************************\n");
    printf("\n");
}

void user_menu() {
    while (1) {
        printf("\n============================================\n");
        printf("           CUSTOMER MAIN MENU              \n");
        printf("  Welcome, %s!                     \n", current_user.name);
        if (current_user.is_admin) {
            printf("  Role: Administrator                      \n");
        } else {
            printf("  Role: Customer                           \n");
        }
        printf("============================================\n");
        printf("1. Browse Now Showing Movies\n");
        printf("2. Book Movie Tickets\n");
        printf("3. My Bookings\n");
        printf("4. Cancel a Booking\n");
        printf("5. My Profile\n");
        printf("6. Update Profile\n");
        if (current_user.is_admin) {
            printf("7. Admin Dashboard\n");
            printf("8. Logout\n");
            printf("============================================\n");
            printf("Enter choice: ");
            int ch = get_valid_int(1, 8);
            switch (ch) {
                case 1: list_movies(0); break;
                case 2: book_ticket(); break;
                case 3: view_my_bookings(); break;
                case 4: cancel_booking(); break;
                case 5: view_profile(); break;
                case 6: update_profile(); break;
                case 7: admin_dashboard(); break;
                case 8: user_logout(); return;
            }
        } else {
            printf("7. Logout\n");
            printf("============================================\n");
            printf("Enter choice: ");
            int ch = get_valid_int(1, 7);
            switch (ch) {
                case 1: list_movies(0); break;
                case 2: book_ticket(); break;
                case 3: view_my_bookings(); break;
                case 4: cancel_booking(); break;
                case 5: view_profile(); break;
                case 6: update_profile(); break;
                case 7: user_logout(); return;
            }
        }
    }
}

void main_menu() {
    while (1) {
        print_banner();
        printf("  1. Register New Account\n");
        printf("  2. Login to Account\n");
        printf("  3. Browse Movies (Guest)\n");
        printf("  4. Exit System\n");
        printf("\n");
        printf("  ***************************************************************\n");
        printf("  *  Default Admin: admin@cinema.com  /  admin123               *\n");
        printf("  ***************************************************************\n");
        printf("\n  Enter your choice (1-4): ");

        int ch = get_valid_int(1, 4);
        switch (ch) {
            case 1:
                user_register();
                printf("\nPress Enter to continue...");
                clear_input_buffer();
                break;
            case 2:
                if (user_login()) {
                    user_menu();
                }
                printf("\nPress Enter to continue...");
                clear_input_buffer();
                break;
            case 3:
                printf("\n--- Movies Now Showing (Guest View) ---\n");
                list_movies(0);
                printf("\nGuest: Please login or register to book tickets.\n");
                printf("\nPress Enter to continue...");
                clear_input_buffer();
                break;
            case 4:
                printf("\n  Thank you for using Cinema System! Goodbye.\n\n");
                return;
        }
    }
}

int main() {
    init_system();
    main_menu();
    return 0;
}
