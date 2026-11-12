#include "headers.h"

void view_all_bookings() {
    printf("\n====================== ALL BOOKINGS ======================\n");
    BookingNode *p = booking_list;
    int count = 0;
    double total_rev = 0;
    while (p) {
        count++;
        total_rev += p->data.total_amount;
        MovieNode *m = find_movie_by_id(p->data.movie_id);
        char title[MAX_TITLE];
        if (m) strcpy(title, m->data.title);
        else strcpy(title, "(Removed)");

        printf("\n---------------------------------------------------------\n");
        printf("Booking #%d\n", p->data.booking_id);
        printf("  User:    %s (%s)\n", p->data.user_email,
            (strcmp(p->data.user_email, current_user.email) == 0) ? "You" : "");
        printf("  Movie:   %s\n", title);
        printf("  Date:    %s\n", p->data.show_date);
        printf("  Show:    %s\n", p->data.show_time);
        printf("  Seats:   %d seat(s)  |  ", p->data.seat_count);
        for (int i = 0; i < p->data.seat_count; i++) {
            int s = p->data.seats_selected[i];
            printf("%c%d ", 'A' + (s / COLS), (s % COLS) + 1);
        }
        printf("\n");
        printf("  Amount:  Rs. %.2f\n", p->data.total_amount);
        if (p->data.payment_id > 0) {
            printf("  Payment: #%d via %s\n", p->data.payment_id, p->data.payment_method);
        } else {
            printf("  Payment: N/A (legacy booking)\n");
        }
        printf("  Booked:  %s\n", p->data.booking_time);
        p = p->next;
    }
    if (count == 0) {
        printf("\n            No bookings in the system.\n");
    }
    printf("\n=========================================================\n");
    printf("Total: %d booking(s)   |   Total Revenue: Rs. %.2f\n", count, total_rev);
}

void manage_users_menu() {
    while (1) {
        printf("\n========== Manage Users ==========\n");
        printf("1. View All Users\n");
        printf("2. Promote User to Admin\n");
        printf("3. Remove User\n");
        printf("4. Back to Admin Dashboard\n");
        printf("\nEnter choice: ");
        int ch = get_valid_int(1, 4);

        if (ch == 1) {
            printf("\n====================== USERS ======================\n");
            UserNode *p = user_list;
            int count = 0;
            printf("SL   Name                 Email                Role       Phone\n");
            printf("------------------------------------------------------------\n");
            while (p) {
                count++;
                printf("%-4d %-20s %-20s %-10s %s\n",
                    count, p->data.name, p->data.email,
                    p->data.is_admin ? "Admin" : "Customer", p->data.phone);
                p = p->next;
            }
            printf("\nTotal: %d user(s)\n", count);
        }
        else if (ch == 2) {
            printf("\nEnter user email to promote: ");
            char email[MAX_EMAIL], input[200];
            fgets(input, sizeof(input), stdin);
            sscanf(input, " %[^\n]", email);
            UserNode *p = user_list;
            while (p) {
                if (strcmp(p->data.email, email) == 0) {
                    if (p->data.is_admin) {
                        printf("%s is already an admin.\n", email);
                    } else {
                        p->data.is_admin = 1;
                        save_all_data();
                        printf("%s has been promoted to Admin.\n", email);
                    }
                    break;
                }
                p = p->next;
            }
            if (!p) printf("User not found.\n");
        }
        else if (ch == 3) {
            printf("\nEnter user email to remove: ");
            char email[MAX_EMAIL], input[200];
            fgets(input, sizeof(input), stdin);
            sscanf(input, " %[^\n]", email);
            if (strcmp(email, current_user.email) == 0) {
                printf("You cannot remove your own account.\n");
                continue;
            }
            UserNode *prev = NULL, *curr = user_list;
            while (curr) {
                if (strcmp(curr->data.email, email) == 0) break;
                prev = curr;
                curr = curr->next;
            }
            if (!curr) { printf("User not found.\n"); continue; }
            printf("Enter 1 to confirm removal of '%s': ", curr->data.name);
            if (get_valid_int(0, 1) != 1) continue;

            BookingNode *bp = booking_list, *bprev = NULL, *btemp;
            while (bp) {
                if (strcmp(bp->data.user_email, email) == 0) {
                    btemp = bp;
                    if (bprev) bprev->next = bp->next;
                    else booking_list = bp->next;
                    bp = bp->next;
                    free(btemp);
                } else {
                    bprev = bp;
                    bp = bp->next;
                }
            }

            PaymentNode *pp = payment_list, *pprev = NULL, *ptemp;
            while (pp) {
                if (strcmp(pp->data.user_email, email) == 0) {
                    ptemp = pp;
                    if (pprev) pprev->next = pp->next;
                    else payment_list = pp->next;
                    pp = pp->next;
                    free(ptemp);
                } else {
                    pprev = pp;
                    pp = pp->next;
                }
            }

            if (prev) prev->next = curr->next;
            else user_list = curr->next;
            free(curr);
            save_all_data();
            printf("User removed successfully.\n");
        }
        else if (ch == 4) break;
    }
}

void manage_movies_menu() {
    while (1) {
        printf("\n========== Manage Movies ==========\n");
        printf("1. List All Movies (with inactive)\n");
        printf("2. Add New Movie\n");
        printf("3. Edit Movie\n");
        printf("4. Delete Movie\n");
        printf("5. Back to Admin Dashboard\n");
        printf("\nEnter choice: ");
        int ch = get_valid_int(1, 5);
        if (ch == 1) list_movies(1);
        else if (ch == 2) add_movie();
        else if (ch == 3) edit_movie();
        else if (ch == 4) delete_movie();
        else if (ch == 5) break;
    }
}

void generate_daily_report() {
    char today[15];
    get_today_date(today);
    char input[50];

    printf("\n========== DAILY REPORT ==========\n");
    printf("Enter date (DD/MM/YYYY) or press Enter for today (%s): ", today);
    fgets(input, sizeof(input), stdin);
    char report_date[15];
    sscanf(input, " %[^\n]", report_date);
    if (strlen(report_date) == 0) {
        strcpy(report_date, today);
    }
    printf("\nGenerating report for: %s\n", report_date);

    BookingNode *p = booking_list;
    int total_bookings = 0;
    int total_tickets = 0;
    double total_revenue = 0;
    int movie_booking_count[1000] = {0};
    int movie_ticket_count[1000] = {0};
    double movie_revenue[1000] = {0};
    int showtime_occupancy[1000][5] = {0};
    int unique_movies[1000];
    int unique_count = 0;

    while (p) {
        if (compare_dates(p->data.show_date, report_date)) {
            total_bookings++;
            total_tickets += p->data.seat_count;
            total_revenue += p->data.total_amount;

            int mid = p->data.movie_id;
            movie_booking_count[mid]++;
            movie_ticket_count[mid] += p->data.seat_count;
            movie_revenue[mid] += p->data.total_amount;

            int found = 0;
            for (int i = 0; i < unique_count; i++) {
                if (unique_movies[i] == mid) { found = 1; break; }
            }
            if (!found && unique_count < 1000) unique_movies[unique_count++] = mid;

            MovieNode *m = find_movie_by_id(mid);
            if (m) {
                for (int s = 0; s < m->data.show_time_count; s++) {
                    if (strcmp(m->data.show_times[s], p->data.show_time) == 0) {
                        showtime_occupancy[mid][s] += p->data.seat_count;
                        break;
                    }
                }
            }
        }
        p = p->next;
    }

    printf("\n==============================================================\n");
    printf("                     CINEMA DAILY REPORT                      \n");
    printf("==============================================================\n");
    printf("Report Date: %s\n", report_date);
    printf("--------------------------------------------------------------\n");
    printf("\n");
    printf("  >>> OVERALL SUMMARY <<<\n");
    printf("  Total Bookings:        %d\n", total_bookings);
    printf("  Total Tickets Sold:    %d\n", total_tickets);
    printf("  Total Revenue:         Rs. %.2f\n", total_revenue);
    printf("  Avg. Tickets/Booking:  %.2f\n", total_bookings ? (double)total_tickets / total_bookings : 0.0);
    printf("  Avg. Value/Booking:    Rs. %.2f\n", total_bookings ? total_revenue / total_bookings : 0.0);
    printf("\n");

    if (unique_count == 0) {
        printf("  No show bookings for this date.\n");
    } else {
        printf("  >>> PER-MOVIE BREAKDOWN <<<\n");
        printf("  -----------------------------------------------------------\n");
        printf("  %-22s Bookings Tickets  Revenue     Occ.%%\n", "Movie");
        printf("  -----------------------------------------------------------\n");
        for (int i = 0; i < unique_count; i++) {
            int mid = unique_movies[i];
            MovieNode *m = find_movie_by_id(mid);
            char name[MAX_TITLE];
            if (m) strcpy(name, m->data.title);
            else strcpy(name, "N/A");

            int total_show_cap = m ? (m->data.show_time_count * ROWS * COLS) : (ROWS * COLS);
            int total_tickets_sold = movie_ticket_count[mid];
            double occ_pct = total_show_cap ? (100.0 * total_tickets_sold / total_show_cap) : 0;

            printf("  %-22s %-8d %-7d Rs. %-8.2f %.1f%%\n",
                name,
                movie_booking_count[mid],
                total_tickets_sold,
                movie_revenue[mid],
                occ_pct > 100 ? 100.0 : occ_pct);

            if (m) {
                for (int s = 0; s < m->data.show_time_count; s++) {
                    int st_seats = showtime_occupancy[mid][s];
                    int cap = ROWS * COLS;
                    double pct = 100.0 * st_seats / cap;
                    printf("     -> %s: %d/%d seats (%.1f%%) | Rs. %.2f\n",
                        m->data.show_times[s], st_seats, cap,
                        pct > 100 ? 100.0 : pct,
                        st_seats * SEAT_PRICE * 1.05);
                }
            }
        }
        printf("  -----------------------------------------------------------\n");
    }

    printf("\n==============================================================\n");
    printf("                  END OF DAILY REPORT                         \n");
    printf("==============================================================\n");

    printf("\nSave report to file? (1=Yes, 0=No): ");
    if (get_valid_int(0, 1) == 1) {
        char fname[50];
        sprintf(fname, "data/report_%s.txt", report_date);
        char fname_clean[50];
        int fi = 0;
        for (int i = 0; fname[i]; i++) {
            if (fname[i] != '/') fname_clean[fi++] = fname[i];
        }
        fname_clean[fi] = 0;

        FILE *fp = fopen(fname_clean, "w");
        if (fp) {
            fprintf(fp, "CINEMA DAILY REPORT - %s\n", report_date);
            fprintf(fp, "Total Bookings: %d\n", total_bookings);
            fprintf(fp, "Total Tickets:  %d\n", total_tickets);
            fprintf(fp, "Total Revenue:  Rs. %.2f\n", total_revenue);
            for (int i = 0; i < unique_count; i++) {
                int mid = unique_movies[i];
                MovieNode *m = find_movie_by_id(mid);
                char name[MAX_TITLE];
                if (m) strcpy(name, m->data.title); else strcpy(name, "N/A");
                fprintf(fp, "Movie: %s | Bookings: %d | Tickets: %d | Revenue: Rs. %.2f\n",
                    name, movie_booking_count[mid], movie_ticket_count[mid], movie_revenue[mid]);
            }
            fclose(fp);
            printf("Report saved to: %s\n", fname_clean);
        } else {
            printf("Could not save report file.\n");
        }
    }
}

void admin_dashboard() {
    while (1) {
        printf("\n============================================\n");
        printf("            ADMIN DASHBOARD                 \n");
        printf("  Logged in as: %s (Admin)           \n", current_user.name);
        printf("============================================\n");
        printf("1. Manage Movies\n");
        printf("2. Manage Users\n");
        printf("3. View All Bookings\n");
        printf("4. View Payment Ledger\n");
        printf("5. Generate Daily Report\n");
        printf("6. Back to Main Menu\n");
        printf("============================================\n");
        printf("Enter choice: ");
        int ch = get_valid_int(1, 6);

        switch (ch) {
            case 1: manage_movies_menu(); break;
            case 2: manage_users_menu(); break;
            case 3: view_all_bookings(); break;
            case 4: view_all_payments(); break;
            case 5: generate_daily_report(); break;
            case 6: return;
        }
    }
}
