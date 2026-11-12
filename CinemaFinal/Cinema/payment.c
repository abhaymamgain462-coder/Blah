#include "headers.h"

/* =====================================================================
   payment.c
   - Multiple payment methods: Credit/Debit Card (Luhn validated),
     UPI, Mobile Wallet and Cash at counter
   - Payment ledger (payments.dat, encrypted on disk)
   - Refund processing for cancelled bookings
   ===================================================================== */

static const char *METHOD_NAMES[] = { "N/A", "Card", "UPI", "Wallet", "Cash" };

const char *payment_method_name(int method) {
    if (method < PAY_METHOD_CARD || method > PAY_METHOD_CASH) return "N/A";
    return METHOD_NAMES[method];
}

const char *payment_status_name(int status) {
    return (status == PAYMENT_STATUS_REFUNDED) ? "Refunded" : "Completed";
}

/* Luhn algorithm: validates credit/debit card numbers. */
static int luhn_valid(const char *digits) {
    int sum = 0, alt = 0;
    int len = (int)strlen(digits);
    for (int i = len - 1; i >= 0; i--) {
        int d = digits[i] - '0';
        if (alt) {
            d *= 2;
            if (d > 9) d -= 9;
        }
        sum += d;
        alt = !alt;
    }
    return sum % 10 == 0;
}

static int read_card_digits(char *out_digits, int outsz) {
    char input[100];
    if (fgets(input, sizeof(input), stdin) == NULL) return 0;
    int j = 0;
    for (int i = 0; input[i] && j < outsz - 1; i++) {
        if (input[i] == ' ' || input[i] == '-' || input[i] == '\n' || input[i] == '\r') continue;
        if (!isdigit((unsigned char)input[i])) return -1; /* invalid character */
        out_digits[j++] = input[i];
    }
    out_digits[j] = '\0';
    return j;
}

static int pay_by_card(Payment *pay) {
    char input[100], digits[32];

    printf("Enter 16-digit card number (or 0 to go back): ");
    int n = read_card_digits(digits, sizeof(digits));
    if (n <= 0) return 0;
    if (n < 13 || n > 19 || !luhn_valid(digits)) {
        printf("Invalid card number (failed checksum). Payment aborted.\n");
        return 0;
    }

    int mm, yy;
    printf("Enter expiry as MM/YY: ");
    if (fgets(input, sizeof(input), stdin) == NULL) return 0;
    if (sscanf(input, " %d/%d", &mm, &yy) != 2 || mm < 1 || mm > 12 || yy < 0) {
        printf("Invalid expiry date. Payment aborted.\n");
        return 0;
    }
    time_t t = time(NULL);
    struct tm *now = localtime(&t);
    int cur_yy = now->tm_year + 1900 - 2000;
    int cur_mm = now->tm_mon + 1;
    if (yy < cur_yy || (yy == cur_yy && mm < cur_mm)) {
        printf("Card is expired. Payment aborted.\n");
        return 0;
    }

    printf("Enter CVV: ");
    if (fgets(input, sizeof(input), stdin) == NULL) return 0;
    char cvv[8];
    if (sscanf(input, " %7s", cvv) != 1) return 0;
    int cvv_len = (int)strlen(cvv);
    if (cvv_len < 3 || cvv_len > 4) {
        printf("Invalid CVV. Payment aborted.\n");
        return 0;
    }
    for (int i = 0; i < cvv_len; i++) {
        if (!isdigit((unsigned char)cvv[i])) {
            printf("Invalid CVV. Payment aborted.\n");
            return 0;
        }
    }

    pay->method = PAY_METHOD_CARD;
    /* Never store the full card number - only the masked version. */
    snprintf(pay->details, sizeof(pay->details), "Card **** **** **** %s (exp %02d/%02d)",
             digits + strlen(digits) - 4, mm, yy);
    return 1;
}

static int pay_by_upi(Payment *pay) {
    char input[100], upi[64];
    printf("Enter UPI ID (e.g. name@upi) or 0 to go back: ");
    if (fgets(input, sizeof(input), stdin) == NULL) return 0;
    sscanf(input, " %63s", upi);
    if (strcmp(upi, "0") == 0 || strlen(upi) == 0) return 0;

    char *at = strchr(upi, '@');
    if (!at || at == upi || !at[1] || strlen(upi) < 6 || at[1] == '.') {
        printf("Invalid UPI ID format. Payment aborted.\n");
        return 0;
    }
    for (char *p = upi; *p; p++) {
        if (!(isalnum((unsigned char)*p) || *p == '@' || *p == '.' || *p == '_' || *p == '-')) {
            printf("Invalid characters in UPI ID. Payment aborted.\n");
            return 0;
        }
    }

    pay->method = PAY_METHOD_UPI;
    snprintf(pay->details, sizeof(pay->details), "UPI: %s", upi);
    return 1;
}

static int pay_by_wallet(Payment *pay) {
    char input[100], phone[16];
    printf("Enter 10-digit wallet-linked mobile number (or 0 to go back): ");
    if (fgets(input, sizeof(input), stdin) == NULL) return 0;
    sscanf(input, " %15s", phone);
    if (strcmp(phone, "0") == 0 || strlen(phone) == 0) return 0;

    if (strlen(phone) != 10) {
        printf("Mobile number must be 10 digits. Payment aborted.\n");
        return 0;
    }
    for (int i = 0; i < 10; i++) {
        if (!isdigit((unsigned char)phone[i])) {
            printf("Mobile number must contain only digits. Payment aborted.\n");
            return 0;
        }
    }

    pay->method = PAY_METHOD_WALLET;
    snprintf(pay->details, sizeof(pay->details), "Wallet ****%s", phone + 6);
    return 1;
}

static int pay_by_cash(double amount, Payment *pay) {
    printf("\nPlease pay Rs. %.2f at the billing counter to confirm this booking.\n", amount);
    printf("The booking stays PENDING until cash is received.\n");
    printf("Enter 1 to confirm cash payment, 0 to go back: ");
    if (get_valid_int(0, 1) != 1) return 0;

    pay->method = PAY_METHOD_CASH;
    snprintf(pay->details, sizeof(pay->details), "Cash at counter");
    return 1;
}

static void print_payment_receipt(const Payment *pay) {
    printf("\n================ PAYMENT RECEIPT ================\n");
    printf("Payment ID:  #%d\n", pay->payment_id);
    printf("Booking ID:  #%d\n", pay->booking_id);
    printf("Paid By:     %s\n", pay->user_email);
    printf("Method:      %s\n", payment_method_name(pay->method));
    printf("Details:     %s\n", pay->details);
    printf("Amount:      Rs. %.2f\n", pay->amount);
    printf("Status:      %s\n", payment_status_name(pay->status));
    printf("Date:        %s\n", pay->paid_at);
    printf("=================================================\n");
}

/* Runs the full payment flow. Returns 1 on success (payment completed
   and added to the ledger), 0 if the user cancelled or payment failed. */
int process_payment(double amount, int booking_id, Payment *out) {
    memset(out, 0, sizeof(Payment));

    printf("\n=================== PAYMENT =====================\n");
    printf("Booking ID: #%d\n", booking_id);
    printf("Amount Due: Rs. %.2f\n", amount);
    printf("=================================================\n");
    printf("Select payment method:\n");
    printf("  1. Credit / Debit Card\n");
    printf("  2. UPI\n");
    printf("  3. Mobile Wallet\n");
    printf("  4. Pay Cash at Counter\n");
    printf("  5. Cancel Payment\n");
    printf("Enter choice (1-5): ");

    int choice = get_valid_int(1, 5);
    if (choice == 5) return 0;

    int ok = 0;
    switch (choice) {
        case PAY_METHOD_CARD:  ok = pay_by_card(out);  break;
        case PAY_METHOD_UPI:   ok = pay_by_upi(out);   break;
        case PAY_METHOD_WALLET: ok = pay_by_wallet(out); break;
        case PAY_METHOD_CASH:  ok = pay_by_cash(amount, out); break;
    }
    if (!ok) {
        printf("Payment not completed. No money was charged.\n");
        return 0;
    }

    out->payment_id = next_payment_id++;
    out->booking_id = booking_id;
    strncpy(out->user_email, current_user.email, MAX_EMAIL - 1);
    out->amount = amount;
    out->status = PAYMENT_STATUS_COMPLETED;

    time_t t = time(NULL);
    strncpy(out->paid_at, ctime(&t), sizeof(out->paid_at) - 1);
    out->paid_at[strcspn(out->paid_at, "\n")] = 0;

    /* Add to the in-memory ledger (persisted by the caller's save_all_data). */
    PaymentNode *node = (PaymentNode *)malloc(sizeof(PaymentNode));
    node->data = *out;
    node->next = NULL;
    if (!payment_list) {
        payment_list = node;
    } else {
        PaymentNode *p = payment_list;
        while (p->next) p = p->next;
        p->next = node;
    }

    print_payment_receipt(out);
    return 1;
}

/* Marks the payment of a booking as refunded. Returns the refunded amount (0 if none). */
double refund_payment(int booking_id) {
    PaymentNode *p = payment_list;
    while (p) {
        if (p->data.booking_id == booking_id && p->data.status == PAYMENT_STATUS_COMPLETED) {
            p->data.status = PAYMENT_STATUS_REFUNDED;
            return p->data.amount;
        }
        p = p->next;
    }
    return 0.0;
}

void view_all_payments() {
    printf("\n==================== PAYMENT LEDGER ====================\n");
    PaymentNode *p = payment_list;
    int count = 0, refunded = 0;
    double total_collected = 0, total_refunded = 0;

    printf("%-6s %-8s %-22s %-8s %-26s %-11s %-9s\n",
           "PayID", "BookID", "User", "Method", "Details", "Amount", "Status");
    printf("-------------------------------------------------------------------------\n");
    while (p) {
        count++;
        if (p->data.status == PAYMENT_STATUS_REFUNDED) {
            refunded++;
            total_refunded += p->data.amount;
        } else {
            total_collected += p->data.amount;
        }
        printf("#%-5d #%-7d %-22s %-8s %-26s Rs.%-10.2f %-9s\n",
               p->data.payment_id, p->data.booking_id, p->data.user_email,
               payment_method_name(p->data.method), p->data.details,
               p->data.amount, payment_status_name(p->data.status));
        p = p->next;
    }
    if (count == 0) {
        printf("                 No payments recorded yet.\n");
    }
    printf("-------------------------------------------------------------------------\n");
    printf("Total payments: %d  |  Refunded: %d\n", count, refunded);
    printf("Net collected:  Rs. %.2f  (Refunded: Rs. %.2f)\n", total_collected, total_refunded);
    printf("========================================================\n");
}
