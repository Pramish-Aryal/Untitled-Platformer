#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <SDL2/SDL.h>
#include "defer.h"
#include "ren_math.h"
#include "ren_string.h"
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define HexColor(c) ((c) >> (8 * 3)) & 0xff, ((c) >> (8 * 2)) & 0xff, ((c) >> (8 * 1)) & 0xff, ((c) >> (8 * 0)) & 0xff
#define ArrayCount(a) (sizeof(a) / sizeof(*(a)))
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Clamp(a, x, b) (Min(Max((a), (x)), (b))

[[noreturn]] void fatal_error(const char *message, SDL_Window *window = nullptr) {
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", message, window);
	exit(-1);
}

struct Texture {
	int width;
	int height;
	SDL_Texture *tex;
};

struct Input {
	bool is_down[256];
	bool was_down[256];
	uint8_t half_transition[256];
};

inline bool is_pressed(Input *input, SDL_Scancode key) {
	return input->is_down[key] && !input->was_down[key];
}

inline bool is_held(Input *input, SDL_Scancode key) {
	return input->is_down[key];
}

inline bool is_released(Input *input, SDL_Scancode key) {
	return !input->is_down[key] && input->was_down[key];
}

// NOTE: Animations need to be defined in order as they appear in the animation file
enum PlayerAnimationState {
	PLAYER_ANIMATION_IDLE,
	PLAYER_ANIMATION_CROUCH,
	PLAYER_ANIMATION_RUN,
	PLAYER_ANIMATION_ATK1,
	PLAYER_ANIMATION_ATK2,
	PLAYER_ANIMATION_ATK3,

	PLAYER_ANIMATION_COUNT
};

enum EnemyAnimationState {
	ENEMY_ANIMATION_IDLE,
	ENEMY_ANIMATION_WALK,
	ENEMY_ANIMATION_ATK,

	ENEMY_ANIMATION_COUNT
};

// TODO: Maybe refactor this? Look into Zero's actual animation frame idea
struct AnimationFrame {
	int start_frame_index;
	int count;
	int texture_index;
	int current_frame_index = 0;
};

struct Animation {
	AnimationFrame *frames;
	int frame_count;
	int width;
	int height;
	int count_till_update;
	int counter;
};

struct Actor {
	V2 pos;
	V2 size;
	V2 accn;
	int animation_index;
	int animation_state;
};

// TODO: Make this platform dependent?
String read_entire_file(const char *filename)
{
	SDL_RWops *rwio = SDL_RWFromFile(filename, "rb");
	if (rwio == nullptr) {
		fatal_error(SDL_GetError(), nullptr);
	}
	Sint64 size = rwio->size(rwio);
	uint8_t *file_content = new uint8_t[size + 1];
	SDL_ClearError();
	size_t n = rwio->read(rwio, file_content, size, 1);
	const char *error = SDL_GetError();
	if (*error != 0 && n == 0) {
		fatal_error(error, nullptr);
	}
	rwio->close(rwio);
	file_content[size] = 0;
	String result(file_content, size);
	return result;
}

Texture load_texture(SDL_Renderer *renderer, const char *filename)
{
	Texture result = {};
	int w, h;
	String file_content = read_entire_file(filename);

	uint8_t *data = stbi_load_from_memory(file_content.data, (int) file_content.len, &w, &h, nullptr, 4);

	delete[] file_content.data;

	if (data == nullptr) {
		fatal_error(stbi_failure_reason(), nullptr);
	}
	result.tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);

	result.width = w;
	result.height = h;
	if (SDL_UpdateTexture(result.tex, nullptr, data, w * 4) < 0) {
		fatal_error(SDL_GetError(), nullptr);
	}

	SDL_SetTextureBlendMode(result.tex, SDL_BLENDMODE_BLEND);
	stbi_image_free(data);
	return result;
}

void display_frame(SDL_Renderer *renderer, Texture *textures, Animation *animations, Actor actor, V2 scale = {1.f, 1.f})
{
	// HACK: Simplify this (or even think up a better solution)
	int frame_index = animations[actor.animation_index].frames[actor.animation_state].current_frame_index +
		animations[actor.animation_index].frames[actor.animation_state].start_frame_index;
	int index_x = (frame_index) %
		(int) (textures[animations[actor.animation_index].frames[actor.animation_state].texture_index].width / animations[actor.animation_index].width);
	int index_y = (frame_index) /
		(int) (textures[animations[actor.animation_index].frames[actor.animation_state].texture_index].width / animations[actor.animation_index].width);

	SDL_Rect src_rect = {};
	src_rect.x = index_x * animations[actor.animation_index].width;
	src_rect.y = index_y * animations[actor.animation_index].height;
	src_rect.w = animations[actor.animation_index].width;
	src_rect.h = animations[actor.animation_index].height;

	SDL_FRect dest_rect = { actor.pos.x, actor.pos.y,  src_rect.w * scale.x, src_rect.h * scale.y };

	SDL_RenderCopyF(renderer, textures[animations[actor.animation_index].frames[actor.animation_state].texture_index].tex, &src_rect, &dest_rect);
}

void update_frame(Animation* animations, Actor actor)
{
	if (animations[actor.animation_index].counter > animations[actor.animation_index].count_till_update) {
		animations[actor.animation_index].frames[actor.animation_state].current_frame_index =
			(animations[actor.animation_index].frames[actor.animation_state].current_frame_index + 1) %
			animations[actor.animation_index].frames[actor.animation_state].count;
		animations[actor.animation_index].counter = 0;
	} else {
		animations[actor.animation_index].counter++;
	}
}

// TODO: Pool this into an allocator
Animation animations[256];
int animation_count;
AnimationFrame animation_frame_buffer[256] = {};
int animation_frame_buffer_count = 0;
Texture textures[64];
int texture_count = 0;

// TODO: refactor out sscanf_s, strtok with better self implemented String functions, co-routines?
int parse_animation_file(SDL_Renderer *renderer, const char *file_path)
{
	Animation animation = {};
	animation.frames = animation_frame_buffer + animation_frame_buffer_count;
	String animation_file = read_entire_file(file_path);
	char scratch_buffer[512];
	{
		char *texture_path = 0;
		char *start = (char *) animation_file.data;
		char *next = strtok(start, "\r\n");
		while (next) {
			if (*next == '#') {
				next++;
				if (sscanf_s(next, "path: %s", scratch_buffer, (uint32_t) sizeof(scratch_buffer)) == 1) {
					texture_path = scratch_buffer + 1;
					texture_path[strlen(texture_path) - 1] = 0;
					textures[texture_count++] = load_texture(renderer, texture_path);
				} else if (sscanf_s(next, "width: %d", &animation.width) == 1);
				else if (sscanf_s(next, "height: %d", &animation.height) == 1);
				else if (sscanf_s(next, "count: %d", &animation.count_till_update) == 1);
				else {
					fatal_error("Unexpected metadata", nullptr);
				}
			} else {
				sscanf_s(next, "%s %d %d", scratch_buffer, (uint32_t) sizeof(scratch_buffer),
						 &animation.frames[animation.frame_count].start_frame_index,
						 &animation.frames[animation.frame_count].count);
				animation.frames[animation.frame_count].texture_index = texture_count - 1;
				animation.frame_count++;
				animation_frame_buffer_count++;
			}

			next = strtok(nullptr, "\r\n");
		}
	}
	animations[animation_count] = animation;
	return animation_count++;
}

int main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fatal_error(SDL_GetError());
	}

	SDL_Window *window = SDL_CreateWindow("Untitled-Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720,
										  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND) < 0) {
		fatal_error(SDL_GetError(), nullptr);
	}
	SDL_ShowWindow(window);

	bool is_running = true;
	uint64_t last_ms = SDL_GetTicks64();
	uint64_t start_ms = SDL_GetTicks64();

	PlayerAnimationState player_anim_state = PLAYER_ANIMATION_IDLE;

	Input input = {};
	Actor player = {};
	Actor enemy = {};

	player.animation_index = parse_animation_file(renderer, u8"./data/player.anims");
	enemy.animation_index = parse_animation_file(renderer, u8"./data/enemy.anims");

	float t = 0;
	float dt = 0.01f;

	uint64_t last_counter = SDL_GetPerformanceCounter();
	const uint64_t query_perf_freq = SDL_GetPerformanceFrequency();
	float accumulator = dt;
	uint64_t frame_counter = 0;
	while (is_running) {

		memcpy(input.was_down, input.is_down, sizeof(input.is_down));
		// memset(input.is_down, 0, sizeof(input.is_down));
		memset(input.half_transition, 0, sizeof(input.half_transition));

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT: {
					is_running = false;
				} break;

				case SDL_KEYDOWN: {
					if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
						is_running = false;

					input.is_down[event.key.keysym.scancode] = true;
					input.half_transition[event.key.keysym.scancode]++;
				} break;

				case SDL_KEYUP: {
					input.is_down[event.key.keysym.scancode] = false;
				} break;
			}
		}

		// TODO: Check the half transitions here
		player.accn = {};
		player.accn.x -= is_held(&input, SDL_SCANCODE_A); // input.half_transition[SDL_SCANCODE_A] > 0;
		player.accn.x += is_held(&input, SDL_SCANCODE_D); // input.half_transition[SDL_SCANCODE_D] > 0;
		player.accn.y -= is_held(&input, SDL_SCANCODE_W); // input.half_transition[SDL_SCANCODE_W] > 0;
		player.accn.y += is_held(&input, SDL_SCANCODE_S); // input.half_transition[SDL_SCANCODE_S] > 0;

		if (is_pressed(&input, SDL_SCANCODE_SPACE)) {
			player.animation_state = (player.animation_state + 1) % PLAYER_ANIMATION_COUNT;
			enemy.animation_state = (enemy.animation_state + 1) % ENEMY_ANIMATION_COUNT;
		}

		normalizez(player.accn);

		float scale = 100.f;

		while (accumulator >= dt) {
			// call into physics
			player.pos += scale * player.accn * dt;
			t += dt;
			accumulator -= dt;
		}

		char buff[64];
		SDL_snprintf(buff, sizeof(buff), "%f %f -> %f %f", player.pos.x, player.pos.y, player.accn.x, player.accn.y);
		SDL_SetWindowTitle(window, buff);

		SDL_SetRenderDrawColor(renderer, HexColor(0x181818ff));
		SDL_RenderClear(renderer);
		
		display_frame(renderer, textures, animations, player, {3, 3});

		display_frame(renderer, textures, animations, enemy, {2, 2});

		SDL_RenderPresent(renderer);

		update_frame(animations, player);
		update_frame(animations, enemy);

		uint64_t current_counter = SDL_GetPerformanceCounter();
		float frameTime = ((1000000.0f * (current_counter - last_counter)) / (float) query_perf_freq) / 1000000.0f;
		last_counter = current_counter;

		frame_counter++;
		accumulator += frameTime;
	}

	return 0;
}
