#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>

// SDL Headers for threading
#include <SDL.h>
#include <SDL_thread.h>

// Mongoose HTTP server
#include "mongoose.h"

// --- Configuration ---
#define WEB_SERVER_PORT "8080"
#define CONFIG_FORM_PATH "/configure"
#define SUBMIT_PATH "/submit_message"
#define MAX_SUBMITTED_STRING_LEN 256

// --- Global Variables for Web Server and Main App Communication ---
static struct mg_mgr mgr; // Mongoose event manager
static char device_ip_address[INET_ADDRSTRLEN] = "";

// Variables for communication between web server thread and main thread
static char g_submitted_string[MAX_SUBMITTED_STRING_LEN];
static SDL_mutex *g_string_mutex = NULL;
static SDL_cond *g_string_cond = NULL;
static volatile bool g_new_string_flag = false;
static volatile bool g_web_server_running = true; // New flag for server control

// --- HTML Content for the Web Form ---
static const char *s_config_form_html =
    "<!DOCTYPE html>"
    "<html><head><title>Device Message</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<style>"
    "body { font-family: sans-serif; margin: 20px; background-color: #f0f0f0; }"
    "h1 { color: #333; }"
    "form { background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); max-width: 400px; margin: auto; }"
    "label { display: block; margin-bottom: 8px; color: #555; }"
    "input[type='text'] { width: calc(100% - 22px); padding: 10px; margin-bottom: 15px; border: 1px solid #ccc; border-radius: 4px; }"
    "input[type='submit'] { background-color: #007bff; color: white; padding: 12px 20px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }"
    "input[type='submit']:hover { background-color: #0056b3; }"
    ".message { padding: 15px; margin-top: 20px; border-radius: 8px; }"
    ".success { background-color: #d4edda; color: #155724; border: 1px solid #c3e6cb; }"
    ".error { background-color: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }"
    "</style>"
    "</head><body>"
    "<form action='" SUBMIT_PATH "' method='POST'>"
    "  <h1>Enter Your Message</h1>"
    "  <p>Type a short message or command:</p>"
    "  <label for='user_message'>Message:</label>"
    "  <input type='text' id='user_message' name='user_message' value='Hello Device!' maxlength='%d'><br><br>"
    "  <input type='submit' value='Send Message'>"
    "</form>"
    "</body></html>";

// --- Function to find the device's primary IP address ---
bool get_local_ip_address(char *ip_buffer, size_t buffer_size) {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    bool found_valid_ip = false;

    ip_buffer[0] = '\0';

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        fprintf(stderr, "ERROR: Could not retrieve network interface information.\n");
        return false;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) { // IPv4
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s for interface %s\n", gai_strerror(s), ifa->ifa_name ? ifa->ifa_name : "unknown");
                continue;
            }
            if (strcmp(host, "127.0.0.1") != 0 && strncmp(host, "169.254.", 8) != 0) {
                strncpy(ip_buffer, host, buffer_size);
                ip_buffer[buffer_size - 1] = '\0';
                found_valid_ip = true;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);

    if (!found_valid_ip) {
        fprintf(stderr, "ERROR: No suitable non-loopback IPv4 address found on any active interface.\n");
    }
    return found_valid_ip;
}

// --- Mongoose Event Handler ---
static void mg_event_handler(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    if (hm->uri.len == strlen(CONFIG_FORM_PATH) && memcmp(hm->uri.buf, CONFIG_FORM_PATH, hm->uri.len) == 0) {
      mg_http_reply(c, 200, "Content-Type: text/html\r\n", s_config_form_html, MAX_SUBMITTED_STRING_LEN -1);
    } else if (hm->uri.len == strlen(SUBMIT_PATH) && memcmp(hm->uri.buf, SUBMIT_PATH, hm->uri.len) == 0) {
      char user_message_buf[MAX_SUBMITTED_STRING_LEN];

      mg_http_get_var(&hm->body, "user_message", user_message_buf, sizeof(user_message_buf));

      // --- Communication to Main Thread (using SDL mutex/cond) ---
      SDL_LockMutex(g_string_mutex);
      strncpy(g_submitted_string, user_message_buf, sizeof(g_submitted_string) - 1);
      g_submitted_string[sizeof(g_submitted_string) - 1] = '\0';
      g_new_string_flag = true;
      g_web_server_running = false; // Signal server to stop
      SDL_CondSignal(g_string_cond); // Signal main thread
      SDL_UnlockMutex(g_string_mutex);
      // --- End Communication ---

      mg_http_reply(c, 200, "Content-Type: text/html\r\n",
                    "<!DOCTYPE html><html><head><style>body{font-family:sans-serif;margin:20px;}</style></head><body>"
                    "<div class='message success'><h1>Message Received!</h1>"
                    "<p>Your message: <b>%s</b></p>"
                    "<p>The device has processed your input.</p>"
                    "<p>The web server is now shutting down.</p>"
                    "</div></body></html>",
                    user_message_buf);

    } else {
      mg_http_reply(c, 404, "Content-Type: text/html\r\n", "Not Found");
    }
  }
}

// --- Web Server Thread Function ---
int web_server_thread_func(void *arg) {
  (void) arg;
  mg_mgr_init(&mgr);

  char listening_url[INET_ADDRSTRLEN + 10];
  snprintf(listening_url, sizeof(listening_url), "http://0.0.0.0:%s", WEB_SERVER_PORT);

  printf("Mongoose web server attempting to start on %s\n", listening_url);
  printf("Access device configuration at: http://%s:%s%s\n", device_ip_address, WEB_SERVER_PORT, CONFIG_FORM_PATH);

  struct mg_connection *c = mg_http_listen(&mgr, listening_url, mg_event_handler, NULL);
  if (c == NULL) {
    fprintf(stderr, "Error: Cannot start Mongoose server on URL %s. Is port already in use, or is the URL malformed?\n", listening_url);
    mg_mgr_free(&mgr);
    return 1;
  }

  // Loop while the g_web_server_running flag is true
  while (g_web_server_running) {
    mg_mgr_poll(&mgr, 100); // Poll for events, wait up to 100ms
  }

  mg_mgr_free(&mgr); // Clean up Mongoose resources
  printf("Mongoose web server thread terminated.\n");
  return 0;
}

// --- Main Application Entry Point ---
int main_app_entry_point() {
    printf("Embedded Device Message App Starting...\n");

    if (SDL_Init(SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    g_string_mutex = SDL_CreateMutex();
    if (!g_string_mutex) {
        fprintf(stderr, "SDL_CreateMutex failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    g_string_cond = SDL_CreateCond();
    if (!g_string_cond) {
        fprintf(stderr, "SDL_CreateCond failed: %s\n", SDL_GetError());
        SDL_DestroyMutex(g_string_mutex);
        SDL_Quit();
        return 1;
    }

    printf("Attempting to discover device IP address...\n");
    if (!get_local_ip_address(device_ip_address, sizeof(device_ip_address))) {
        SDL_DestroyMutex(g_string_mutex);
        SDL_DestroyCond(g_string_cond);
        SDL_Quit();
        return 1;
    }
    printf("Device IP Address: %s\n", device_ip_address);

    SDL_Thread *web_server_thread_id = SDL_CreateThread(web_server_thread_func, "WebServerThread", NULL);
    if (!web_server_thread_id) {
        fprintf(stderr, "Error creating web server thread: %s\n", SDL_GetError());
        SDL_DestroyMutex(g_string_mutex);
        SDL_DestroyCond(g_string_cond);
        SDL_Quit();
        return 1;
    }
    printf("Web server thread launched.\n");

    // --- Main embedded application logic ---
    printf("Main application running and listening for messages...\n");
    int heartbeat_counter = 0;

    // The main loop now only runs once, waiting for the first message,
    // then signals the web server to shut down.
    while (true) {
        printf("Main App Heartbeat: %d\n", heartbeat_counter++);

        SDL_LockMutex(g_string_mutex);
        int wait_result = SDL_CondWaitTimeout(g_string_cond, g_string_mutex, 5000); // Wait for up to 5 seconds for a message

        if (g_new_string_flag) {
            printf("\nMAIN APP RECEIVED MESSAGE: \"%s\"\n", g_submitted_string);

            // --- Your device-specific logic for the received string goes here ---
            // Process the string, e.g., turn on/off a component
            // -------------------------------------------------------------------

            g_new_string_flag = false;
            memset(g_submitted_string, 0, sizeof(g_submitted_string));

            // Server shutdown logic:
            printf("Message received. Signaling web server to shut down...\n");
            // g_web_server_running is set to false in mg_event_handler
            // and the condition variable is signaled.
            // The web server thread will pick up the flag and exit its loop.
            SDL_UnlockMutex(g_string_mutex); // Unlock before waiting for thread
            break; // Exit main app loop after receiving the first message and signaling shutdown
        }
        SDL_UnlockMutex(g_string_mutex);

        // If after 5 seconds, no message was received, the main app continues its heartbeat
        // until a message is received or the app is otherwise terminated.
        if (!g_web_server_running) { // Check if web server was signaled to stop by other means
            printf("Web server has been signaled to stop from elsewhere. Exiting main app loop.\n");
            break;
        }
    }

    // Wait for the web server thread to finish its cleanup and exit
    SDL_WaitThread(web_server_thread_id, NULL);
    printf("Web server thread has completed shutdown.\n");


    // Clean up SDL resources
    SDL_DestroyMutex(g_string_mutex);
    SDL_DestroyCond(g_string_cond);
    SDL_Quit();
    printf("Device message app exiting.\n");
    return 0;
}

int main(int argc, char *argv[]) {
    return main_app_entry_point();
}
