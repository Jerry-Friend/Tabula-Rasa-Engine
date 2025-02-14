
#include "trApp.h"
#include "trInput.h"
#include "trRenderer3D.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_sdl.h"
#include "ImGui/imgui_impl_opengl2.h"

#include "trFileSystem.h"
#include "trWindow.h"


#define MAX_KEYS 300

trInput::trInput() : trModule()
{
	this->name = "Input";
	keyboard = new KEY_STATE[MAX_KEYS];
	memset(keyboard, KEY_IDLE, sizeof(KEY_STATE) * MAX_KEYS);
	memset(mouse_buttons, KEY_IDLE, sizeof(KEY_STATE) * MAX_MOUSE_BUTTONS);
}

// Destructor
trInput::~trInput()
{
	delete[] keyboard;
}

// Called before render is available
bool trInput::Init()
{
	TR_LOG("trInput: Init SDL input event system");
	bool ret = true;
	SDL_Init(0);

	if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0)
	{
		TR_LOG("trInput: SDL_EVENTS could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}

	return ret;
}

// Called every draw update
bool trInput::PreUpdate(float dt)
{
	SDL_PumpEvents();

	const Uint8* keys = SDL_GetKeyboardState(NULL);

	for (int i = 0; i < MAX_KEYS; ++i)
	{
		if (keys[i] == 1)
		{
			if (keyboard[i] == KEY_IDLE)
				keyboard[i] = KEY_DOWN;
			else
				keyboard[i] = KEY_REPEAT;
		}
		else
		{
			if (keyboard[i] == KEY_REPEAT || keyboard[i] == KEY_DOWN)
				keyboard[i] = KEY_UP;
			else
				keyboard[i] = KEY_IDLE;
		}
	}

	Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);

	mouse_x /= App->window->scale;
	mouse_y /= App->window->scale;
	mouse_z = 0;

	for (int i = 0; i < 5; ++i)
	{
		if (buttons & SDL_BUTTON(i))
		{
			if (mouse_buttons[i] == KEY_IDLE)
				mouse_buttons[i] = KEY_DOWN;
			else
				mouse_buttons[i] = KEY_REPEAT;
		}
		else
		{
			if (mouse_buttons[i] == KEY_REPEAT || mouse_buttons[i] == KEY_DOWN)
				mouse_buttons[i] = KEY_UP;
			else
				mouse_buttons[i] = KEY_IDLE;
		}
	}

	mouse_x_motion = mouse_y_motion = 0;

	bool quit = false;
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		ImGui_ImplSDL2_ProcessEvent(&e);

		switch (e.type)
		{
			case SDL_MOUSEWHEEL:
				mouse_z = e.wheel.y;
				break;

			case SDL_MOUSEMOTION:
				mouse_x = e.motion.x / App->window->scale;
				mouse_y = e.motion.y / App->window->scale;

				mouse_x_motion = e.motion.xrel / App->window->scale;
				mouse_y_motion = e.motion.yrel / App->window->scale;
				break;

			case SDL_QUIT:
				quit = true;
				break;

			case SDL_WINDOWEVENT:
			{
				if (e.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					App->render->OnResize(e.window.data1, e.window.data2);
					App->window->SetWidth(e.window.data1);  // TODO: do this with events
					App->window->SetHeight(e.window.data2); // TODO: do this with events
				}
					
				break;
			}

			case SDL_DROPFILE:
			{
				dropped_file_path = e.drop.file;

				if (dropped_file_path != nullptr) 
				{
					TR_LOG("File path dropped on window: %s", dropped_file_path);

					App->file_system->CopyFileFrom(dropped_file_path);

					/*std::string file_format(".xyz");
					file_format[3] = dropped_filedir[strlen(dropped_filedir) - 1];
					file_format[2] = dropped_filedir[strlen(dropped_filedir) - 2];
					file_format[1] = dropped_filedir[strlen(dropped_filedir) - 3];
					file_format[0] = dropped_filedir[strlen(dropped_filedir) - 4];*/

					//if (file_format.compare(".fbx") == 0 || file_format.compare(".FBX") == 0)

					//else																		// TODO when mouse picking or smth
						//App->texture->LoadImageFromPath(dropped_filedir);
				}
				else
					TR_LOG("Cannot get file dropped path");

				SDL_free(dropped_file_path);
				break;
			}
		}
	}

	if (quit == true || keyboard[SDL_SCANCODE_ESCAPE] == KEY_UP)
		return false;

	return true;
}

// Called before quitting
bool trInput::CleanUp()
{
	TR_LOG("Quitting SDL input event subsystem");
	SDL_QuitSubSystem(SDL_INIT_EVENTS);
	return true;
}