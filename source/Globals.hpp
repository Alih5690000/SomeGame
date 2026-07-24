#pragma once
#include <SDL2/SDL.h>
#include <vector>

float hitStopTime=0.f;
float gravity=400.f;
float dt=0.f;
float spawncd1=3.f;
int start,end;
SDL_Window* window = nullptr;
bool running = true;

SDL_Renderer* renderer = nullptr;