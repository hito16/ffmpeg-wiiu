/* A very simple file picker.  Opens up, starts an SDL window with file 
   navigtion.  On exit, lists the file the user selected.
   On WiiU, sometimes you just want to supply a file name before the 
   app starts.
   */
#define _DEFAULT_SOURCE  // Required for dirent and realpath on some systems
#define _BSD_SOURCE      // Required for dirent and realpath on some systems

#include <SDL.h>
#include <SDL_ttf.h>
#include <dirent.h>   // For opendir, readdir
#include <limits.h>   // For PATH_MAX, NAME_MAX
#include <stdbool.h>  // For bool type
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>  // For stat
#include <unistd.h>    // For getcwd

// --- Configuration ---
#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 400
#define ITEM_HEIGHT 25
#define FONT_SIZE 20
#define PADDING 10

// Colors (SDL_Color)
const SDL_Color COLOR_WHITE = {255, 255, 255, 255};
const SDL_Color COLOR_BLACK = {0, 0, 0, 255};
const SDL_Color COLOR_BLUE = {0, 0, 255, 255};      // Directories
const SDL_Color COLOR_GREEN = {0, 255, 0, 255};     // Files
const SDL_Color COLOR_YELLOW = {255, 255, 0, 255};  // Selection background

// Controller Button Mappings (Standard SDL_GameController button indices)
#define CONTROLLER_SELECT_BUTTON SDL_CONTROLLER_BUTTON_A
#define CONTROLLER_PARENT_BUTTON SDL_CONTROLLER_BUTTON_B
#define CONTROLLER_UP_BUTTON SDL_CONTROLLER_BUTTON_DPAD_UP
#define CONTROLLER_DOWN_BUTTON SDL_CONTROLLER_BUTTON_DPAD_DOWN

// --- Data Structures ---
typedef enum {
    ITEM_TYPE_UNKNOWN,
    ITEM_TYPE_DIR,
    ITEM_TYPE_FILE,
    ITEM_TYPE_PARENT  // For ".."
} ItemType;

typedef struct {
    char name[NAME_MAX + 1];  // Max file name length
    ItemType type;
} DirItem;

typedef struct {
    DirItem* items;
    int count;
    int capacity;
} ItemList;

// Define abstract browser actions
typedef enum {
    ACTION_NONE,
    ACTION_NAV_UP,
    ACTION_NAV_DOWN,
    ACTION_SELECT,
    ACTION_PARENT,
    ACTION_QUIT_BROWSER  // Special action for quitting
} BrowserAction;

const char* FONT_PATH = "romfs/res/Roboto-Regular.ttf";

// --- Helper Functions ---

// Comparison function for sorting directory items
int compareDirItems(const void* a, const void* b) {
    DirItem* itemA = (DirItem*)a;
    DirItem* itemB = (DirItem*)b;

    // Always put ".." first
    if (itemA->type == ITEM_TYPE_PARENT) return -1;
    if (itemB->type == ITEM_TYPE_PARENT) return 1;

    // Sort directories before files
    if (itemA->type == ITEM_TYPE_DIR && itemB->type != ITEM_TYPE_DIR) return -1;
    if (itemA->type != ITEM_TYPE_DIR && itemB->type == ITEM_TYPE_DIR) return 1;

    // Sort alphabetically by name
    return strcmp(itemA->name, itemB->name);
}

// Frees the ItemList
void free_item_list(ItemList* list) {
    if (list->items) {
        free(list->items);
        list->items = NULL;
    }
    list->count = 0;
    list->capacity = 0;
}

// Populates the ItemList for a given directory
int get_directory_items(const char* directory, ItemList* list) {
    DIR* d;
    struct dirent* dir;
    struct stat st;
    char full_path[PATH_MAX];

    free_item_list(list);  // Clear previous list

    // Allocate initial capacity
    list->capacity = 32;
    list->items = (DirItem*)malloc(list->capacity * sizeof(DirItem));
    if (!list->items) {
        fprintf(stderr, "Error allocating memory for item list\n");
        return -1;  // Allocation error
    }
    list->count = 0;

    // Add ".." if not at root using realpath comparison
    char current_real[PATH_MAX];
    char parent_real[PATH_MAX];
    bool is_root = false;
    if (realpath(directory, current_real)) {
        char parent_dir_path[PATH_MAX];
        if (snprintf(parent_dir_path, sizeof(parent_dir_path), "%s/..",
                     current_real) < sizeof(parent_dir_path)) {
            if (realpath(parent_dir_path, parent_real)) {
                if (strcmp(current_real, parent_real) == 0) {
                    is_root = true;  // current_dir is the root
                }
            }
        }
    }

    if (!is_root) {
        if (list->count >= list->capacity) {
            list->capacity *= 2;
            DirItem* new_items = (DirItem*)realloc(
                list->items, list->capacity * sizeof(DirItem));
            if (!new_items) {
                fprintf(stderr, "Error reallocating memory for item list\n");
                return -1;
            }
            list->items = new_items;
        }
        strncpy(list->items[list->count].name, "..", NAME_MAX);
        list->items[list->count].name[NAME_MAX] =
            '\0';  // Ensure null termination
        list->items[list->count].type = ITEM_TYPE_PARENT;
        list->count++;
    }

    d = opendir(directory);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            // Skip "." and the ".." we added manually
            if (strcmp(dir->d_name, ".") == 0 ||
                strcmp(dir->d_name, "..") == 0) {
                continue;
            }

            // Construct full path to get stat relative to the *provided*
            // directory
            if (snprintf(full_path, sizeof(full_path), "%s/%s", directory,
                         dir->d_name) >= sizeof(full_path)) {
                fprintf(stderr, "Warning: Path too long for %s/%s\n", directory,
                        dir->d_name);
                continue;  // Skip this item
            }

            // Get file info
            if (stat(full_path, &st) == 0) {
                // Check if we need to reallocate
                if (list->count >= list->capacity) {
                    list->capacity *= 2;
                    DirItem* new_items = (DirItem*)realloc(
                        list->items, list->capacity * sizeof(DirItem));
                    if (!new_items) {
                        fprintf(stderr,
                                "Error reallocating memory for item list\n");
                        closedir(d);
                        free_item_list(
                            list);  // Clean up already allocated memory
                        return -1;  // Reallocation error
                    }
                    list->items = new_items;
                }

                // Add item to the list
                strncpy(list->items[list->count].name, dir->d_name, NAME_MAX);
                list->items[list->count].name[NAME_MAX] =
                    '\0';  // Ensure null termination

                if (S_ISDIR(st.st_mode)) {
                    list->items[list->count].type = ITEM_TYPE_DIR;
                } else if (S_ISREG(st.st_mode)) {
                    list->items[list->count].type = ITEM_TYPE_FILE;
                } else {
                    list->items[list->count].type = ITEM_TYPE_UNKNOWN;
                }
                list->count++;
            } else {
                // Handle stat error if necessary (e.g., permission denied)
                fprintf(stderr, "Warning: Could not stat file/dir %s\n",
                        full_path);
            }
        }
        closedir(d);

        // Sort the list
        qsort(list->items, list->count, sizeof(DirItem), compareDirItems);

    } else {
        perror("Error opening directory");
        return -1;  // Failed to open directory
    }

    return 0;  // Success
}

// Renders text to a texture
SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font,
                        const char* text, SDL_Color color) {
    if (!font) {
        fprintf(stderr, "Font is not loaded!\n");
        return NULL;
    }
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) {
        fprintf(stderr, "TTF_RenderText_Blended Error: %s\n", TTF_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface Error: %s\n",
                SDL_GetError());
    }
    SDL_FreeSurface(surface);  // Free the surface after creating texture
    return texture;
}

// --- Main Browser Function ---

/**
 * @brief Opens a minimal text-based file browser using SDL.
 *
 * @param initial_dir The starting directory for the browser.
 * @param out_buffer Buffer to fill with the selected file path.
 * @param buffer_size Size of the output buffer.
 * @return 0 on success (file selected), -1 on error, 1 on cancellation.
 */
int browse_files(const char* initial_dir, char* out_buffer,
                 size_t buffer_size) {
    // Ensure buffer is valid
    if (!out_buffer || buffer_size == 0) {
        fprintf(stderr, "Invalid output buffer provided\n");
        return -1;
    }
    out_buffer[0] = '\0';  // Null-terminate buffer initially

    // --- SDL Initialization ---
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return -1;
    }

    // --- TTF Initialization ---
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return -1;
    }

    // --- Window and Renderer Creation ---
    SDL_Window* window = SDL_CreateWindow(
        "File Browser", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // --- Font Loading ---
    // Replace "path/to/your/font.ttf" with the actual path to a .ttf font file
    // This path should be accessible from where you run the program.
    // On macOS, you might try "/Library/Fonts/Arial.ttf" or similar system
    // fonts, or place a font file in the same directory as your executable.
    TTF_Font* font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    if (!font) {
        fprintf(stderr, "TTF_OpenFont Error: %s\n", TTF_GetError());
        fprintf(stderr,
                "Please ensure \"path/to/your/font.ttf\" is a valid font file "
                "path and readable.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // --- Controller Setup ---
    SDL_GameController* game_controller = NULL;
    if (SDL_NumJoysticks() > 0) {
        // Check if the first joystick is a game controller
        if (SDL_IsGameController(0)) {
            game_controller = SDL_GameControllerOpen(
                0);  // Open the first available game controller
            if (!game_controller) {
                fprintf(stderr,
                        "Warning: Could not open game controller 0: %s\n",
                        SDL_GetError());
            } else {
                printf("Game Controller 0 opened: %s\n",
                       SDL_GameControllerName(game_controller));
            }
        } else {
            printf("Joystick 0 is not a recognized game controller.\n");
        }
    } else {
        printf("No joysticks/game controllers detected.\n");
    }

    // --- File Browser State ---
    char current_dir[PATH_MAX];
    ItemList dir_list = {NULL, 0, 0};
    int selected_index = 0;
    int scroll_offset = 0;  // Number of items scrolled off the top
    int items_per_page = (SCREEN_HEIGHT - (FONT_SIZE + PADDING) - PADDING) /
                         ITEM_HEIGHT;  // Adjusted calculation
    bool running = true;
    int return_status = 1;  // Default to cancelled (1)

    // Set initial directory
    if (realpath(initial_dir, current_dir) == NULL) {
        perror("Error resolving initial directory");
        // Try current working directory as fallback
        if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
            perror("Error getting current working directory");
            // Cannot proceed without a valid directory
            running = false;     // Exit loop immediately
            return_status = -1;  // Error (-1)
        }
    }

    // Populate initial list
    if (running) {  // Only attempt if we have a valid starting directory
        if (get_directory_items(current_dir, &dir_list) != 0) {
            // Error loading initial directory
            fprintf(stderr, "Failed to load initial directory: %s\n",
                    current_dir);
            running = false;
            return_status = -1;  // Error (-1)
        }
    }

    // --- Main Loop ---
    while (running) {
        SDL_GameController* pad;
        SDL_Event event;
        // Process all events in the queue
        while (SDL_PollEvent(&event) != 0) {
            BrowserAction action = ACTION_NONE;

            switch (event.type) {
                case SDL_QUIT:
                    action = ACTION_QUIT_BROWSER;
                    break;

                case SDL_KEYDOWN:
                    // No items to navigate if list is empty
                    if (dir_list.count == 0) break;

                    switch (event.key.keysym.sym) {
                        case SDLK_DOWN:
                            action = ACTION_NAV_DOWN;
                            break;
                        case SDLK_UP:
                            action = ACTION_NAV_UP;
                            break;
                        case SDLK_RETURN:
                            action = ACTION_SELECT;
                            break;
                        case SDLK_BACKSPACE:
                            action = ACTION_PARENT;
                            break;
                        default:
                            // Keep ACTION_NONE
                            break;
                    }
                    break;  // Break from SDL_KEYDOWN case

                case SDL_CONTROLLERBUTTONDOWN:
                    // No items to navigate if list is empty or no controller
                    if (dir_list.count == 0 || !game_controller) break;

                    switch (event.cbutton.button) {
                        case CONTROLLER_DOWN_BUTTON:
                            action = ACTION_NAV_DOWN;
                            break;
                        case CONTROLLER_UP_BUTTON:
                            action = ACTION_NAV_UP;
                            break;
                        case CONTROLLER_SELECT_BUTTON:
                            action = ACTION_SELECT;
                            break;
                        case CONTROLLER_PARENT_BUTTON:
                            action = ACTION_PARENT;
                            break;
                        default:
                            // Keep ACTION_NONE
                            break;
                    }
                    break;  // Break from SDL_CONTROLLERBUTTONDOWN case
                case SDL_CONTROLLERDEVICEADDED:
                    pad = SDL_GameControllerOpen(event.cdevice.which);
                    printf("Added controller %d: %s\n", event.cdevice.which,
                           SDL_GameControllerName(pad));
                    break;

                case SDL_CONTROLLERDEVICEREMOVED:
                    pad = SDL_GameControllerFromInstanceID(event.cdevice.which);
                    printf("Removed controller: %s\n",
                           SDL_GameControllerName(pad));
                    SDL_GameControllerClose(pad);
                    break;
                default:
                    // Keep ACTION_NONE for other event types
                    break;
            }  // End switch (event.type)

            // --- Process the determined action ---
            bool directory_changed = false;  // Flag to reset selection/scroll

            switch (action) {
                case ACTION_QUIT_BROWSER:
                    running = false;
                    return_status = 1;  // Cancelled
                    break;

                case ACTION_NAV_UP:
                    if (dir_list.count > 0) {
                        selected_index--;
                        if (selected_index < 0) {
                            selected_index = dir_list.count - 1;  // Wrap around
                        }
                    }
                    break;

                case ACTION_NAV_DOWN:
                    if (dir_list.count > 0) {
                        selected_index++;
                        if (selected_index >= dir_list.count) {
                            selected_index = 0;  // Wrap around
                        }
                    }
                    break;

                case ACTION_SELECT:
                    if (dir_list.count > 0 && selected_index >= 0 &&
                        selected_index < dir_list.count) {
                        DirItem* selected_item =
                            &dir_list.items[selected_index];
                        char next_path[PATH_MAX];
                        if (snprintf(next_path, sizeof(next_path), "%s/%s",
                                     current_dir, selected_item->name) >=
                            sizeof(next_path)) {
                            fprintf(stderr,
                                    "Error: Path too long when trying to "
                                    "access '%s'\n",
                                    selected_item->name);
                            break;  // Action failed
                        }

                        if (selected_item->type == ITEM_TYPE_DIR ||
                            selected_item->type == ITEM_TYPE_PARENT) {
                            // Navigate into directory or up
                            char resolved_path[PATH_MAX];
                            if (realpath(next_path, resolved_path)) {
                                strncpy(current_dir, resolved_path,
                                        sizeof(current_dir) - 1);
                                current_dir[sizeof(current_dir) - 1] = '\0';
                                if (get_directory_items(current_dir,
                                                        &dir_list) == 0) {
                                    directory_changed =
                                        true;  // Indicate list reloaded
                                               // successfully
                                } else {
                                    fprintf(stderr,
                                            "Failed to load directory: %s. "
                                            "Attempting reload of previous.\n",
                                            current_dir);
                                    // Re-attempt loading the *same* directory
                                    // items on failure.
                                    if (get_directory_items(current_dir,
                                                            &dir_list) != 0) {
                                        fprintf(stderr,
                                                "Failed to reload directory "
                                                "items. Exiting.\n");
                                        running = false;
                                        return_status = -1;  // Error
                                    } else {
                                        // Reloaded previous directory items
                                        directory_changed =
                                            true;  // Reset selection/scroll
                                    }
                                }
                            } else {
                                perror("Error resolving directory path");
                            }
                        } else if (selected_item->type == ITEM_TYPE_FILE) {
                            // Select file
                            char resolved_path[PATH_MAX];
                            if (realpath(next_path, resolved_path)) {
                                if (strlen(resolved_path) < buffer_size) {
                                    strncpy(out_buffer, resolved_path,
                                            buffer_size - 1);
                                    out_buffer[buffer_size - 1] = '\0';
                                    running = false;    // Exit loop
                                    return_status = 0;  // Success
                                } else {
                                    fprintf(stderr,
                                            "Selected file path '%s' exceeds "
                                            "buffer size (%zu)\n",
                                            resolved_path, buffer_size);
                                    // Stay in browser
                                }
                            } else {
                                perror("Error resolving file path");
                            }
                        }
                    }
                    break;  // Break from ACTION_SELECT

                case ACTION_PARENT: {
                    // Go up to parent directory
                    char parent_dir_path[PATH_MAX];
                    char current_real[PATH_MAX];
                    if (!realpath(current_dir, current_real)) {
                        perror(
                            "Error resolving current directory for parent "
                            "navigation");
                        break;  // Action failed
                    }
                    if (snprintf(parent_dir_path, sizeof(parent_dir_path),
                                 "%s/..",
                                 current_real) >= sizeof(parent_dir_path)) {
                        fprintf(stderr, "Error: Parent path too long\n");
                        break;  // Action failed
                    }

                    char resolved_parent_dir[PATH_MAX];
                    if (realpath(parent_dir_path, resolved_parent_dir) &&
                        strcmp(current_real, resolved_parent_dir) != 0) {
                        strncpy(current_dir, resolved_parent_dir,
                                sizeof(current_dir) - 1);
                        current_dir[sizeof(current_dir) - 1] = '\0';
                        if (get_directory_items(current_dir, &dir_list) == 0) {
                            directory_changed =
                                true;  // Indicate list reloaded successfully
                        } else {
                            fprintf(stderr,
                                    "Failed to load parent directory. Staying "
                                    "in current dir. Attempting reload.\n");
                            if (get_directory_items(current_dir, &dir_list) !=
                                0) {
                                fprintf(
                                    stderr,
                                    "Failed to reload current directory items "
                                    "after parent nav error. Exiting.\n");
                                running = false;
                                return_status = -1;  // Error
                            } else {
                                // Reloaded previous directory items
                                directory_changed =
                                    true;  // Reset selection/scroll
                            }
                        }
                    }
                    break;  // Break from ACTION_PARENT
                }
                case ACTION_NONE:
                    // Do nothing
                    break;
            }  // End switch (action)

            // Reset selection and scroll if directory changed
            if (directory_changed) {
                selected_index = 0;
                scroll_offset = 0;
            }

            // If running became false inside event processing, break the inner
            // loop
            if (!running) break;

        }  // End while (SDL_PollEvent)

        // If running became false in event loop (e.g., load failure or file
        // selected), break outer loop
        if (!running) break;

        // --- Scrolling Logic (Ensure selected index and scroll_offset are
        // valid) --- Ensure selected_index is valid if dir_list changed (e.g.,
        // reload failed)
        if (selected_index >= dir_list.count) {
            selected_index = dir_list.count > 0 ? dir_list.count - 1 : 0;
        }
        if (selected_index < 0) selected_index = 0;

        // Adjust scroll_offset to keep the selected item visible and within
        // bounds
        if (dir_list.count == 0) {
            scroll_offset = 0;
        } else if (dir_list.count <= items_per_page) {
            scroll_offset = 0;
        } else {
            if (selected_index < scroll_offset) {
                scroll_offset = selected_index;
            } else if (selected_index >= scroll_offset + items_per_page) {
                scroll_offset = selected_index - items_per_page + 1;
            }
            if (scroll_offset < 0) scroll_offset = 0;
            if (scroll_offset > dir_list.count - items_per_page) {
                scroll_offset = dir_list.count - items_per_page;
            }
        }

        // --- Drawing ---
        SDL_SetRenderDrawColor(renderer, COLOR_WHITE.r, COLOR_WHITE.g,
                               COLOR_WHITE.b, COLOR_WHITE.a);
        SDL_RenderClear(renderer);

        // Draw current path
        SDL_Texture* path_texture =
            renderText(renderer, font, current_dir, COLOR_BLACK);
        if (path_texture) {
            int texW, texH;
            SDL_QueryTexture(path_texture, NULL, NULL, &texW, &texH);
            SDL_Rect dstrect = {PADDING, PADDING, texW, texH};
            SDL_RenderCopy(renderer, path_texture, NULL, &dstrect);
            SDL_DestroyTexture(path_texture);  // Free texture
        }

        // Draw file/directory list
        int start_y = FONT_SIZE + PADDING * 2;  // Position below the path
        for (int i = 0; i < items_per_page; ++i) {
            int item_display_index = scroll_offset + i;
            if (item_display_index >= dir_list.count) {
                break;  // No more items to draw within visible area
            }

            DirItem* current_item = &dir_list.items[item_display_index];
            SDL_Color item_color;
            const char* prefix;

            // Use colors for distinction
            switch (current_item->type) {
                case ITEM_TYPE_DIR:
                    item_color = COLOR_BLUE;  // Blue for directories
                    prefix = "[DIR] ";
                    break;
                case ITEM_TYPE_FILE:
                    item_color = COLOR_BLACK;
                    prefix = "      ";
                    break;
                case ITEM_TYPE_PARENT:
                    item_color = COLOR_BLACK;  // White for ".."
                    prefix = "[UP]  ";
                    break;
                case ITEM_TYPE_UNKNOWN:
                default:
                    item_color = COLOR_BLACK;
                    prefix = "      ";  // Padding
                    break;
            }

            char item_text[NAME_MAX +
                           10];  // Room for prefix + name + null terminator
            if (snprintf(item_text, sizeof(item_text), "%s%s", prefix,
                         current_item->name) >= sizeof(item_text)) {
                // Should not happen with NAME_MAX + 10, but safety first
                strncpy(item_text, "Name Too Long", sizeof(item_text) - 1);
                item_text[sizeof(item_text) - 1] = '\0';
            }

            // Draw selection highlight
            if (item_display_index == selected_index) {
                SDL_SetRenderDrawColor(renderer, COLOR_YELLOW.r, COLOR_YELLOW.g,
                                       COLOR_YELLOW.b, COLOR_YELLOW.a);
                SDL_Rect highlight_rect = {0, start_y + i * ITEM_HEIGHT,
                                           SCREEN_WIDTH, ITEM_HEIGHT};
                SDL_RenderFillRect(renderer, &highlight_rect);
            }

            // Draw item text
            SDL_Texture* item_texture =
                renderText(renderer, font, item_text, item_color);
            if (item_texture) {
                int texW, texH;
                SDL_QueryTexture(item_texture, NULL, NULL, &texW,
                                 &texH);  // Corrected call
                SDL_Rect dstrect = {
                    PADDING,
                    start_y + i * ITEM_HEIGHT + (ITEM_HEIGHT - texH) / 2, texW,
                    texH};  // Center vertically in item height
                SDL_RenderCopy(renderer, item_texture, NULL, &dstrect);
                SDL_DestroyTexture(item_texture);  // Free texture
            }
        }

        SDL_RenderPresent(renderer);
    }  // End main loop

    // --- Cleanup ---
    free_item_list(&dir_list);
    if (game_controller) {
        SDL_GameControllerClose(game_controller);
    }
    if (font) {
        TTF_CloseFont(font);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    TTF_Quit();
    SDL_Quit();

    return return_status;
}

// --- Example Usage ---
int main(int argc, char* argv[]) {
    char selected_file_buffer[PATH_MAX];  // Buffer to receive the selected path

    printf("Opening file browser...\n");

    // Call the browser function
    // You can replace "." with a specific directory path like
    // "/Users/yourusername/Documents"
    int result =
        browse_files(".", selected_file_buffer, sizeof(selected_file_buffer));

    if (result == 0) {
        // File was selected successfully
        printf("\nSelected file: %s\n", selected_file_buffer);
    } else if (result == 1) {
        // Browser was cancelled
        printf("\nFile selection cancelled.\n");
    } else {
        // An error occurred
        printf("\nAn error occurred during Browse.\n");
    }

    return 0;
}
