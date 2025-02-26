#include "iostream"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>

using namespace std; 

SDL_Texture* loadTexture(const char* file, SDL_Renderer *renderer)
{
  SDL_Texture *texture = IMG_LoadTexture(renderer, file);
  printf("%s\n", file);
  if(texture == nullptr) {
	cout << "SDL texture error." << "\n";
  }

  else printf("SDL TEXTURE INIT SUCCESSFUL\n");
  return texture;
}

int main( int argc, char* argv[] )
{
	printf("Show power off starting...\n");
	const char *file_name;
	Uint32 start_time = 0, quit = 0;
	if ( argc < 2 ) {
		printf("need to specify a image file.\n");
		return -1;
	} 	

	file_name = argv[1];

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
                return 1;
        } // if

	int flags = IMG_INIT_JPG | IMG_INIT_PNG;
	int initted = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
	if((flags & initted) != flags){
		SDL_Log("IMG_Init: Failed to init required jpg and png support!\n");
                SDL_Log("IMG_Init: %s\n", IMG_GetError());
	} 
	
	else printf("SDL IMG INIT SUCCESSFUL\n");

	// the following parameter is:
	// Title, X, Y, Width, Height, flags
	SDL_Window* window = SDL_CreateWindow("SDL2 Display Image",  SDL_WINDOWPOS_CENTERED,  SDL_WINDOWPOS_CENTERED, 1280, 1024, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);

	if ( window == nullptr ) {
		SDL_Quit();
		return 1;
	}
	
	else printf("SDL WINDOW INIT SUCCESSFUL\n"); 	

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	if( renderer == nullptr ) {
		SDL_DestroyWindow(window);
		return 1;
	}
	
	else printf("SDL RENDERER INIT SUCCESSFUL\n");

	SDL_Texture *logo = loadTexture(file_name, renderer);
	if(logo == nullptr) {
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		IMG_Quit();
		SDL_Quit();
		return 1;
	}

	SDL_Rect dst;
  	dst.x = 0;
  	dst.y = 0;
  	dst.w = 1280;
  	dst.h = 1024;

	start_time = SDL_GetTicks();
	while(!quit) {
		SDL_RenderCopy(renderer, logo, NULL, &dst);
		SDL_RenderPresent(renderer);
		printf("Present IMAGE...\n");
		SDL_Delay(33);

		if ( ((SDL_GetTicks() - start_time) / 1000.0) > 3 ){
			quit = 1;
			printf("SHOW IMAGE EXCEED 3 second...\n");
		} // if
	} // while

	SDL_DestroyTexture(logo);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();
	printf("Show power off ending...\n");
	return 0;
}
