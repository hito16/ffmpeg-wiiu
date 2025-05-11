/* A generic SDL draw triangle example that compiles for Mac OS,
  and with small changes, compiles on WIIU
*/

//  WIIU code change 1.
//  WUT homebrew pkgconfig includes the SDL sub directories
#ifdef __WIIU__
#include <SDL.h>
#include <SDL_mixer.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#endif

#include <stdio.h>

void event_loop();

//  WIIU code change 2.
//  move main to sdlaudio1_main
int sdlaudio1_main(int argc, char* argv[]) {
    // int main(int argc, char *argv[]) {

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    // Screen dimensions
    const int SCREEN_WIDTH = 640;
    const int SCREEN_HEIGHT = 480;

    // Check if the filename argument was provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <mp3_filename>\n", argv[0]);
        return 1;
    }

    const char* mp3Filename = argv[1];

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS |
                 SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL Error: %s\n",
                SDL_GetError());
        return 1;
    }

    // Create window and renderer
    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN,
                                &window, &renderer);
    if (!window || !renderer) {
        printf("SDL Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderDrawColor(renderer, 0, 128, 128, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    // Initialize SDL_mixer

    Mix_Init(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG |
             MIX_INIT_MID);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096);
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "SDL_mixer could not initialize! SDL_mixer Error: %s\n",
                Mix_GetError());
        SDL_Quit();
        return 1;
    }

    // Load the MP3 file from the command-line argument
    Mix_Music* music = Mix_LoadMUS(mp3Filename);
    if (music == NULL) {
        fprintf(stderr, "Failed to load music '%s'! SDL_mixer Error: %s\n",
                mp3Filename, Mix_GetError());
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    // Play the music
    if (Mix_PlayMusic(music, -1) == -1) {  // -1 means loop indefinitely
        fprintf(stderr, "Failed to play music! SDL_mixer Error: %s\n",
                Mix_GetError());
        Mix_FreeMusic(music);
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    printf("Playing music: %s. Press 'q' to quit.\n", mp3Filename);
    event_loop();

    // Free resources
    Mix_FreeMusic(music);
    Mix_Quit();
    SDL_Quit();

    return 0;
}

void event_loop() {
    SDL_GameController* pad;
    SDL_Event e;
    SDL_bool quit = SDL_FALSE;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = SDL_TRUE;
            } else if (e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                printf("SDL_KEYDOWN or SDL_MOUSEBUTTONDOWN\n");
                quit = SDL_TRUE;
                // WIIU code change 3c.
                // Add controller events so we can exit on any button push
            } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                quit = SDL_TRUE;
                printf("SDL_PollEvent revd SDL_CONTROLLERBUTTONDOWN\n");
            } else if (e.type == SDL_CONTROLLERDEVICEADDED) {
                pad = SDL_GameControllerOpen(e.cdevice.which);
                printf("Added controller %d: %s\n", e.cdevice.which,
                       SDL_GameControllerName(pad));
            } else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
                pad = SDL_GameControllerFromInstanceID(e.cdevice.which);
                printf("Removed controller: %s\n", SDL_GameControllerName(pad));
                SDL_GameControllerClose(pad);
            }
        }
    }
}