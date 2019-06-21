#include <iostream>
#include "liu_renderer.h"

using namespace sc2;

LiuRenderer::LiuRenderer()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		const char* error = SDL_GetError();
		std::cerr << "SDL_Init failed with error: " << error << std::endl;
		exit(1);
	}
	
	// if there is a second display, show the debug window on it
	int display_num = SDL_GetNumVideoDisplays() - 1;

	SDL_DisplayMode dm;
	if (SDL_GetDesktopDisplayMode(display_num, &dm) != 0) {
		const char* error = SDL_GetError();
		std::cerr << "SDL_GetDesktopDisplayMode with error: " << error << std::endl;
		exit(1);
	}

	SDL_Rect display_bound;
	SDL_GetDisplayBounds(display_num, &display_bound);

	m_window = SDL_CreateWindow("StarCraftII Remote Debug", display_bound.x, display_bound.y, dm.w*3/4, dm.h*3/4, SDL_WINDOW_SHOWN);

	m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
	//? Tries alpha 0
	SDL_SetRenderDrawColor(m_renderer, 0xff, 0xff, 0xff, 0);
	SDL_RenderClear(m_renderer);
}

LiuRenderer::~LiuRenderer()
{
	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}

void sc2::LiuRenderer::DrawObservations(const std::vector<const ObservationInterface*> observations)
{
	//todo get the winodow size and split it into parts cooresponding to the size of observations

	//todo for each observations or sub window, draw it
}

void sc2::LiuRenderer::DrawObservation(const ObservationInterface* observation)
{
	int w, h;
	SDL_SetRenderDrawColor(m_renderer, 0xff, 0xff, 0xff, 0xff);
	SDL_RenderClear(m_renderer);
	SDL_GetWindowSize(m_window, &w, &h);
	DrawObservation(observation, 0, 0, w, h);

	SDL_RenderPresent(m_renderer);
}

void sc2::LiuRenderer::DrawObservation(const ObservationInterface* observation, int offset_x, int offset_y, int w, int h)
{
	//calculate the ratio from original map to draw window
	Point2D playable_min = observation->GetGameInfo().playable_min;
	Vector2D playable_length = { observation->GetGameInfo().playable_max.x - playable_min.x,observation->GetGameInfo().playable_max.y - playable_min.y };
	float ratio = playable_length.x / w > playable_length.y / h ? w / playable_length.x : h / playable_length.y;
	// handle how to draw the unit
	Units us = observation->GetUnits();
	Point2D playable_pos;
	SDL_Rect unit_rect; //? Note that the properties of rect are ints
	for (const Unit* u:us)
	{
		//todo the colors
		if (u->owner == 1) {
			SDL_SetRenderDrawColor(m_renderer, 0, 0xff, 0, 0xff);
		}
		else if (u->owner == 2) {
			SDL_SetRenderDrawColor(m_renderer, 0xff, 0, 0, 0xff);
		}
		else {
			SDL_SetRenderDrawColor(m_renderer, 0, 0, 0xff, 0xff);
		}
		playable_pos = u->pos - playable_min;
		// get the corresponding pos in draw window
		float x = (float)offset_x + playable_pos.x * ratio;
		float y = (float)offset_y + (playable_length.y - playable_pos.y) * ratio;
		float size = u->radius * ratio < 20 ? 20 : u->radius * ratio;
		unit_rect = { (int)x, (int)y, (int)size, (int)size};
		SDL_RenderDrawRect(m_renderer, &unit_rect);
		SDL_RenderFillRect(m_renderer, &unit_rect);
	}
}
