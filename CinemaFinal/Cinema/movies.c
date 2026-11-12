#include "headers.h"

extern int next_movie_id;

MovieNode* find_movie_by_id(int id) {
    MovieNode *p = movie_list;
    while (p) {
        if (p->data.id == id) return p;
        p = p->next;
    }
    return NULL;
}

void list_movies(int show_all) {
    MovieNode *p = movie_list;
    int count = 0;

    printf("\n=============================================================\n");
    printf("                        MOVIE LISTING                        \n");
    printf("=============================================================\n");
    printf("ID   Title                Genre       Duration  Rating  Status\n");
    printf("-------------------------------------------------------------\n");

    while (p) {
        if (show_all || p->data.is_active) {
            printf("%-4d %-20s %-11s %-9d %-6.1f  %s\n",
                p->data.id,
                p->data.title,
                p->data.genre,
                p->data.duration_min,
                p->data.rating,
                p->data.is_active ? "Active" : "Inactive");
            count++;
        }
        p = p->next;
    }
    if (count == 0) {
        printf("                     No movies found.\n");
    }
    printf("=============================================================\n");
    printf("Total: %d movie(s)\n", count);
}

void add_movie() {
    printf("\n========== Add New Movie ==========\n");
    Movie m;
    char input[200];

    m.id = next_movie_id++;
    printf("Movie ID assigned: %d\n", m.id);

    printf("Enter movie title: ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, " %[^\n]", m.title);
    if (strlen(m.title) == 0) {
        printf("Title cannot be empty.\n");
        next_movie_id--;
        return;
    }

    printf("Enter genre: ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, " %[^\n]", m.genre);

    printf("Enter duration (minutes): ");
    m.duration_min = get_valid_int(30, 300);

    printf("Enter rating (0.0 - 10.0): ");
    while (1) {
        fgets(input, sizeof(input), stdin);
        float r;
        if (sscanf(input, "%f", &r) == 1 && r >= 0.0 && r <= 10.0) {
            m.rating = r;
            break;
        }
        printf("Invalid rating. Enter 0.0 - 10.0: ");
    }

    printf("Enter number of show times (1-5): ");
    m.show_time_count = get_valid_int(1, 5);

    for (int i = 0; i < m.show_time_count; i++) {
        printf("Show time %d (HH:MM format, e.g. 14:30): ", i + 1);
        fgets(input, sizeof(input), stdin);
        sscanf(input, " %[^\n]", m.show_times[i]);
    }

    m.is_active = 1;

    MovieNode *node = (MovieNode*)malloc(sizeof(MovieNode));
    node->data = m;
    node->next = NULL;

    if (!movie_list) {
        movie_list = node;
    } else {
        MovieNode *p = movie_list;
        while (p->next) p = p->next;
        p->next = node;
    }

    save_all_data();
    printf("\nMovie added successfully!\n");
}

void edit_movie() {
    printf("\n========== Edit Movie ==========\n");
    list_movies(1);

    printf("\nEnter movie ID to edit: ");
    int id = get_valid_int(1, 99999);

    MovieNode *node = find_movie_by_id(id);
    if (!node) {
        printf("Movie with ID %d not found.\n", id);
        return;
    }

    Movie *m = &node->data;
    char input[200];

    printf("\nEditing: %s (ID: %d)\n", m->title, m->id);

    printf("Current title: %s\n", m->title);
    printf("Enter new title (leave blank to keep): ");
    fgets(input, sizeof(input), stdin);
    char new_val[MAX_TITLE];
    sscanf(input, " %[^\n]", new_val);
    if (strlen(new_val) > 0) strcpy(m->title, new_val);

    printf("Current genre: %s\n", m->genre);
    printf("Enter new genre (leave blank to keep): ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, " %[^\n]", new_val);
    if (strlen(new_val) > 0) strcpy(m->genre, new_val);

    printf("Current duration: %d min\n", m->duration_min);
    printf("Enter new duration (0 to keep): ");
    int dur = get_valid_int(0, 300);
    if (dur > 0) m->duration_min = dur;

    printf("Current rating: %.1f\n", m->rating);
    printf("Enter new rating (-1 to keep): ");
    fgets(input, sizeof(input), stdin);
    float r;
    if (sscanf(input, "%f", &r) == 1 && r >= 0.0 && r <= 10.0) {
        m->rating = r;
    }

    printf("Current show times: ");
    for (int i = 0; i < m->show_time_count; i++) {
        printf("%s ", m->show_times[i]);
    }
    printf("\nEdit show times? (1=Yes, 0=No): ");
    int edit_times = get_valid_int(0, 1);
    if (edit_times) {
        printf("Enter new number of show times (1-5): ");
        m->show_time_count = get_valid_int(1, 5);
        for (int i = 0; i < m->show_time_count; i++) {
            printf("Show time %d (HH:MM): ", i + 1);
            fgets(input, sizeof(input), stdin);
            sscanf(input, " %[^\n]", m->show_times[i]);
        }
    }

    printf("Current status: %s\n", m->is_active ? "Active" : "Inactive");
    printf("Toggle status? (1=Yes, 0=No): ");
    int toggle = get_valid_int(0, 1);
    if (toggle) m->is_active = !m->is_active;

    save_all_data();
    printf("\nMovie updated successfully!\n");
}

void delete_movie() {
    printf("\n========== Delete Movie ==========\n");
    list_movies(1);

    printf("\nEnter movie ID to delete: ");
    int id = get_valid_int(1, 99999);

    MovieNode *prev = NULL, *curr = movie_list;
    while (curr) {
        if (curr->data.id == id) break;
        prev = curr;
        curr = curr->next;
    }

    if (!curr) {
        printf("Movie with ID %d not found.\n", id);
        return;
    }

    printf("Are you sure you want to delete '%s'? This will also remove related bookings.\n", curr->data.title);
    printf("Enter 1 to confirm: ");
    if (get_valid_int(0, 1) != 1) {
        printf("Deletion cancelled.\n");
        return;
    }

    BookingNode *bp = booking_list, *bprev = NULL, *btemp;
    while (bp) {
        if (bp->data.movie_id == id) {
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

    if (prev) prev->next = curr->next;
    else movie_list = curr->next;
    free(curr);

    save_all_data();
    printf("Movie deleted successfully.\n");
}
