#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_NAME 50
#define MAX_EMAIL 80
#define MAX_PASSWORD 30
#define MAX_TITLE 100
#define MAX_GENRE 30
#define MAX_TIME 10
#define ROWS 8
#define COLS 10
#define SEAT_PRICE 150.0
#define ADMIN_CODE "ADMIN2026"
#define MAX_PAY_DETAILS 48
#define MAX_PAY_METHOD_NAME 16

/* Payment methods */
enum PaymentMethod {
    PAY_METHOD_NONE = 0,
    PAY_METHOD_CARD = 1,
    PAY_METHOD_UPI = 2,
    PAY_METHOD_WALLET = 3,
    PAY_METHOD_CASH = 4
};

/* Payment statuses */
enum PaymentStatus {
    PAYMENT_STATUS_COMPLETED = 1,
    PAYMENT_STATUS_REFUNDED = 2
};

typedef struct {
    int payment_id;
    int booking_id;
    char user_email[MAX_EMAIL];
    double amount;
    int method;                      /* enum PaymentMethod */
    char details[MAX_PAY_DETAILS];   /* masked card / UPI id / wallet id */
    int status;                      /* enum PaymentStatus */
    char paid_at[30];
} Payment;

typedef struct PaymentNode {
    Payment data;
    struct PaymentNode *next;
} PaymentNode;

typedef struct {
    int booking_id;
    char user_email[MAX_EMAIL];
    int movie_id;
    char show_time[MAX_TIME];
    char show_date[15];
    int seats_selected[ROWS * COLS];
    int seat_count;
    double total_amount;
    int payment_id;
    char payment_method[MAX_PAY_METHOD_NAME];
    char booking_time[30];
} Booking;

typedef struct BookingNode {
    Booking data;
    struct BookingNode *next;
} BookingNode;

typedef struct {
    int id;
    char title[MAX_TITLE];
    char genre[MAX_GENRE];
    int duration_min;
    float rating;
    char show_times[5][MAX_TIME];
    int show_time_count;
    int is_active;
} Movie;

typedef struct MovieNode {
    Movie data;
    struct MovieNode *next;
} MovieNode;

typedef struct {
    char email[MAX_EMAIL];
    char name[MAX_NAME];
    char password[MAX_PASSWORD];
    int is_admin;
    char phone[15];
} User;

typedef struct UserNode {
    User data;
    struct UserNode *next;
} UserNode;

extern User current_user;
extern int is_logged_in;
extern MovieNode *movie_list;
extern UserNode *user_list;
extern BookingNode *booking_list;
extern PaymentNode *payment_list;
extern int next_booking_id;
extern int next_movie_id;
extern int next_payment_id;

void init_system();
void save_all_data();
void load_all_data();

/* encryption.c */
void sha256_hex(const char *input, char out_hex[65]);
void hash_password(const char *email, const char *password, char out[MAX_PASSWORD]);
int password_is_hashed(const char *pw);
int verify_password(const char *email, const char *plain, const char *stored);
int enc_file_is_encrypted(const char *path);
int enc_save_records(const char *path, const void *records, size_t rec_size, int count);
int enc_load_records(const char *path, size_t rec_size, void **out_records);

/* payment.c */
const char *payment_method_name(int method);
const char *payment_status_name(int status);
int process_payment(double amount, int booking_id, Payment *out);
double refund_payment(int booking_id);
void view_all_payments();

void user_register();
int user_login();
void user_logout();
void view_profile();
void update_profile();

void add_movie();
void edit_movie();
void delete_movie();
void list_movies(int show_all);
MovieNode* find_movie_by_id(int id);

void display_seats(int movie_id, char *show_time, char *show_date);
int book_ticket();
void view_my_bookings();
void cancel_booking();

void admin_dashboard();
void manage_movies_menu();
void manage_users_menu();
void view_all_bookings();
void generate_daily_report();

void main_menu();
void user_menu();
void clear_input_buffer();
int get_valid_int(int min, int max);
void get_today_date(char *buffer);
int compare_dates(char *d1, char *d2);

#endif
