#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>

#define PORT 8080
#define MAX_SEATS 20
#define MAX_BOOKINGS 200
#define MAX_PASSENGERS 10
#define MAX_NAME 80
#define MAX_BODY 16384
#define FARE 450
#define CONVENIENCE_FEE 20

typedef struct {
    int booked[MAX_SEATS + 1];
    int total_bookings;
    int total_cancellations;
} SharedData;

typedef struct {
    char name[MAX_NAME];
    int age;
    char gender[16];
    int seat;
} Passenger;

typedef struct {
    char id[32];
    int active;
    int seat_count;
    int seats[MAX_PASSENGERS];
    Passenger passengers[MAX_PASSENGERS];
    int fare;
    int fee;
    int total;
    char date[32];
    char departure[16];
} Booking;

SharedData *shared_data;
sem_t *seat_sem;
Booking bookings[MAX_BOOKINGS];
int booking_count = 0;

char response_buffer[65536];

void send_response(int client, int status, const char *type, const char *body) {
    char header[1024];

    const char *status_text = "OK";

    if (status == 201) status_text = "Created";
    if (status == 400) status_text = "Bad Request";
    if (status == 404) status_text = "Not Found";
    if (status == 409) status_text = "Conflict";
    if (status == 500) status_text = "Internal Server Error";

    snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status,
        status_text,
        type,
        strlen(body)
    );

    send(client, header, strlen(header), 0);
    send(client, body, strlen(body), 0);
}

void send_json(int client, int status, const char *json) {
    send_response(client, status, "application/json", json);
}

void send_options(int client) {
    send_response(client, 200, "text/plain", "");
}

void json_escape(const char *input, char *output, size_t size) {
    size_t j = 0;

    for (size_t i = 0; input[i] && j + 2 < size; i++) {
        if (input[i] == '"' || input[i] == '\\') {
            if (j + 2 >= size) break;
            output[j++] = '\\';
            output[j++] = input[i];
        } else if (input[i] == '\n') {
            if (j + 2 >= size) break;
            output[j++] = '\\';
            output[j++] = 'n';
        } else {
            output[j++] = input[i];
        }
    }

    output[j] = '\0';
}

int json_int(const char *body, const char *key, int fallback) {
    char pattern[128];
    char *p;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    p = strstr(body, pattern);

    if (!p)
        return fallback;

    p = strchr(p, ':');

    if (!p)
        return fallback;

    return atoi(p + 1);
}

int json_string(
    const char *body,
    const char *key,
    char *output,
    size_t output_size
) {
    char pattern[128];
    char *p;
    char *start;
    char *end;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    p = strstr(body, pattern);

    if (!p)
        return 0;

    p = strchr(p, ':');

    if (!p)
        return 0;

    p++;

    while (*p && isspace((unsigned char)*p))
        p++;

    if (*p != '"')
        return 0;

    start = p + 1;
    end = start;

    while (*end) {
        if (*end == '"' && *(end - 1) != '\\')
            break;

        end++;
    }

    if (!*end)
        return 0;

    size_t length = (size_t)(end - start);

    if (length >= output_size)
        length = output_size - 1;

    memcpy(output, start, length);
    output[length] = '\0';

    return 1;
}

int extract_seats(const char *body, int *seats, int max_seats) {
    char *p = strstr(body, "\"seats\"");

    if (!p)
        return 0;

    p = strchr(p, '[');

    if (!p)
        return 0;

    p++;

    int count = 0;

    while (*p && *p != ']' && count < max_seats) {

        while (*p && (isspace((unsigned char)*p) || *p == ','))
            p++;

        if (*p == ']')
            break;

        if (isdigit((unsigned char)*p)) {
            seats[count++] = atoi(p);

            while (isdigit((unsigned char)*p))
                p++;
        } else {
            p++;
        }
    }

    return count;
}

void generate_booking_id(char *id, size_t size) {
    const char chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    srand((unsigned int)(time(NULL) ^ getpid() ^ booking_count));

    char random_part[9];

    for (int i = 0; i < 8; i++)
        random_part[i] = chars[rand() % (sizeof(chars) - 1)];

    random_part[8] = '\0';

    snprintf(id, size, "HX-%s", random_part);
}

void generate_ticket(Booking *booking) {
    char filename[256];

    snprintf(
        filename,
        sizeof(filename),
        "tickets/HERMEX_%s.txt",
        booking->id
    );

    FILE *file = fopen(filename, "w");

    if (!file)
        return;

    fprintf(file, "============================================================\n");
    fprintf(file, "                         HERMEX\n");
    fprintf(file, "                        E-TICKET\n");
    fprintf(file, "============================================================\n\n");

    fprintf(file, "BOOKING ID     : %s\n\n", booking->id);

    fprintf(file, "FROM           : Chennai\n");
    fprintf(file, "TO             : Bengaluru\n");
    fprintf(file, "DATE           : %s\n", booking->date);
    fprintf(file, "DEPARTURE      : %s\n\n", booking->departure);

    fprintf(file, "BUS            : CITYLINE EXPRESS 204\n");
    fprintf(file, "TYPE           : AC SEATER\n");
    fprintf(file, "LAYOUT         : 2 + 2\n\n");

    fprintf(file, "------------------------------------------------------------\n");
    fprintf(file, "PASSENGER DETAILS\n");
    fprintf(file, "------------------------------------------------------------\n\n");

    for (int i = 0; i < booking->seat_count; i++) {
        fprintf(file, "SEAT %02d\n", booking->passengers[i].seat);
        fprintf(file, "NAME           : %s\n", booking->passengers[i].name);
        fprintf(file, "AGE            : %d\n", booking->passengers[i].age);
        fprintf(file, "GENDER         : %s\n\n", booking->passengers[i].gender);
    }

    fprintf(file, "------------------------------------------------------------\n");

    fprintf(file, "NUMBER OF SEATS : %d\n", booking->seat_count);
    fprintf(file, "TICKET FARE     : INR %d\n", booking->fare);
    fprintf(file, "CONVENIENCE FEE : INR %d\n", booking->fee);
    fprintf(file, "TOTAL           : INR %d\n\n", booking->total);

    fprintf(file, "------------------------------------------------------------\n");
    fprintf(file, "                  BOOKING CONFIRMED\n");
    fprintf(file, "------------------------------------------------------------\n\n");

    fprintf(file, "Thank you for travelling with HERMEX.\n");

    fclose(file);
}

int reserve_seats(Booking *booking) {
    int pipefd[2];

    if (pipe(pipefd) == -1)
        return 0;

    pid_t pid = fork();

    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 0;
    }

    if (pid == 0) {
        close(pipefd[0]);

        int success = 1;

        if (sem_wait(seat_sem) != 0)
            success = 0;

        if (success) {

            for (int i = 0; i < booking->seat_count; i++) {
                int seat = booking->seats[i];

                if (
                    seat < 1 ||
                    seat > MAX_SEATS ||
                    shared_data->booked[seat]
                ) {
                    success = 0;
                    break;
                }
            }

            if (success) {

                for (int i = 0; i < booking->seat_count; i++) {
                    shared_data->booked[booking->seats[i]] = 1;
                }

                shared_data->total_bookings++;
            }

            sem_post(seat_sem);
        }

        write(pipefd[1], &success, sizeof(success));

        close(pipefd[1]);

        _exit(success ? 0 : 1);
    }

    close(pipefd[1]);

    int result = 0;

    read(pipefd[0], &result, sizeof(result));

    close(pipefd[0]);

    waitpid(pid, NULL, 0);

    return result;
}

void release_seats(Booking *booking) {
    sem_wait(seat_sem);

    for (int i = 0; i < booking->seat_count; i++) {
        int seat = booking->seats[i];

        if (seat >= 1 && seat <= MAX_SEATS)
            shared_data->booked[seat] = 0;
    }

    shared_data->total_cancellations++;

    sem_post(seat_sem);
}

void handle_buses(int client) {
    const char *json =
        "{"
        "\"success\":true,"
        "\"buses\":["
        "{"
        "\"id\":\"cityline-204\","
        "\"name\":\"CITYLINE EXPRESS 204\","
        "\"rating\":4.6,"
        "\"from\":\"Chennai\","
        "\"to\":\"Bengaluru\","
        "\"departure\":\"09:30 PM\","
        "\"arrival\":\"06:15 AM\","
        "\"duration\":\"8h 45m\","
        "\"type\":\"AC Seater\","
        "\"layout\":\"2+2\","
        "\"fare\":450,"
        "\"amenities\":[\"Charging\",\"Live Tracking\",\"Reclining Seats\"]"
        "},"
        "{"
        "\"id\":\"metro-118\","
        "\"name\":\"METRO TRAVELS 118\","
        "\"rating\":4.4,"
        "\"from\":\"Chennai\","
        "\"to\":\"Bengaluru\","
        "\"departure\":\"10:15 PM\","
        "\"arrival\":\"07:00 AM\","
        "\"duration\":\"8h 45m\","
        "\"type\":\"AC Seater\","
        "\"layout\":\"2+2\","
        "\"fare\":520,"
        "\"amenities\":[\"Charging\",\"Blanket\",\"Water\"]"
        "}"
        "]"
        "}";

    send_json(client, 200, json);
}

void handle_seats(int client) {
    char json[8192];

    int available = 0;

    char *p = json;

    p += sprintf(p, "{");
    p += sprintf(p, "\"success\":true,");
    p += sprintf(p, "\"seats\":[");

    for (int i = 1; i <= MAX_SEATS; i++) {

        if (!shared_data->booked[i])
            available++;

        p += sprintf(
            p,
            "{\"number\":%d,\"status\":\"%s\"}%s",
            i,
            shared_data->booked[i] ? "booked" : "available",
            i == MAX_SEATS ? "" : ","
        );
    }

    p += sprintf(p, "],");
    p += sprintf(p, "\"available\":%d,", available);
    p += sprintf(p, "\"booked\":%d,", MAX_SEATS - available);
    p += sprintf(p, "\"total\":%d", MAX_SEATS);
    p += sprintf(p, "}");

    send_json(client, 200, json);
}

int parse_passengers(
    const char *body,
    Booking *booking
) {
    char *p = strstr(body, "\"passengers\"");

    if (!p)
        return 0;

    p = strchr(p, '[');

    if (!p)
        return 0;

    int count = 0;

    while (*p && *p != ']' && count < booking->seat_count) {

        char *object_start = strchr(p, '{');

        if (!object_start)
            break;

        char *object_end = strchr(object_start, '}');

        if (!object_end)
            break;

        size_t length =
            (size_t)(object_end - object_start + 1);

        char object[1024];

        if (length >= sizeof(object))
            length = sizeof(object) - 1;

        memcpy(object, object_start, length);
        object[length] = '\0';

        char name[MAX_NAME] = "";
        char gender[16] = "";

        json_string(
            object,
            "name",
            name,
            sizeof(name)
        );

        json_string(
            object,
            "gender",
            gender,
            sizeof(gender)
        );

        int age =
            json_int(object, "age", 0);

        int seat =
            json_int(object, "seat", 0);

        strncpy(
            booking->passengers[count].name,
            name,
            MAX_NAME - 1
        );

        booking->passengers[count].age =
            age;

        strncpy(
            booking->passengers[count].gender,
            gender,
            sizeof(booking->passengers[count].gender) - 1
        );

        booking->passengers[count].seat =
            seat;

        count++;

        p = object_end + 1;
    }

    return count;
}

void handle_booking(int client, const char *body) {

    if (booking_count >= MAX_BOOKINGS) {
        send_json(
            client,
            500,
            "{\"success\":false,\"message\":\"Booking storage full\"}"
        );
        return;
    }

    Booking booking;

    memset(&booking, 0, sizeof(booking));

    int seats[MAX_PASSENGERS];

    int seat_count =
        extract_seats(
            body,
            seats,
            MAX_PASSENGERS
        );

    if (seat_count <= 0) {
        send_json(
            client,
            400,
            "{\"success\":false,\"message\":\"No seats selected\"}"
        );
        return;
    }

    booking.seat_count =
        seat_count;

    for (int i = 0; i < seat_count; i++)
        booking.seats[i] =
            seats[i];

    time_t now =
        time(NULL);

    struct tm *tm_info =
        localtime(&now);

    strftime(
        booking.date,
        sizeof(booking.date),
        "%d %b %Y",
        tm_info
    );

    strcpy(
        booking.departure,
        "09:30 PM"
    );

    int passenger_count =
        parse_passengers(
            body,
            &booking
        );

    if (passenger_count != seat_count) {

        send_json(
            client,
            400,
            "{\"success\":false,\"message\":\"Passenger details do not match selected seats\"}"
        );

        return;
    }

    for (int i = 0; i < seat_count; i++) {

        if (
            booking.passengers[i].seat !=
            booking.seats[i]
        ) {
            send_json(
                client,
                400,
                "{\"success\":false,\"message\":\"Passenger seat mismatch\"}"
            );

            return;
        }
    }

    booking.fare =
        seat_count * FARE;

    booking.fee =
        CONVENIENCE_FEE;

    booking.total =
        booking.fare +
        booking.fee;

    generate_booking_id(
        booking.id,
        sizeof(booking.id)
    );

    if (!reserve_seats(&booking)) {

        send_json(
            client,
            409,
            "{\"success\":false,\"message\":\"One or more selected seats are no longer available\"}"
        );

        return;
    }

    booking.active = 1;

    bookings[booking_count++] =
        booking;

    generate_ticket(&booking);

    char json[4096];

    char passengers_json[2048] = "";

    char *pp =
        passengers_json;

    pp += sprintf(pp, "[");

    for (int i = 0; i < booking.seat_count; i++) {

        char safe_name[MAX_NAME * 2];

        json_escape(
            booking.passengers[i].name,
            safe_name,
            sizeof(safe_name)
        );

        pp += sprintf(
            pp,
            "{\"name\":\"%s\",\"age\":%d,\"gender\":\"%s\",\"seat\":%d}%s",
            safe_name,
            booking.passengers[i].age,
            booking.passengers[i].gender,
            booking.passengers[i].seat,
            i == booking.seat_count - 1 ? "" : ","
        );
    }

    pp += sprintf(pp, "]");

    snprintf(
        json,
        sizeof(json),
        "{"
        "\"success\":true,"
        "\"message\":\"Booking confirmed\","
        "\"bookingId\":\"%s\","
        "\"from\":\"Chennai\","
        "\"to\":\"Bengaluru\","
        "\"bus\":\"CITYLINE EXPRESS 204\","
        "\"date\":\"%s\","
        "\"departure\":\"%s\","
        "\"seats\":%d,"
        "\"fare\":%d,"
        "\"fee\":%d,"
        "\"total\":%d,"
        "\"ticket\":\"tickets/HERMEX_%s.txt\","
        "\"passengers\":%s"
        "}",
        booking.id,
        booking.date,
        booking.departure,
        booking.seat_count,
        booking.fare,
        booking.fee,
        booking.total,
        booking.id,
        passengers_json
    );

    send_json(
        client,
        201,
        json
    );
}

void handle_bookings(int client) {

    char json[16384];

    char *p =
        json;

    p += sprintf(
        p,
        "{\"success\":true,\"bookings\":["
    );

    int first = 1;

    for (int i = 0; i < booking_count; i++) {

        if (!bookings[i].active)
            continue;

        Booking *b =
            &bookings[i];

        if (!first)
            p += sprintf(p, ",");

        first = 0;

        p += sprintf(
            p,
            "{"
            "\"bookingId\":\"%s\","
            "\"from\":\"Chennai\","
            "\"to\":\"Bengaluru\","
            "\"bus\":\"CITYLINE EXPRESS 204\","
            "\"date\":\"%s\","
            "\"departure\":\"%s\","
            "\"seatCount\":%d,"
            "\"fare\":%d,"
            "\"fee\":%d,"
            "\"total\":%d,"
            "\"seats\":[",
            b->id,
            b->date,
            b->departure,
            b->seat_count,
            b->fare,
            b->fee,
            b->total
        );

        for (int j = 0; j < b->seat_count; j++) {

            p += sprintf(
                p,
                "%d%s",
                b->seats[j],
                j == b->seat_count - 1 ? "" : ","
            );
        }

        p += sprintf(
            p,
            "]}"
        );
    }

    p += sprintf(p, "]}");

    send_json(client, 200, json);
}

void handle_cancel(int client, const char *body) {

    char id[64] = "";

    if (!json_string(
            body,
            "bookingId",
            id,
            sizeof(id)
        )) {

        send_json(
            client,
            400,
            "{\"success\":false,\"message\":\"Booking ID required\"}"
        );

        return;
    }

    for (int i = 0; i < booking_count; i++) {

        if (
            bookings[i].active &&
            strcmp(bookings[i].id, id) == 0
        ) {

            release_seats(
                &bookings[i]
            );

            bookings[i].active = 0;

            char json[512];

            snprintf(
                json,
                sizeof(json),
                "{"
                "\"success\":true,"
                "\"message\":\"Booking cancelled\","
                "\"bookingId\":\"%s\""
                "}",
                id
            );

            send_json(
                client,
                200,
                json
            );

            return;
        }
    }

    send_json(
        client,
        404,
        "{\"success\":false,\"message\":\"Booking not found\"}"
    );
}

void route_request(
    int client,
    const char *method,
    const char *path,
    const char *body
) {
    if (strcmp(method, "OPTIONS") == 0) {
        send_options(client);
        return;
    }

    if (
        strcmp(method, "GET") == 0 &&
        strcmp(path, "/api/buses") == 0
    ) {
        handle_buses(client);
        return;
    }

    if (
        strcmp(method, "GET") == 0 &&
        strcmp(path, "/api/seats") == 0
    ) {
        handle_seats(client);
        return;
    }

    if (
        strcmp(method, "GET") == 0 &&
        strcmp(path, "/api/bookings") == 0
    ) {
        handle_bookings(client);
        return;
    }

    if (
        strcmp(method, "POST") == 0 &&
        strcmp(path, "/api/book") == 0
    ) {
        handle_booking(client, body);
        return;
    }

    if (
        strcmp(method, "POST") == 0 &&
        strcmp(path, "/api/cancel") == 0
    ) {
        handle_cancel(client, body);
        return;
    }

    send_json(
        client,
        404,
        "{\"success\":false,\"message\":\"Route not found\"}"
    );
}

void handle_client(int client) {

    char buffer[MAX_BODY + 8192];

    memset(
        buffer,
        0,
        sizeof(buffer)
    );

    int received =
        recv(
            client,
            buffer,
            sizeof(buffer) - 1,
            0
        );

    if (received <= 0) {
        close(client);
        return;
    }

    char method[16] = "";
    char path[256] = "";

    sscanf(
        buffer,
        "%15s %255s",
        method,
        path
    );

    char *body =
        strstr(buffer, "\r\n\r\n");

    if (body)
        body += 4;
    else
        body = "";

    route_request(
        client,
        method,
        path,
        body
    );

    close(client);
}

void initialize_backend() {

    shared_data =
        mmap(
            NULL,
            sizeof(SharedData),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS,
            -1,
            0
        );

    if (shared_data == MAP_FAILED) {
        perror("shared memory");
        exit(EXIT_FAILURE);
    }

    memset(
        shared_data,
        0,
        sizeof(SharedData)
    );

    seat_sem =
        sem_open(
            "/hermex_booking_sem",
            O_CREAT,
            0600,
            1
        );

    if (seat_sem == SEM_FAILED) {
        perror("semaphore");
        exit(EXIT_FAILURE);
    }

    if (mkdir("tickets", 0755) == -1 && errno != EEXIST) {
        perror("mkdir");
        exit(EXIT_FAILURE);
    }
}

int main() {

    signal(
        SIGCHLD,
        SIG_IGN
    );

    initialize_backend();

    int server =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (server < 0) {
        perror("socket");
        return 1;
    }

    int reuse = 1;

    setsockopt(
        server,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse)
    );

    struct sockaddr_in address;

    memset(
        &address,
        0,
        sizeof(address)
    );

    address.sin_family =
        AF_INET;

    address.sin_addr.s_addr =
        INADDR_ANY;

    address.sin_port =
        htons(PORT);

    if (
        bind(
            server,
            (struct sockaddr *)&address,
            sizeof(address)
        ) < 0
    ) {
        perror("bind");
        close(server);
        return 1;
    }

    if (listen(server, 20) < 0) {
        perror("listen");
        close(server);
        return 1;
    }

    printf("\n");
    printf("=============================================\n");
    printf("                  HERMEX\n");
    printf("             BACKEND SERVER\n");
    printf("=============================================\n");
    printf("Server     : http://localhost:%d\n", PORT);
    printf("Status     : Running\n");
    printf("Seats      : %d\n", MAX_SEATS);
    printf("Fare       : INR %d\n", FARE);
    printf("=============================================\n\n");

    while (1) {

        struct sockaddr_in client_address;
        socklen_t client_length =
            sizeof(client_address);

        int client =
            accept(
                server,
                (struct sockaddr *)&client_address,
                &client_length
            );

        if (client < 0) {

            if (errno == EINTR)
                continue;

            perror("accept");
            continue;
        }

        handle_client(client);
    }

    close(server);

    sem_close(seat_sem);
    sem_unlink("/hermex_booking_sem");

    munmap(
        shared_data,
        sizeof(SharedData)
    );

    return 0;
}