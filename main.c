#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_events.h>

#include "inv_machine.h"

#define SCREEN_WIDTH (224)
#define SCREEN_HEIGHT (256)

static int to_exit = 0;
static machine *mac;

static SDL_Event event;
static SDL_Texture *tex;
static SDL_Renderer *rend;
static SDL_Window *wind;

static int timer = 0; 

void main_loop() {
	while(SDL_PollEvent(&event) != 0) {
		if(event.type == SDL_QUIT) {
			to_exit = 1;
			return;
		} else if(event.type == SDL_KEYDOWN) {
			int key = event.key.keysym.sym;
			switch(key) {
				case SDLK_LEFT:
					machine_key_down(mac, LEFT);
					break;
				case SDLK_RIGHT:
					machine_key_down(mac, RIGHT);
					break;
				case SDLK_SPACE:
					machine_key_down(mac, SHOOT);
					break;
				case SDLK_RETURN:
					machine_key_down(mac, START);
					break;
				case SDLK_c:
					machine_key_down(mac, COIN);
					break;
				case SDLK_t:
					machine_key_down(mac, TILT);
					break;
			}
		} else if(event.type == SDL_KEYUP) {
			int key = event.key.keysym.sym;
			switch(key) {
				case SDLK_LEFT:
					machine_key_up(mac, LEFT);
					break;
				case SDLK_RIGHT:
					machine_key_up(mac, RIGHT);
					break;
				case SDLK_SPACE:
					machine_key_up(mac, SHOOT);
					break;
				case SDLK_RETURN:
					machine_key_up(mac, START);
					break;
				case SDLK_c:
					machine_key_up(mac, COIN);
					break;
				case SDLK_t:
					machine_key_up(mac, TILT);
					break;
			}
		}
	}	

	//run every 1/60 seconds
	if(SDL_GetTicks() - timer > (1000 / 60)) {
		timer = SDL_GetTicks();

		run_cpu(mac);
		load_screen_buffer(mac);

		SDL_UpdateTexture(tex, NULL, mac->screen_buffer, 4 * SCREEN_WIDTH);
	}

	SDL_RenderClear(rend);
	SDL_RenderCopy(rend, tex, NULL, NULL);
	SDL_RenderPresent(rend);
}

int main() {
	
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Error initializing SDL: %s", SDL_GetError());
		exit(1);
	}

	wind = SDL_CreateWindow("Space Invaders emulation by BlueZ",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			SCREEN_WIDTH * 2.5, SCREEN_HEIGHT * 2.5, 0);
	if(!wind) {
		printf("Cannot create window! %s", SDL_GetError());
		exit(1);
	}
	SDL_SetWindowResizable(wind, SDL_TRUE);

	rend = SDL_CreateRenderer(wind, -1, 
			SDL_RENDERER_ACCELERATED |
			SDL_RENDERER_PRESENTVSYNC);
	if(!rend) {
		printf("Cannot create renderer! %s", SDL_GetError());
		SDL_DestroyWindow(wind);
		SDL_Quit();
		exit(1);
	}	

	SDL_RendererInfo r_info;
	SDL_GetRendererInfo(rend, &r_info);
	printf("Using renderer: %s\n", r_info.name);
	
	tex = SDL_CreateTexture(rend,
			SDL_PIXELFORMAT_RGBA32,
			SDL_TEXTUREACCESS_STREAMING,
			SCREEN_WIDTH,
			SCREEN_HEIGHT);
	if(!tex) {
		printf("Cannot create texture! %s", SDL_GetError());
		SDL_DestroyRenderer(rend);
		SDL_DestroyWindow(wind);
		SDL_Quit();
		exit(1);
	}

	
	mac = init_machine();		

	SDL_UpdateTexture(tex, NULL, mac->screen_buffer, 4 * SCREEN_WIDTH);

	timer = SDL_GetTicks();

	while(!to_exit) {
		main_loop();			
	}
	
	free(mac->state->memory);
	free(mac->state);
	free(mac->screen_buffer);
	free(mac);
			
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(wind);
	SDL_Quit();
	return 0;
}
