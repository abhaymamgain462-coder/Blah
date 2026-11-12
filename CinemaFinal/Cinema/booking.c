#include "headers.h"

extern int next_booking_id;

static void get_booked_seats(int movie_id, char *show_time, char *show_date, int booked_seats[]) {
    for (int i = 0; i < ROWS * COLS; i++) booked_seats[i] = 0;
    BookingNode *p = booking_list;
    while (p) {
        if (p->data.movie_id == movie_id &&
            strcmp(p->data.show_time, show_time) == 0 &&
            strcmp(p->data.show_date, show_date) == 0) {
            for (int i = 0; i < p->data.seat_count; i++) {
                int s = p->data.seats_selected[i];
                if (s >= 0 && s < ROWS * COLS) booked_seats[s] = 1;
            }
        }
        p = p->next;
    }
}

void display_seats(int movie_id, char *show_time, char *show_date) {
    int booked[ROWS * COLS];
    get_booked_seats(movie_id, show_time, show_date, booked);

    printf("\n=============== SCREEN ===============\n");
    printf("         [      SCREEN HERE      ]\n");
    printf("---------------------------------------\n");
    printf("Seats:  Available [ ]  Booked  [X]\n\n");

    for (int r = 0; r < ROWS; r++) {
        printf("Row %c: ", 'A' + r);
        for (int c = 0; c < COLS; c++) {
            int idx = r * COLS + c;
            if (booked[idx]) printf("[X] ");
            else printf("[%d] ", c + 1);
            if (c == 4) printf("  ");
        }
        printf("\n");
    }
    printf("\nTotal seats: %d   Price per seat: Rs. %.2f\n", ROWS * COLS, SEAT_PRICE);
}

static int select_showtime(Movie *m, char *out_time) {
    printf("\nAvailable show times:\n");
    for (int i = 0; i < m->show_time_count; i++) {
        printf("  %d. %s\n", i + 1, m->show_times[i]);
    }
    printf("Select show time (1-%d): ", m->show_time_count);
    int ch = get_valid_int(1, m->show_time_count);
    strcpy(out_time, m->show_times[ch - 1]);
    return 1;
}

int book_ticket() {
    if (!is_logged_in) {
        printf("\nPlease login first to book tickets.\n");
        return 0;
    }

    printf("\n========== Book Movie Ticket ==========\n");
    list_movies(0);

    printf("\nEnter movie ID to book: ");
    int id = get_valid_int(1, 99999);
    MovieNode *mnode = find_movie_by_id(id);
    if (!mnode || !mnode->data.is_active) {
        printf("Invalid or inactive movie ID.\n");
        return 0;
    }
    Movie *m = &mnode->data;

    char show_time[MAX_TIME];
    select_showtime(m, show_time);

    char show_date[15], input[50];
    printf("Enter show date (DD/MM/YYYY) or press Enter for today: ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, " %[^\n]", show_date);
    if (strlen(show_date) == 0) {
        get_today_date(show_date);
        printf("Using today's date: %s\n", show_date);
    }

    printf("\nMovie: %s | Show: %s | Date: %s\n", m->title, show_time, show_date);
    display_seats(id, show_time, show_date);

    printf("How many seats to book? (1-8): ");
    int num_seats = get_valid_int(1, 8);

    int selected[ROWS * COLS];
    int sel_count = 0;
    int booked[ROWS * COLS];
    get_booked_seats(id, show_time, show_date, booked);

    for (int i = 0; i < num_seats; i++) {
        while (1) {
            printf("\nSeat %d of %d\n", i + 1, num_seats);
            printf("Enter Row (A-%c): ", 'A' + ROWS - 1);
            fgets(input, sizeof(input), stdin);
            char row_char;
            sscanf(input, " %c", &row_char);
            row_char = toupper(row_char);

            printf("Enter Column (1-%d): ", COLS);
            int col = get_valid_int(1, COLS);

            int r = row_char - 'A';
            int c = col - 1;
            int idx = r * COLS + c;

            if (r < 0 || r >= ROWS) {
                printf("Invalid row. Try again.\n");
                continue;
            }
            if (booked[idx]) {
                printf("Sorry, seat %c%d is already booked.\n", 'A' + r, c + 1);
                continue;
            }

            int already_sel = 0;
            for (int j = 0; j < sel_count; j++) {
                if (selected[j] == idx) {
                    already_sel = 1;
                    break;
                }
            }
            if (already_sel) {
                printf("You already selected that seat.\n");
                continue;
            }

            selected[sel_count++] = idx;
            printf("Selected seat %c%d successfully.\n", 'A' + r, c + 1);
            break;
        }
    }

    printf("\n================= BOOKING SUMMARY =================\n");
    printf("Booking ID: #%d\n", next_booking_id);
    printf("Movie:      %s\n", m->title);
    printf("Date:       %s\n", show_date);
    printf("Show Time:  %s\n", show_time);
    printf("Seats:      ");
    for (int i = 0; i < sel_count; i++) {
        int s = selected[i];
        printf("%c%d ", 'A' + (s / COLS), (s % COLS) + 1);
    }
    printf("\n");
    printf("Seat Count: %d\n", sel_count);
    double total = sel_count * SEAT_PRICE;
    double tax = total * 0.05;
    double final = total + tax;
    printf("Base Price: Rs. %.2f\n", total);
    printf("GST (5%%):   Rs. %.2f\n", tax);
    printf("TOTAL:      Rs. %.2f\n", final);
    printf("==================================================\n");

    printf("\nConfirm booking? (1=Yes, 0=No): ");
    if (get_valid_int(0, 1) != 1) {
        printf("Booking cancelled.\n");
        return 0;
    }

    /* ---- Payment step ---- */
    Payment pay;
    if (!process_payment(final, next_booking_id, &pay)) {
        printf("Booking aborted - payment was not completed.\n");
        return 0;
    }

    Booking b;
    b.booking_id = pay.booking_id;
    if (b.booking_id >= next_booking_id) next_booking_id = b.booking_id + 1;
    strcpy(b.user_email, current_user.email);
    b.movie_id = id;
    strcpy(b.show_time, show_time);
    strcpy(b.show_date, show_date);
    memcpy(b.seats_selected, selected, sizeof(int) * sel_count);
    b.seat_count = sel_count;
    b.total_amount = final;
    b.payment_id = pay.payment_id;
    strncpy(b.payment_method, payment_method_name(pay.method), MAX_PAY_METHOD_NAME - 1);
    b.payment_method[MAX_PAY_METHOD_NAME - 1] = '\0';

    time_t t = time(NULL);
    strcpy(b.booking_time, ctime(&t));
    b.booking_time[strcspn(b.booking_time, "\n")] = 0;

    BookingNode *node = (BookingNode*)malloc(sizeof(BookingNode));
    node->data = b;
    node->next = NULL;
    if (!booking_list) {
        booking_list = node;
    } else {
        BookingNode *p = booking_list;
        while (p->next) p = p->next;
        p->next = node;
    }

    save_all_data();
    printf("\n*** BOOKING CONFIRMED! Booking ID: #%d ***\n", b.booking_id);
    printf("Payment #%d via %s was successful.\n", b.payment_id, b.payment_method);
    return 1;
}

void view_my_bookings() {
    if (!is_logged_in) {
        printf("Please login first.\n");
        return;
    }
    printf("\n====================== MY BOOKINGS ======================\n");
    BookingNode *p = booking_list;
    int count = 0;
    while (p) {
        if (strcmp(p->data.user_email, current_user.email) == 0) {
            count++;
            MovieNode *m = find_movie_by_id(p->data.movie_id);
            char title[MAX_TITLE];
            if (m) strcpy(title, m->data.title);
            else strcpy(title, "(Removed)");

            printf("\n---------------------------------------------------------\n");
            printf("Booking ID:   #%d\n", p->data.booking_id);
            printf("Movie:        %s\n", title);
            printf("Date:         %s\n", p->data.show_date);
            printf("Show Time:    %s\n", p->data.show_time);
            printf("Seats:        ");
            for (int i = 0; i < p->data.seat_count; i++) {
                int s = p->data.seats_selected[i];
                printf("%c%d ", 'A' + (s / COLS), (s % COLS) + 1);
            }
            printf("\n");
            printf("Total Paid:   Rs. %.2f\n", p->data.total_amount);
            if (p->data.payment_id > 0) {
                printf("Payment:      #%d via %s\n", p->data.payment_id, p->data.payment_method);
            }
            printf("Booked on:    %s\n", p->data.booking_time);
        }
        p = p->next;
    }
    if (count == 0) {
        printf("\n            You have no bookings yet.\n");
    }
    printf("\n=========================================================\n");
    printf("Total bookings: %d\n", count);
}

void cancel_booking() {
    if (!is_logged_in) {
        printf("Please login first.\n");
        return;
    }
    view_my_bookings();
    BookingNode *p = booking_list;
    int has = 0;
    while (p) {
        if (strcmp(p->data.user_email, current_user.email) == 0) { has = 1; break; }
        p = p->next;
    }
    if (!has) return;

    printf("\nEnter Booking ID to cancel: ");
    int bid = get_valid_int(1, 999999);

    BookingNode *prev = NULL;
    p = booking_list;
    while (p) {
        if (p->data.booking_id == bid && strcmp(p->data.user_email, current_user.email) == 0) break;
        prev = p;
        p = p->next;
    }
    if (!p) {
        printf("Booking #%d not found or does not belong to you.\n", bid);
        return;
    }

    printf("\nThis will cancel booking #%d (Refund: Rs. %.2f).\n", bid, p->data.total_amount * 0.85);
    printf("Enter 1 to confirm cancellation: ");
    if (get_valid_int(0, 1) != 1) {
        printf("Cancellation aborted.\n");
        return;
    }

    if (prev) prev->next = p->next;
    else booking_list = p->next;
    free(p);

    /* Process the refund for the original payment. */
    double refunded = refund_payment(bid);
    if (refunded > 0.0) {
        printf("Refund of Rs. %.2f has been initiated to the original payment method.\n",
               refunded * 0.85);
    }

    save_all_data();
    printf("Booking #%d cancelled successfully. 15%% cancellation fee applied.\n", bid);
}
