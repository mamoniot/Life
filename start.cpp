#include <stdio.h>
#include "SDL.h"
#include "basic.h"
#include "math.h"
#include "assert.h"
#include "random.hh"
#undef main


struct Vector {
	int32 x;
	int32 y;
};
struct Dim {
	uint32 width;
	uint32 height;
};

#define BUTTONS_TOTAL 5
enum InputType {
	INPUT_NULL = 0,
	M1 = 1,
	M2 = 2,
	M3 = 3,
	SPACE = BUTTONS_TOTAL,
};
struct GameInput {
	uint32 button_presses[8];
	bool is_down[8];
	Vector mouse_move_plus_one;
	Dim window_resize;
	Dim bitmap_resize;
};
struct PlatformData {
	Dim screen;
	Dim bitmap;
	Vector mouse;
	bool button_is_down[5];
};
struct RenderData {
	uint32* bitmap;
	uint32 bitmap_pitch;
};
struct UserData {
	bool is_dragging;
	Vector last_cell_in_drag;
};
struct GameState {
	PlatformData platform;
	UserData user;
	uint32 steps;
	Dim cells;
	bool is_first_cells_active;
	bool run_simulation;
	PCG rng;
};


inline bool get_cell(const bool* cells, uint32 cells_width, Vector pos) {
	auto cell = &cells[cells_width*pos.y + pos.x];
	return *cell;
}
inline void set_cell(bool* cells, uint32 cells_width, Vector pos, bool state) {
	auto cell = &cells[cells_width*pos.y + pos.x];
	*cell = (state);
}
inline bool get_cell(const bool* cells, uint32 cells_width, int32 x, int32 y) {
	Vector v = {x, y};
	return get_cell(cells, cells_width, v);
}
inline void set_cell(bool* cells, uint32 cells_width, int32 x, int32 y, bool state) {
	Vector v = {x, y};
	set_cell(cells, cells_width, v, state);
}

inline Vector convert_coord(Dim dest, Dim origin, Vector v) {
	Vector w = {cast(int32, v.x*dest.width/origin.width), cast(int32, v.y*dest.height/origin.height)};
	return w;
}

inline Vector lerp(Vector v0, Vector v1, float t) {
	Vector ret;
	ret.x = v0.x + round(t*(v1.x - v0.x));
	ret.y = v0.y + round(t*(v1.y - v0.y));
	return ret;
}

void initialize_game(byte* game_memory, const PlatformData* platform) {
	GameState* game_state = claim_bytes(GameState, &game_memory, 1);
	uint32 cells_width = platform->bitmap.width;
	uint32 cells_height = platform->bitmap.height;
	uint32 cells_size = cells_height*cells_width;
	assert(cells_width > 3 and cells_height > 3);

	bool* cells = claim_bytes(bool, &game_memory, cells_size);
	bool* new_cells = claim_bytes(bool, &game_memory, cells_size);

	memzero(game_state, sizeof(GameState));
	game_state->platform = *platform;
	// game_state->user.last_cell_in_drag.x = 0
	// game_state->steps = 0;
	// game_state->steps = 0;
	game_state->cells.width = cells_width;
	game_state->cells.height = cells_height;
	game_state->is_first_cells_active = 1;
	game_state->run_simulation = 1;
	pcg_seed(&game_state->rng, 12);

	for_each_in(cell, cells, cells_size) {
		*cell = (pcg_random_uniform(&game_state->rng) < .1);
	}
	memzero(new_cells, cells_size);
}

void render_from_cells(uint32* pixels, bool* cells, Dim cells_dim, Dim pixels_dim) {
	for(uint32 row = 0; row < cells_dim.width*cells_dim.height; row += cells_dim.width) {
		for_each_lt(x, cells_dim.width) {
			bool cur_cell = cells[row + x];
			pixels[row + x] = cur_cell ? 0xFFFFFF : 0x111111;
		}
	}
}
RenderData* update_game(byte* game_memory, byte* trans_memory, GameInput input) {
	byte* game_memory_start = game_memory;
	GameState* game_state = claim_bytes(GameState, &game_memory, 1);

	auto steps = game_state->steps;
	auto cells = game_state->cells;
	auto cells_size = cells.height*cells.width;

	bool* cells0 = claim_bytes(bool, &game_memory, cells_size);
	bool* cells1 = claim_bytes(bool, &game_memory, cells_size);
	uint32* pixels = claim_bytes(uint32, &game_memory, cells_size);

	RenderData* ret = claim_bytes(RenderData, &trans_memory, 1);
	ret->bitmap = pixels;
	ret->bitmap_pitch = 4*cells.width;

	if(!game_state->is_first_cells_active) {
		swap(&cells0, &cells1);
	}

	bool do_render_update = false;
	if(input.window_resize.width > 0) {
		Dim screen = input.window_resize;
		Dim new_bitmap = input.bitmap_resize;
		Dim new_cells = new_bitmap;
		auto new_cells_size = screen.height*screen.width;
		// printf("%d, %d, %d, %d\n", screen.width, screen.height, new_bitmap.width, new_bitmap.height);

		byte* new_game_memory = game_memory_start;
		claim_bytes(GameState, &new_game_memory, 1);
		bool* new_cells0 = claim_bytes(bool, &new_game_memory, new_cells_size);
		bool* new_cells1 = claim_bytes(bool, &new_game_memory, new_cells_size);
		uint32* new_pixels = claim_bytes(uint32, &new_game_memory, new_cells_size);
		if(!game_state->is_first_cells_active) {
			swap(&new_cells0, &new_cells1);
		}

		// memzero(trans_memory, new_cells_size);
		for_each_in(cell, trans_memory, new_cells_size) {
			*cell = (pcg_random_uniform(&game_state->rng) < .1);
		}
		for_each_lt(row, min(cells.height, new_cells.height)) {
			memcpy(&trans_memory[new_cells.width*row], &cells0[cells.width*row], min(new_cells.width, cells.width));
		}
		memcpy(new_cells0, trans_memory, new_cells_size);
		memzero(new_cells1, new_cells_size);
		cells = new_cells;
		cells_size = new_cells_size;
		cells0 = new_cells0;
		cells1 = new_cells1;
		pixels = new_pixels;
		game_state->cells = cells;
		game_state->platform.bitmap = new_bitmap;
		game_state->platform.screen = screen;
		ret->bitmap = pixels;
		ret->bitmap_pitch = 4*cells.width;
		do_render_update = true;
	}
	if(input.mouse_move_plus_one.x > 0) {
		Vector new_mouse = {input.mouse_move_plus_one.x - 1, input.mouse_move_plus_one.y - 1};
		game_state->platform.mouse = new_mouse;
		if(game_state->user.is_dragging == 1) {
			Vector pre_mouse = game_state->platform.mouse;
			Vector cell0 = game_state->user.last_cell_in_drag;
			Vector cell1 = convert_coord(game_state->platform.bitmap, game_state->platform.screen, game_state->platform.mouse);
			auto dx = abs(cell0.x - cell1.x);
			auto dy = abs(cell0.y - cell1.y);
			auto d = max(dx, dy);
			for(uint i = 1; i < d; i += 1) {
				auto cell = lerp(cell0, cell1, cast(float, i)/d);
				set_cell(cells0, cells.width, cell, 1);
			}
			set_cell(cells0, cells.width, cell1, 1);
			game_state->user.last_cell_in_drag = cell1;
			do_render_update = true;
		}
	}
	for_each_in_range(id, 1, BUTTONS_TOTAL) {
		auto times_pressed = input.button_presses[id];
		for_each_lt(i, times_pressed) {
			bool is_down = (input.is_down[id]^((times_pressed - i)%2 == 0));
			bool pre_is_down = game_state->platform.button_is_down[id];
			if(pre_is_down == is_down) continue;
			game_state->platform.button_is_down[id] = is_down;
			if(id == SPACE and is_down) {
				game_state->run_simulation ^= 1;
			} else if(id == M1){
				if(is_down) {
					Vector cell = convert_coord(game_state->platform.bitmap, game_state->platform.screen, game_state->platform.mouse);
					set_cell(cells0, cells.width, cell, 1);
					game_state->user.last_cell_in_drag = cell;
					game_state->user.is_dragging = 1;
					do_render_update = true;
				} else {
					game_state->user.is_dragging = 0;
				}
			}
		}
	}
	if(game_state->user.is_dragging == 1) {
		set_cell(cells0, cells.width, game_state->user.last_cell_in_drag, 1);
	}

	if(!game_state->run_simulation) {//exit here
		if(do_render_update) {
			render_from_cells(pixels, cells0, cells, game_state->platform.bitmap);
		}
		return ret;
	}

	uint32 up_row  = (cells.height - 2)*cells.width;
	uint32 cur_row = (cells.height - 1)*cells.width;
	for(uint32 down_row = 0; down_row < cells.width*cells.height; down_row += cells.width) {
		uint32 up_col   = cells.width - 2;
		uint32 cur_col  = cells.width - 1;
		for_each_lt(down_col, cells.width) {
			uint8 total_adj_cell = 0;
			total_adj_cell += cells0[up_row   + up_col];
			total_adj_cell += cells0[up_row   + cur_col];
			total_adj_cell += cells0[up_row   + down_col];
			total_adj_cell += cells0[cur_row  + up_col];
			total_adj_cell += cells0[cur_row  + down_col];
			total_adj_cell += cells0[down_row + up_col];
			total_adj_cell += cells0[down_row + cur_col];
			total_adj_cell += cells0[down_row + down_col];
			bool new_state = ((total_adj_cell == 3) or (cells0[cur_row + cur_col] and total_adj_cell == 2));
			cells1[cur_row + cur_col] = new_state;

			pixels[cur_row + cur_col] = new_state ? 0xFFFFFF : 0x111111;
			up_col = cur_col;
			cur_col = down_col;
		}
		up_row = cur_row;
		cur_row = down_row;
	}
	game_state->is_first_cells_active ^= 1;
	if(game_state->user.is_dragging == 1) {
		Vector cell = game_state->user.last_cell_in_drag;
		pixels[cell.y*cells.width + cell.x] = 0xFFFFFF;
	}
	return ret;
}


float get_delta_ms(uint64 t0, uint64 t1) {
	return (1000.0f*(t1 - t0))/SDL_GetPerformanceFrequency();
}

int main(int argc, char** argv) {
	int succ = SDL_Init(SDL_INIT_EVERYTHING);
	if(succ == -1) {
		//TODO: Handle failure
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		return -1;
	}

	Dim screen = {1800, 1000};

	SDL_Window* window = SDL_CreateWindow("life", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen.width, screen.height, SDL_WINDOW_RESIZABLE);
	if(!window) {
		//TODO: Handle null window
		SDL_Quit();
		return -1;
	}
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
	if(!window) {
		//TODO: Handle null renderer
		SDL_Quit();
		return -1;
	}

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);


	Dim bitmap = {screen.width/2, screen.height/2};
	SDL_Texture* bitmap_handle = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, bitmap.width, bitmap.height);

	uint64 game_memory_size = 64*MEGABYTE;
	byte* game_memory = malloc(byte, game_memory_size);
	uint64 trans_memory_size = 64*MEGABYTE;
	byte* trans_memory = malloc(byte, trans_memory_size);

	PlatformData platform = {};
	platform.mouse.x = 0;
	platform.mouse.y = 0;
	platform.screen = screen;
	platform.bitmap = bitmap;
	initialize_game(game_memory, &platform);


	float ms_per_frame = 1000.0f/30.0f;
	uint32 sleep_resolution_ms = 1;
	int display_index = SDL_GetWindowDisplayIndex(window);
	SDL_DisplayMode dm;
	if(SDL_GetDesktopDisplayMode(display_index, &dm) < 0) {
	    SDL_Log("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
	} else {
		ms_per_frame = (1000.0f/dm.refresh_rate)*2;
	}

	uint64 start_of_frame = SDL_GetPerformanceCounter();
	uint64 end_of_compute;
	bool is_game_running = 1;
	while(true) {
		GameInput input = {};
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				is_game_running = 0;
				break;
			} else if(event.type == SDL_MOUSEMOTION) {
				input.mouse_move_plus_one.x = event.motion.x + 1;
				input.mouse_move_plus_one.y = event.motion.y + 1;
			} else if(event.type == SDL_MOUSEBUTTONDOWN or event.type == SDL_MOUSEBUTTONUP) {
				InputType button = INPUT_NULL;
				if(event.button.button == SDL_BUTTON_LEFT) {
					button = M1;
				} else if(event.button.button == SDL_BUTTON_RIGHT) {
					button = M2;
				} else if(event.button.button == SDL_BUTTON_MIDDLE) {
					button = M3;
				}
				if(button != INPUT_NULL) {
					input.button_presses[button] += 1;
					input.is_down[button] = (event.button.state == SDL_PRESSED);
				}
			} else if(event.type == SDL_KEYDOWN or event.type == SDL_KEYUP) {
				if(!event.key.repeat) {
					auto scancode = event.key.keysym.scancode;
					InputType button = INPUT_NULL;
					if(scancode == SDL_SCANCODE_SPACE) {
						button = SPACE;
					}
					if(button != INPUT_NULL) {
						// printf("button was: %d, status: %d\n", button, (event.key.state == SDL_PRESSED));
						bool is_down = (event.key.state == SDL_PRESSED);
						if(input.button_presses[button] > 0) {
							if(input.is_down[button] != is_down) {
								input.is_down[button] = is_down;
								input.button_presses[button] += 1;
							}
						} else {
							input.is_down[button] = is_down;
							input.button_presses[button] += 1;
						}
					}
				}
			} else if(event.type == SDL_WINDOWEVENT) {
				auto event_id = event.window.event;
				if(event_id == SDL_WINDOWEVENT_SHOWN) {

				} else if(event_id == SDL_WINDOWEVENT_HIDDEN) {

				} else if(event_id == SDL_WINDOWEVENT_ENTER) {

				} else if(event_id == SDL_WINDOWEVENT_LEAVE) {

				} else if(event_id == SDL_WINDOWEVENT_RESIZED) {
					Dim new_screen = {event.window.data1, event.window.data2};
					assert(new_screen.width > 0 and new_screen.height > 0);
					SDL_DestroyTexture(bitmap_handle);
					bitmap.width = new_screen.width/2;
					bitmap.height = new_screen.height/2;
					bitmap_handle = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, bitmap.width, bitmap.height);
					screen = new_screen;
					input.window_resize = screen;
					input.bitmap_resize = bitmap;
				}
			}
		}
		if(!is_game_running) break;
		RenderData* render_data = update_game(game_memory, trans_memory, input);
		SDL_UpdateTexture(bitmap_handle, 0, render_data->bitmap, render_data->bitmap_pitch);
		SDL_RenderCopy(renderer, bitmap_handle, 0, 0);

		end_of_compute = SDL_GetPerformanceCounter();
		float time_to_compute = get_delta_ms(start_of_frame, end_of_compute);
		uint64 end_of_frame;
		if(time_to_compute < ms_per_frame) {
			uint32 time_to_wait = cast(uint32, ms_per_frame - time_to_compute);
			if(time_to_wait > sleep_resolution_ms) {//TODO: decide sleep resolution
				SDL_Delay(time_to_wait - sleep_resolution_ms);
			}
			end_of_frame = SDL_GetPerformanceCounter();
			//TODO: make get_delta faster?
			while(get_delta_ms(start_of_frame, end_of_frame) < ms_per_frame) {
				end_of_frame = SDL_GetPerformanceCounter();
			}
		} else {
			//TODO: framerate manipulation
			printf("frame took longer than expected: %2.2f", time_to_compute);
			end_of_frame = end_of_compute;
		}
		printf("%2.2f\n", time_to_compute);
		start_of_frame = end_of_frame;
		SDL_RenderPresent(renderer);

		memzero(trans_memory, sizeof(byte)*trans_memory_size);//only for debugging, prevents transient data from being using between frames
	}

	//only program exit point
	SDL_Quit();
	return 0;
}
