#include <SDL2/SDL.h>

//screen dimensions costants
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 960

//data structure holding the objects needed to create a window and draw on it
struct interface {
    SDL_Window * window = NULL;
    SDL_Surface * surface = NULL;
    SDL_Renderer * renderer = NULL;
};

//function which inits the sdl and creates an interface object
interface init() {
    interface screen;
	printf("init\n");
    SDL_Init(SDL_INIT_VIDEO);
    screen.window = SDL_CreateWindow("", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
	if(!screen.window){
		printf("Create windows fail: %s\n", SDL_GetError());
	}
    //screen.surface = SDL_GetWindowSurface(screen.window);
	//if(!screen.surface){
	//	printf("GetWindowSurface error: %s\n", SDL_GetError());
	//}

	//SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB32);
	//SDL_ConvertSurface(screen.surface, format, 0);
	//SDL_FreeFormat(format);
    screen.renderer = SDL_CreateRenderer(screen.window, -1, SDL_RENDERER_ACCELERATED);
	if(!screen.renderer){
		printf("CreateRenderer Error: %s\n", SDL_GetError());
	}
    return screen;
}

//function to free the memory and close the sdl application
void close(interface screen) {
    SDL_DestroyRenderer(screen.renderer);
    SDL_DestroyWindow(screen.window);
    screen.renderer = NULL;
    screen.window = NULL;
    SDL_Quit();
}

int main(int argc, char* args[]) {

    //start the application
    interface screen = init();

    //setup for event handling
    bool quit = false;
    SDL_Event event;

    //the shape to render
    SDL_Rect fillRect = { 100, 100, 200, 200};

    //main loop which first handles events
    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT)
                quit = true;
        }
        //should draw a red rectangle on the screen
        //SDL_SetRenderDrawColor(screen.renderer, 0xff, 0x00, 0x00, 0x00);
        //SDL_RenderClear(screen.renderer);
        SDL_SetRenderDrawColor(screen.renderer, 0xff, 0xff, 0xff, 0xff);
        SDL_RenderFillRect(screen.renderer, &fillRect);
		SDL_RenderDrawLine(screen.renderer, 0, 0, 500, 500);
		SDL_SetRenderDrawColor(screen.renderer, 0xff, 0xff, 0x00, 0x00);
		SDL_RenderDrawLine(screen.renderer, 0, 500, 500, 500);
		SDL_SetRenderDrawColor(screen.renderer, 0xcc, 0xcc, 0xcc, 0xcc);
		SDL_RenderDrawLine(screen.renderer, 0, 500, 500, 500);
		SDL_RenderPresent(screen.renderer);
    	SDL_Delay(10);
    }

    //End the application
    close(screen);
    return 0;
}

