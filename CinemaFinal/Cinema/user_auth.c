#include "headers.h"

static int email_exists(char *email) {
    UserNode *p = user_list;
    while (p) {
        if (strcmp(p->data.email, email) == 0) return 1;
        p = p->next;
    }
    return 0;
}

static int is_valid_email(char *email) {
    int len = (int)strlen(email);
    char *at_pos = strchr(email, '@');
    char *dot_pos = strrchr(email, '.');
    if (!at_pos || !dot_pos) return 0;
    if (dot_pos < at_pos) return 0;
    if (dot_pos - at_pos < 2) return 0;
    if (len - (dot_pos - email) < 3) return 0;
    if (at_pos == email || dot_pos == email + len - 1) return 0;
    return 1;
}

void user_register() {
    printf("\n========== User Registration ==========\n");
    User u;
    char input[200];

    printf("Enter your name: ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, " %[^\n]", u.name);
    if (strlen(u.name) == 0) {
        printf("Name cannot be empty.\n");
        return;
    }

    printf("Enter your email: ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, " %[^\n]", u.email);
    if (!is_valid_email(u.email)) {
        printf("Invalid email format.\n");
        return;
    }
    if (email_exists(u.email)) {
        printf("An account with this email already exists.\n");
        return;
    }

    printf("Enter password (min 6 chars): ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, " %[^\n]", u.password);
    if (strlen(u.password) < 6) {
        printf("Password must be at least 6 characters.\n");
        return;
    }
    /* Encrypt (salted SHA-256 hash) before the password ever touches disk. */
    hash_password(u.email, u.password, u.password);

    printf("Enter phone number: ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, " %[^\n]", u.phone);

    u.is_admin = 0;

    UserNode *node = (UserNode*)malloc(sizeof(UserNode));
    node->data = u;
    node->next = NULL;

    if (!user_list) {
        user_list = node;
    } else {
        UserNode *p = user_list;
        while (p->next) p = p->next;
        p->next = node;
    }

    save_all_data();
    printf("\nRegistration successful! You can now login.\n");
}

int user_login() {
    printf("\n========== User Login ==========\n");
    char email[MAX_EMAIL], password[MAX_PASSWORD], input[200];
    int attempts = 0;

    while (attempts < 3) {
        printf("Enter email: ");
        fgets(input, sizeof(input), stdin);
        sscanf(input, " %[^\n]", email);

        printf("Enter password: ");
        fgets(input, sizeof(input), stdin);
        sscanf(input, " %[^\n]", password);

        UserNode *p = user_list;
        while (p) {
            if (strcmp(p->data.email, email) == 0 &&
                verify_password(email, password, p->data.password)) {
                /* Upgrade legacy plain-text passwords to hashed on first login. */
                if (!password_is_hashed(p->data.password)) {
                    hash_password(email, password, p->data.password);
                    save_all_data();
                }
                current_user = p->data;
                is_logged_in = 1;
                printf("\nWelcome, %s!\n", current_user.name);
                return 1;
            }
            p = p->next;
        }

        attempts++;
        if (attempts < 3) {
            printf("\nInvalid email or password. %d attempt(s) remaining.\n", 3 - attempts);
        }
    }
    printf("\nToo many failed login attempts. Try again later.\n");
    return 0;
}

void user_logout() {
    is_logged_in = 0;
    memset(&current_user, 0, sizeof(User));
    printf("\nYou have been logged out successfully.\n");
}

void view_profile() {
    if (!is_logged_in) {
        printf("Please login first.\n");
        return;
    }
    printf("\n========== My Profile ==========\n");
    printf("Name:    %s\n", current_user.name);
    printf("Email:   %s\n", current_user.email);
    printf("Phone:   %s\n", current_user.phone);
    printf("Role:    %s\n", current_user.is_admin ? "Administrator" : "Customer");

    int count = 0;
    BookingNode *bp = booking_list;
    while (bp) {
        if (strcmp(bp->data.user_email, current_user.email) == 0) count++;
        bp = bp->next;
    }
    printf("Bookings: %d total\n", count);
}

void update_profile() {
    if (!is_logged_in) {
        printf("Please login first.\n");
        return;
    }
    printf("\n========== Update Profile ==========\n");
    char input[200];

    printf("Current name: %s\n", current_user.name);
    printf("Enter new name (leave blank to keep): ");
    fgets(input, sizeof(input), stdin);
    char new_name[MAX_NAME];
    sscanf(input, " %[^\n]", new_name);
    if (strlen(new_name) > 0) {
        strcpy(current_user.name, new_name);
    }

    printf("Current phone: %s\n", current_user.phone);
    printf("Enter new phone (leave blank to keep): ");
    fgets(input, sizeof(input), stdin);
    char new_phone[15];
    sscanf(input, " %[^\n]", new_phone);
    if (strlen(new_phone) > 0) {
        strcpy(current_user.phone, new_phone);
    }

    printf("Enter new password (leave blank to keep): ");
    fgets(input, sizeof(input), stdin);
    char new_pwd[MAX_PASSWORD];
    sscanf(input, " %[^\n]", new_pwd);
    if (strlen(new_pwd) >= 6) {
        hash_password(current_user.email, new_pwd, current_user.password);
    } else if (strlen(new_pwd) > 0) {
        printf("Password too short, keeping original.\n");
    }

    UserNode *p = user_list;
    while (p) {
        if (strcmp(p->data.email, current_user.email) == 0) {
            p->data = current_user;
            break;
        }
        p = p->next;
    }
    save_all_data();
    printf("\nProfile updated successfully.\n");
}
