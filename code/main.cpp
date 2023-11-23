/*
	TODOs:
	* Implement Memory Allocators
	* Implement Dynamic Arrays
	* Fix the collision order, apparently we're supposed to move the first guy with the penetration
	  vector amount. Right now I'm handling it by adding half the distance on both objects, but 
	  that's not good because we can't be moving the grounds and stuff.
	* Add a broad phase and a narrow phase in the collision system
		- Can't afford to call GJK between each pair of existing colliders
		- Use something akin to an AABB (or even quad trees in the future) to get list 
		  of possible collisions and finally test the collisions and resolve if required
	* Tileset editor or maybe just a level editor
		- maybe use bitmasks to make them blend automatically, something akin to cellular 
		  automata by counting the neighbours.
	* Re-think the animation system: Maybe use animation queues?
	  - essentially each animation knows if it is a one shot thing or not, 
	    as well as how much time it has to be played for and possibly if it can be interrupted
	  - so if we want to play an animation, we simply queue it up in the buffer of the enumeration 
	    values on which animation to play next
	  - then while updating the animation, if it is interruptable, we interrupt it, if its a one shot we
	    switch back to the idle animation, and finally if its a loopable animation, we just replay it.
*/

#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <math.h>

// TODO: replace SDL with DirectX
#include <SDL2/SDL.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float r32;
typedef double r64;
typedef ptrdiff_t imem;


[[noreturn]] void fatal_error(const char *message, SDL_Window *window = nullptr) {
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", message, window);
	SDL_Log(message);
	exit(-1);
}

void log_error(const char *message, SDL_Window *window = nullptr) {
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", message, window);
	SDL_Log(message);
}

#include "defer.h"
#include "ren_math.h"
// TODO: Add support for something like Option<T>?
#include "ren_string.h"
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define HexColor(c) ((c) >> (8 * 3)) & 0xff, ((c) >> (8 * 2)) & 0xff, ((c) >> (8 * 1)) & 0xff, ((c) >> (8 * 0)) & 0xff
#define ArrayCount(a) (sizeof(a) / sizeof(*(a)))

//Maybe templatize them?
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Clamp(a, x, b) (Min(Max((a), (x)), (b))


struct Texture {
	i32 width;
	i32 height;
	SDL_Texture *tex;
};

//TODO: Revamp input system
struct Input {
	bool is_down[256];
	bool was_down[256];
	u8 half_transition[256];
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
// TODO: Maybe use hashmaps to not do this?
enum {
	PLAYER_ANIMATION_IDLE,
	PLAYER_ANIMATION_CROUCH,
	PLAYER_ANIMATION_RUN,
	PLAYER_ANIMATION_ATK1,
	PLAYER_ANIMATION_ATK2,
	PLAYER_ANIMATION_ATK3,

	COUNT_PLAYER_ANIMATION
};

enum {
	ENEMY_ANIMATION_IDLE,
	ENEMY_ANIMATION_WALK,
	ENEMY_ANIMATION_ATK,

	COUNT_ENEMY_ANIMATION
};

// TODO: Maybe refactor this? Look into Zero's actual animation frame idea
struct AnimationFrame {
	i32 start_frame_index;
	i32 count;
	i32 texture_index;
};

struct Animation {
	AnimationFrame *frames;
	i32 frame_count;
	i32 width;
	i32 height;
	i32 count_till_update;
	i32 counter;
};

struct Actor {
	V2 pos;
	V2 size;
	V2 vel;
	V2 accn;
	// animation data
	Animation *animation;
	i32 animation_state;
	i32 current_animation_frame;
	i32 combo; // only used for the player
	i32 idle_animation; // default animation to return to after one shot
	bool one_shot;
	bool flipped;
};

enum Action {
	ACTION_NONE,
	ACTION_MOVE_LEFT,
	ACTION_MOVE_RIGHT,
	ACTION_MOVE_UP,
	ACTION_MOVE_DOWN,
	ACTION_ATTACK,
	ACTION_JUMP,
	ACTION_CROUCH,

	COUNT_ACTION,
};

struct InputAction {
	Action action;
	r32 duration;
	bool consumed;
};

/////////////////////////////////////////////////////////
////////////            globals

// TODO: Pool this into an allocator
// TODO: Turn them into array_view as well
Animation animations[256];
i32 animation_count;
AnimationFrame animation_frame_buffer[256] = {};
i32 animation_frame_buffer_count = 0;
Texture textures[64];
i32 texture_count = 0;

InputAction buffer_actions[16];
int buffer_action_size = 0;



// TODO: Make this platform dependent?
String read_entire_file(const char *filename)
{
	SDL_RWops *rwio = SDL_RWFromFile(filename, "rb");
	if (rwio == nullptr) {
		fatal_error(SDL_GetError(), nullptr);
	}
	Sint64 size = rwio->size(rwio);
	u8 *file_content = (u8*) SDL_malloc(sizeof(*file_content) * (size + 1));
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
	i32 w, h;
	String file_content = read_entire_file(filename);

	u8 *data = stbi_load_from_memory(file_content.data, (i32) file_content.len, &w, &h, nullptr, 4);

	SDL_free(file_content.data);

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

//TODO: Better camera system
V2 camera;
V2 resolution;

void display_frame(SDL_Renderer *renderer, Texture *textures,Actor* actor)
{
	Animation *animation = actor->animation;
	// HACK: Simplify this (or even think up a better solution)
	i32 frame_index = actor->current_animation_frame +
		animation->frames[actor->animation_state].start_frame_index;
	i32 index_x = (frame_index) %
		(i32) (textures[animation->frames[actor->animation_state].texture_index].width / animation->width);
	i32 index_y = (frame_index) /
		(i32) (textures[animation->frames[actor->animation_state].texture_index].width / animation->width);

	SDL_Rect src_rect = {};
	src_rect.x = index_x * animation->width;
	src_rect.y = index_y * animation->height;
	src_rect.w = animation->width;
	src_rect.h = animation->height;

	SDL_FRect dest_rect = { actor->pos.x - camera.x + resolution.x / 2.f, actor->pos.y - camera.y + resolution.y / 2.f,  actor->size.x, actor->size.y };

	//SDL_RenderCopyF(renderer, textures[animation->frames[actor->animation_state].texture_index].tex, &src_rect, &dest_rect);
	SDL_RenderCopyExF(renderer, 
					  textures[animation->frames[actor->animation_state].texture_index].tex, 
					  &src_rect, &dest_rect,
					  0, nullptr, 
					  actor->flipped ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

void update_frame(Actor* actor)
{
	Animation *animation = actor->animation;
	if (animation->counter > animation->count_till_update) {
		actor->current_animation_frame = (actor->current_animation_frame + 1) % animation->frames[actor->animation_state].count;
		animation->counter = 0;

		if (actor->current_animation_frame == 0) {
			if (actor->one_shot) {
				actor->animation_state = actor->idle_animation;
				actor->one_shot = false;
			}
		}

	} else {
		animation->counter++;
	}
}

bool expect(String* a, u8 c, const char *error)
{
	if (a->data[0] == c) {
		string_chop_left(a, 1);
		return true;
	}
	fatal_error(error);
}

Animation* parse_animation_file(SDL_Renderer *renderer, const char *file_path)
{
	Animation animation = {};
	animation.frames = animation_frame_buffer + animation_frame_buffer_count;
	String animation_file = read_entire_file(file_path);
	String animation_file_start = animation_file;

	{
		String line = string_chop_by_delim(&animation_file, '\n');
		while (line.len > 0) {
			if (line[0] == '#') {
				String prefix = string_trim(string_chop_by_delim(&line, ' '));
				string_chop_left(&prefix, 1);
				line = string_trim(line);
				if (prefix == String("path:")) {
					if (expect(&line, '"', "Path must start with a \"")) {
						String texture_path = string_chop_by_delim(&line, '"');
						char scratch_buffer[512] = {};	// TODO remove this by making it so that load_texture can work with String as well
						SDL_memcpy(scratch_buffer, texture_path.data, texture_path.len);
						textures[texture_count++] = load_texture(renderer, scratch_buffer);
					}
				} else if (prefix == String("width:")) { animation.width = string_parse_i32(line); }
				else if (prefix == String("height:")) { animation.height = string_parse_i32(line); }
				else if (prefix == String("count:")) { animation.count_till_update = string_parse_i32(line); }
				else {
					fatal_error("Unexpected metadata", nullptr);
				}
			} else if (line[0] != '\r') {
				string_chop_by_delim(&line, ':');
				line = string_trim(line);
				animation.frames[animation.frame_count].start_frame_index = string_parse_i32(line);
				string_chop_by_delim(&line, ' ');
				line = string_trim(line);
				animation.frames[animation.frame_count].count = string_parse_i32(line);
				// in case one shot: true is required in the animation file
				/*string_chop_by_delim(&line, ' ');
				line = string_trim(line);
				animation.frames[animation.frame_count].one_shot = line == "true";*/

				animation.frames[animation.frame_count].texture_index = texture_count - 1;
				animation.frame_count++;
				animation_frame_buffer_count++;
			}
			line = string_chop_by_delim(&animation_file, '\n');
		}
	}
	SDL_free(animation_file_start.data);

	animations[animation_count] = animation;
	return &animations[animation_count++];
}

// TODO: YEET
void draw_ring(SDL_Renderer *renderer, Circle circle, Uint32 color)
{
	SDL_SetRenderDrawColor(renderer, HexColor(color));

	r32 offsetx, offsety, d;

	offsetx = 0;
	offsety = circle.radius;
	d = circle.radius - 1;
	r32 x = circle.pos.x;
	r32 y = circle.pos.y;

	while (offsety >= offsetx) {
		SDL_RenderDrawPointF(renderer, x + offsetx, y + offsety);
		SDL_RenderDrawPointF(renderer, x + offsety, y + offsetx);
		SDL_RenderDrawPointF(renderer, x - offsetx, y + offsety);
		SDL_RenderDrawPointF(renderer, x - offsety, y + offsetx);
		SDL_RenderDrawPointF(renderer, x + offsetx, y - offsety);
		SDL_RenderDrawPointF(renderer, x + offsety, y - offsetx);
		SDL_RenderDrawPointF(renderer, x - offsetx, y - offsety);
		SDL_RenderDrawPointF(renderer, x - offsety, y - offsetx);

		if (d >= 2 * offsetx) {
			d -= 2 * offsetx + 1;
			offsetx += 1;
		} else if (d < 2 * (circle.radius - offsety)) {
			d += 2 * offsety - 1;
			offsety -= 1;
		} else {
			d += 2 * (offsety - offsetx - 1);
			offsety -= 1;
			offsetx += 1;
		}
	}
}

void make_polygon(Polygon *p, i32 n, r32 r, r32 offset_angle = 0.f) {
	assert(n <= MAX_POINTS);
	p->size = n;
	for (i32 i = 0; i < n; ++i) {
		p->points[i] = V2(r * cosf(offset_angle + 2 * PI32 * i / (r32) n), r * sinf(offset_angle + 2 * PI32 * i / (r32) n));
	}
}

void draw_polygon(SDL_Renderer *renderer, Polygon *p, Uint32 color) {
	SDL_SetRenderDrawColor(renderer, HexColor(color));
	for (i32 i = 0; i < p->size; ++i) {
		V2 p1 = p->pos + p->points[i];
		V2 p2 = p->pos + p->points[(i + 1) % p->size];
		SDL_RenderDrawLineF(renderer, p1.x - camera.x + resolution.x / 2.f, p1.y - camera.y + resolution.y / 2.f, p2.x - camera.x + resolution.x / 2.f, p2.y - camera.y + resolution.y / 2.f);
	}
}

void draw_capsule(SDL_Renderer *renderer, Capsule c, Uint32 color)
{
	SDL_SetRenderDrawColor(renderer, HexColor(color));
	draw_ring(renderer, {c.a - camera + resolution / 2.f, c.radius}, color);
	V2 ab = c.b - c.a;
	V2 norm = normalizez(V2(-ab.y, ab.x));
	V2 p1 = c.a + c.radius * norm;
	V2 p2 = c.b + c.radius * norm;
	SDL_RenderDrawLineF(renderer, p1.x - camera.x + resolution.x / 2.f, p1.y - camera.y + resolution.y / 2.f, p2.x - camera.x + resolution.x / 2.f, p2.y - camera.y + resolution.y / 2.f);
	p1 = c.a - c.radius * norm;
	p2 = c.b - c.radius * norm;
	SDL_RenderDrawLineF(renderer, p1.x - camera.x + resolution.x / 2.f, p1.y - camera.y + resolution.y / 2.f, p2.x - camera.x + resolution.x / 2.f, p2.y - camera.y + resolution.y / 2.f);
	draw_ring(renderer, {c.b - camera + resolution / 2.f, c.radius}, color);
}

void refresh_buffer(InputAction* buffer, int* size) 
{
	InputAction buffer_buffer[ArrayCount(buffer_actions)] = {};
	int buffer_buffer_count = 0;

	for (int i = 0; i < *size; ++i) {
		if (buffer[i].duration > 0) {
			buffer_buffer[buffer_buffer_count++] = buffer[i];
		}
	}
	for (int i = 0; i < buffer_buffer_count; ++i) {
		buffer[i] = buffer_buffer[i];
	}

	*size = buffer_buffer_count;
	
	memset(buffer, 0, sizeof(buffer_actions)); // TODO: fix this with an array type
	memcpy(buffer, buffer_buffer, buffer_buffer_count * sizeof(*buffer));
}

i32 main(i32 argc, char **argv)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fatal_error(SDL_GetError());
	}

	resolution = V2(1280, 720);
	SDL_Window *window = SDL_CreateWindow("Untitled-Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (i32) resolution.x, (i32) resolution.y,
										  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
	// NOTE: Apparently using SDL_RENDERER_ACCELERATED is bad because it forces us to create a hardware
	// renderer, and just crash if it can't instead of falling back to a software renderer.

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND) < 0) {
		fatal_error(SDL_GetError(), nullptr);
	}
	SDL_ShowWindow(window);

	bool is_running = true;
	u64 last_ms = SDL_GetTicks64();
	u64 start_ms = SDL_GetTicks64();

	Input input = {};
	Actor player = {};
	Actor enemy = {};

	player.animation = parse_animation_file(renderer, "./data/player.anims");
	player.idle_animation = PLAYER_ANIMATION_IDLE;
	player.size = { 3.f * player.animation->width, 3.f * player.animation->height };

	enemy.animation = parse_animation_file(renderer, "./data/enemy.anims");
	enemy.idle_animation = ENEMY_ANIMATION_IDLE;
	enemy.size = { 2.f * enemy.animation->width, 2.f * enemy.animation->height };

	enemy.pos = (resolution - enemy.size) / 2;

	r32 t = 0;
	r32 dt = 0.01f;

	u64 last_counter = SDL_GetPerformanceCounter();
	const u64 query_perf_freq = SDL_GetPerformanceFrequency();
	r32 accumulator = dt;

	Polygon poly;
	make_polygon(&poly, MAX_POINTS, 50);
	//poly.pos = V2(mouse.x, mouse.y);
	poly.pos = resolution * 0.25f;

	bool left_button_is_down = false;
	bool left_button_was_down = false;
	r32 frame_time = 1/60.f;

	r32 total_frame_time = 0;

	while (is_running) {

		SDL_memcpy(input.was_down, input.is_down, sizeof(input.is_down));
		// memset(input.is_down, 0, sizeof(input.is_down));
		SDL_memset(input.half_transition, 0, sizeof(input.half_transition));

		left_button_was_down = left_button_is_down;

		total_frame_time += frame_time;

		V2 mouse;
		{
			i32 mouse_x, mouse_y;

			//left_button_clicked = SDL_GetMouseState(&mouse_x, &mouse_y) & SDL_BUTTON_LMASK;
			SDL_GetMouseState(&mouse_x, &mouse_y);
			mouse = { (r32) mouse_x, (r32) mouse_y };
		}

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
				{
					is_running = false;
				} break;

				case SDL_KEYDOWN:
				{
					if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
						is_running = false;

					input.is_down[event.key.keysym.scancode] = true;
					input.half_transition[event.key.keysym.scancode]++;
				} break;

				case SDL_MOUSEBUTTONDOWN: {
					left_button_is_down = true;
				} break;

				case SDL_MOUSEBUTTONUP: {
					left_button_is_down = false;
				} break;

				case SDL_KEYUP:
				{
					input.is_down[event.key.keysym.scancode] = false;
				} break;
			}
		}

		// TODO: Check the half transitions here

		bool left_button_clicked = left_button_is_down && !left_button_was_down;

		if (is_held(&input, SDL_SCANCODE_W)) {
			buffer_actions[buffer_action_size++] = { .action = ACTION_MOVE_UP,};
		}
		if (is_held(&input, SDL_SCANCODE_A)) {
			buffer_actions[buffer_action_size++] = { .action = ACTION_MOVE_LEFT,};
		}
		if (is_held(&input, SDL_SCANCODE_S)) {
			buffer_actions[buffer_action_size++] = { .action = ACTION_MOVE_DOWN,};
		}
		if (is_held(&input, SDL_SCANCODE_D)) {
			buffer_actions[buffer_action_size++] = { .action = ACTION_MOVE_RIGHT,};
		}
		if (is_pressed(&input, SDL_SCANCODE_E) || left_button_clicked) {
			buffer_actions[buffer_action_size++] = { .action = ACTION_ATTACK, .duration = .9f, };
		}
		if (is_pressed(&input, SDL_SCANCODE_SPACE)) {
			buffer_actions[buffer_action_size++] = { .action = ACTION_JUMP,};
		}
		if (is_held(&input, SDL_SCANCODE_LCTRL)) {
			buffer_actions[buffer_action_size++] = { .action = ACTION_CROUCH,};
		}


		player.accn = {};

		// apply all buffer actions here
		{
			if (buffer_action_size >= ArrayCount(buffer_actions)) {
				__debugbreak();
			}

			bool attack_encountered = false;
			static int counter = 0;

			for (int i = 0; i < buffer_action_size; ++i) {
				if (!attack_encountered && buffer_actions[i].action == ACTION_ATTACK)
					buffer_actions[i].duration -= frame_time;
				switch (buffer_actions[i].action) {
					case ACTION_NONE: {
						// no op
						player.animation_state = PLAYER_ANIMATION_IDLE;
						player.one_shot = false;
					} break;
					case ACTION_MOVE_LEFT: {
						player.accn.x -= 1;
						player.animation_state = PLAYER_ANIMATION_RUN;
						player.flipped = true;
						player.one_shot = false;
					} break;

					case ACTION_MOVE_RIGHT: {
						player.accn.x += 1;
						player.animation_state = PLAYER_ANIMATION_RUN;
						player.flipped = false;
						player.one_shot = false;
					} break;

					case ACTION_MOVE_UP: {
						player.accn.y -= 1;
					} break;

					case ACTION_MOVE_DOWN: {
						player.accn.y += 1;
					} break;

					case ACTION_ATTACK: {
						if (buffer_actions[i].duration > 0 && !attack_encountered && !buffer_actions[i].consumed) {
							buffer_actions[i].consumed = true;
							player.animation_state = PLAYER_ANIMATION_ATK1 + player.combo;
							player.current_animation_frame = 0;
							player.combo = (player.combo + 1) % 3; // TODO: un-hardcode this
							player.one_shot = true;
							char buff[32] = {};
							SDL_snprintf(buff, sizeof(buff), "Attack %d", counter++);
							SDL_SetWindowTitle(window, buff);

						}
						attack_encountered = true;
					} break;

					case ACTION_JUMP: {

					} break;

					case ACTION_CROUCH: {
						player.animation_state = PLAYER_ANIMATION_CROUCH;
					} break;
				}
			}

			if (buffer_action_size == 0) {
				player.animation_state = PLAYER_ANIMATION_IDLE;
			}

			if (!attack_encountered) {
				player.combo = 0;
				counter = 0;
				SDL_SetWindowTitle(window, "Attacks Ended");
			}

		}

		refresh_buffer(buffer_actions, &buffer_action_size);

		enemy.accn = {};
		enemy.accn.y -= is_held(&input, SDL_SCANCODE_UP);
		enemy.accn.x -= is_held(&input, SDL_SCANCODE_LEFT);
		enemy.accn.x += is_held(&input, SDL_SCANCODE_RIGHT);
		enemy.accn.y += is_held(&input, SDL_SCANCODE_DOWN);

		//if (is_pressed(&input, SDL_SCANCODE_SPACE)) {
		//	// player.animation_state = (player.animation_state + 1) % PLAYER_ANIMATION_COUNT;
		//	enemy.animation_state = (enemy.animation_state + 1) % COUNT_ENEMY_ANIMATION;
		//}

		player.accn = normalizez(player.accn);
		enemy.accn = normalizez(enemy.accn);

		r32 speed = 250.f;

		//if (player.accn.x != 0 && player.animation_state == PLAYER_ANIMATION_RUN)
		//		player.animation_state = PLAYER_ANIMATION_IDLE;

		//if (is_held(&input, SDL_SCANCODE_LCTRL)) {
		//} else {
		//	if (player.animation_state == PLAYER_ANIMATION_CROUCH)
		//		player.animation_state = PLAYER_ANIMATION_IDLE;
		//}

		// TODO: generate sword collider and make others get damaged if appropriate

		Rect r_player = { player.pos, player.pos + player.size };
		Rect r_enemy = { enemy.pos + enemy.size / 4.f , enemy.pos + enemy.size * 3.f / 4.f };
		Capsule c_player;
		c_player.radius = player.size.x / 5.f;
		c_player.a = player.pos + V2(1, 0.75) * player.size * 0.5f;
		c_player.b = c_player.a + V2(0, 1) * player.size * 0.4f;
		//Circle c_player = { player.pos + player.size / 2.f, player.size.y / 2.f };
		Uint32 collision_color = 0xff0000ff;

		while (accumulator >= dt) {
			// call into physics

			player.pos += speed * player.accn * dt;
			// forgot to update the player capsule after moving woops
			c_player.a = player.pos + V2(1, 0.75) * player.size * 0.5f;
			c_player.b = c_player.a + V2(0, 1) * player.size * 0.4f;
			r_player = { player.pos, player.pos + player.size };

			// forgot to update the enemy rect after moving woops
			enemy.pos += speed * enemy.accn * dt;
			r_enemy = { enemy.pos + enemy.size / 4.f , enemy.pos + enemy.size * 3.f / 4.f };

			{
				V2 dist;

				if (epa(c_player, r_enemy, dist)) {
					player.pos -= dist / 2;
					enemy.pos += dist / 2;
					c_player.a = player.pos + V2(1, 0.75) * player.size * 0.5f;
					c_player.b = c_player.a + V2(0, 1) * player.size * 0.4f;
					r_enemy = { enemy.pos + enemy.size / 4.f , enemy.pos + enemy.size * 3.f / 4.f };
					collision_color = 0xffffffff;
				}
			}
			{
				V2 dist;
				if (epa(poly, r_enemy, dist)) {
					poly.pos -= dist;	// for the polygon, just updating its position works
					collision_color = 0xff00ffff;
				}
			}
			{
				V2 dist;
				if (epa(c_player, poly, dist)) {
					player.pos -= dist;	// same here
					c_player.a = player.pos + V2(1, 0.75) * player.size * 0.5f;
					c_player.b = c_player.a + V2(0, 1) * player.size * 0.4f;
					collision_color = 0x00ffffff;
				}
			}

			camera = lerp(camera, 0.025f, player.pos);

			update_frame(&player);
			update_frame(&enemy);


			t += dt;
			accumulator -= dt;
		}

		SDL_SetRenderDrawColor(renderer, HexColor(0x181818ff));
		SDL_RenderClear(renderer);

		display_frame(renderer, textures, &player);

		display_frame(renderer, textures, &enemy);

		auto rect_to_sdl_rect = [] (Rect a) -> SDL_FRect {
			return { a.min.x, a.min.y, (a.max - a.min).x, (a.max - a.min).y };
		};

		SDL_SetRenderDrawColor(renderer, HexColor(collision_color));

		SDL_FRect f_enemy = rect_to_sdl_rect(r_enemy);
		f_enemy.x = f_enemy.x - camera.x + resolution.x / 2.f;
		f_enemy.y = f_enemy.y - camera.y + resolution.y / 2.f;
		SDL_RenderDrawRectF(renderer, &f_enemy);

		draw_capsule(renderer, c_player, collision_color);

		draw_polygon(renderer, &poly, collision_color);

		SDL_RenderPresent(renderer);


		u64 current_counter = SDL_GetPerformanceCounter();
		frame_time = ((1000000.0f * (current_counter - last_counter)) / (r32) query_perf_freq) / 1000000.0f;
		last_counter = current_counter;

		/*{
			char buff[32] = {};
			SDL_snprintf(buff, sizeof(buff), "%f", 1.f / frame_time);
			SDL_SetWindowTitle(window, buff);
		}*/
		// TODO: look into this
		// camera = damp(camera, 0.025f, frame_time, player.pos);

		accumulator += frame_time;
	}

	return 0;
}