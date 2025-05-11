#ifndef SDLPORTABLES_H
#define SDLPORTABLES_H

// resources directory, or for WiiU, the ROMFS mount romfs:/
extern const char* RES_ROOT;

// standalone programs we load into our main with minor changes
int sdltriangle_main();
int sdlanimate_main();
int sdlaudio1_main(int argc, char *argv[]);

#endif // SDLPORTABLES_H
