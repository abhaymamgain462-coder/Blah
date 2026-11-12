#include "headers.h"

User current_user;
int is_logged_in = 0;
MovieNode *movie_list = NULL;
UserNode *user_list = NULL;
BookingNode *booking_list = NULL;
PaymentNode *payment_list = NULL;

int next_booking_id = 1;
int next_movie_id = 1;
int next_payment_id = 1;

void get_today_date(char *buffer) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(buffer, "%02d/%02d/%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
}

int compare_dates(char *d1, char *d2) {
    return strcmp(d1, d2) == 0;
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int get_valid_int(int min, int max) {
    int val;
    char input[100];
    while (1) {
        if (fgets(input, sizeof(input), stdin) == NULL) return min - 1;
        char *end;
        long lval = strtol(input, &end, 10);
        if (end != input && (*end == '\n' || *end == '\0')) {
            val = (int)lval;
            if (val >= min && val <= max) return val;
        }
        printf("Invalid input! Enter a number between %d and %d: ", min, max);
    }
}

/* ---- Encrypted save/load helpers ---- */

static void list_to_array_users(User **out, int *count) {
    *count = 0;
    UserNode *p = user_list;
    while (p) { (*count)++; p = p->next; }
    *out = NULL;
    if (*count == 0) return;
    *out = (User *)malloc(sizeof(User) * (size_t)*count);
    p = user_list;
    int i = 0;
    while (p) { (*out)[i++] = p->data; p = p->next; }
}

static void save_users() {
    User *arr = NULL;
    int n = 0;
    list_to_array_users(&arr, &n);
    if (!enc_save_records("data/users.dat", arr, sizeof(User), n)) {
        printf("Warning: Could not save users data.\n");
    }
    free(arr);
}

static void load_users() {
    void *recs = NULL;
    int n = enc_load_records("data/users.dat", sizeof(User), &recs);
    if (n <= 0) { free(recs); return; }
    User *users = (User *)recs;
    for (int i = 0; i < n; i++) {
        UserNode *node = (UserNode*)malloc(sizeof(UserNode));
        node->data = users[i];
        node->next = NULL;
        if (!user_list) {
            user_list = node;
        } else {
            UserNode *p = user_list;
            while (p->next) p = p->next;
            p->next = node;
        }
    }
    free(recs);
}

static void save_movies() {
    int n = 0;
    MovieNode *p = movie_list;
    while (p) { n++; p = p->next; }
    Movie *arr = NULL;
    if (n > 0) {
        arr = (Movie *)malloc(sizeof(Movie) * (size_t)n);
        p = movie_list;
        int i = 0;
        while (p) { arr[i++] = p->data; p = p->next; }
    }
    if (!enc_save_records("data/movies.dat", arr, sizeof(Movie), n)) {
        printf("Warning: Could not save movies data.\n");
    }
    free(arr);
}

static void load_movies() {
    void *recs = NULL;
    int n = enc_load_records("data/movies.dat", sizeof(Movie), &recs);
    if (n <= 0) { free(recs); return; }
    Movie *movies = (Movie *)recs;
    for (int i = 0; i < n; i++) {
        MovieNode *node = (MovieNode*)malloc(sizeof(MovieNode));
        node->data = movies[i];
        node->next = NULL;
        if (!movie_list) {
            movie_list = node;
        } else {
            MovieNode *p = movie_list;
            while (p->next) p = p->next;
            p->next = node;
        }
        if (movies[i].id >= next_movie_id) next_movie_id = movies[i].id + 1;
    }
    free(recs);
}

static void save_bookings() {
    int n = 0;
    BookingNode *p = booking_list;
    while (p) { n++; p = p->next; }
    Booking *arr = NULL;
    if (n > 0) {
        arr = (Booking *)malloc(sizeof(Booking) * (size_t)n);
        p = booking_list;
        int i = 0;
        while (p) { arr[i++] = p->data; p = p->next; }
    }
    if (!enc_save_records("data/bookings.dat", arr, sizeof(Booking), n)) {
        printf("Warning: Could not save bookings data.\n");
    }
    free(arr);
}

static void append_booking(const Booking *b) {
    BookingNode *node = (BookingNode*)malloc(sizeof(BookingNode));
    node->data = *b;
    node->next = NULL;
    if (!booking_list) {
        booking_list = node;
    } else {
        BookingNode *p = booking_list;
        while (p->next) p = p->next;
        p->next = node;
    }
    if (b->booking_id >= next_booking_id) next_booking_id = b->booking_id + 1;
}

static void load_bookings() {
    void *recs = NULL;
    int n = enc_load_records("data/bookings.dat", sizeof(Booking), &recs);
    if (n > 0) {
        Booking *bookings = (Booking *)recs;
        for (int i = 0; i < n; i++) append_booking(&bookings[i]);
        free(recs);
        return;
    }
    free(recs);

    /* Legacy plaintext file written by an older version of the program:
       it uses the old Booking layout without payment fields. */
    if (enc_file_is_encrypted("data/bookings.dat")) return; /* empty encrypted file, nothing legacy to do */
    FILE *fp = fopen("data/bookings.dat", "rb");
    if (!fp) return;

    /* Old struct layout, exactly as it was written by previous versions. */
    typedef struct {
        int booking_id;
        char user_email[MAX_EMAIL];
        int movie_id;
        char show_time[MAX_TIME];
        char show_date[15];
        int seats_selected[ROWS * COLS];
        int seat_count;
        double total_amount;
        char booking_time[30];
    } LegacyBooking;

    LegacyBooking lb;
    while (fread(&lb, sizeof(LegacyBooking), 1, fp) == 1) {
        Booking b;
        memset(&b, 0, sizeof(b));
        b.booking_id = lb.booking_id;
        memcpy(b.user_email, lb.user_email, MAX_EMAIL);
        b.movie_id = lb.movie_id;
        memcpy(b.show_time, lb.show_time, MAX_TIME);
        memcpy(b.show_date, lb.show_date, 15);
        memcpy(b.seats_selected, lb.seats_selected, sizeof(int) * (ROWS * COLS));
        b.seat_count = lb.seat_count;
        b.total_amount = lb.total_amount;
        memcpy(b.booking_time, lb.booking_time, sizeof(b.booking_time));
        b.payment_id = 0;
        strcpy(b.payment_method, "N/A");
        append_booking(&b);
    }
    fclose(fp);
}

static void save_payments() {
    int n = 0;
    PaymentNode *p = payment_list;
    while (p) { n++; p = p->next; }
    Payment *arr = NULL;
    if (n > 0) {
        arr = (Payment *)malloc(sizeof(Payment) * (size_t)n);
        p = payment_list;
        int i = 0;
        while (p) { arr[i++] = p->data; p = p->next; }
    }
    if (!enc_save_records("data/payments.dat", arr, sizeof(Payment), n)) {
        printf("Warning: Could not save payments data.\n");
    }
    free(arr);
}

static void load_payments() {
    void *recs = NULL;
    int n = enc_load_records("data/payments.dat", sizeof(Payment), &recs);
    if (n <= 0) { free(recs); return; }
    Payment *pays = (Payment *)recs;
    for (int i = 0; i < n; i++) {
        PaymentNode *node = (PaymentNode*)malloc(sizeof(PaymentNode));
        node->data = pays[i];
        node->next = NULL;
        if (!payment_list) {
            payment_list = node;
        } else {
            PaymentNode *p = payment_list;
            while (p->next) p = p->next;
            p->next = node;
        }
        if (pays[i].payment_id >= next_payment_id) next_payment_id = pays[i].payment_id + 1;
    }
    free(recs);
}

void load_all_data() {
    load_users();
    load_movies();
    load_bookings();
    load_payments();
}

void save_all_data() {
    save_users();
    save_movies();
    save_bookings();
    save_payments();
}

static void seed_admin_user() {
    if (user_list) return;
    UserNode *node = (UserNode*)malloc(sizeof(UserNode));
    memset(node, 0, sizeof(UserNode));
    strcpy(node->data.email, "admin@cinema.com");
    strcpy(node->data.name, "Administrator");
    hash_password(node->data.email, "admin123", node->data.password);
    strcpy(node->data.phone, "9876543210");
    node->data.is_admin = 1;
    node->next = NULL;
    user_list = node;
}

static void seed_sample_movies() {
    if (movie_list) return;

    Movie samples[] = {
        {1, "Inception", "Sci-Fi", 148, 8.8, {"10:00", "14:30", "19:00"}, 3, 1},
        {2, "The Dark Knight", "Action", 152, 9.0, {"11:00", "15:00", "20:00"}, 3, 1},
        {3, "Interstellar", "Sci-Fi", 169, 8.6, {"10:30", "16:00", "21:00"}, 3, 1},
        {4, "The Godfather", "Crime", 175, 9.2, {"13:00", "18:00"}, 2, 1},
        {5, "Pulp Fiction", "Thriller", 154, 8.9, {"12:00", "17:30", "22:00"}, 3, 1}
    };

    for (int i = 0; i < 5; i++) {
        MovieNode *node = (MovieNode*)malloc(sizeof(MovieNode));
        node->data = samples[i];
        node->next = NULL;
        if (!movie_list) {
            movie_list = node;
        } else {
            MovieNode *p = movie_list;
            while (p->next) p = p->next;
            p->next = node;
        }
    }
    next_movie_id = 6;
}

void init_system() {
    system("mkdir data 2>nul");
    load_all_data();
    seed_admin_user();
    seed_sample_movies();
    save_all_data();
}
