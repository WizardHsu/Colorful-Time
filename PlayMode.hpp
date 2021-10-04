#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "Texture2DProgram.hpp"

#include <cstddef>
#include <glm/glm.hpp>
#include <ft2build.h>
#include <string>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

#include <vector>
#include <deque>
#include <array>


struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//font
	FT_Library ft_library;
	FT_Face ft_face;
	hb_font_t* hb_font { nullptr };
	hb_buffer_t* hb_buffer { nullptr };
	std::vector<GLuint> texture_ids;
	
	unsigned int glyph_count;
	hb_glyph_info_t* glyph_info { nullptr };
	hb_glyph_position_t* glyph_pos { nullptr };
	
	glm::vec2 text_anchor;
	std::string text;
	glm::u8vec4 text_color;
	int line_height { 110 };
	static GLuint index_buffer;
	const size_t line_count = 5;
	float fadeIn_time = 6.0f;
	float current_time = 0.0f;

	//mesh
	Scene scene;
	Scene::Camera *camera { nullptr };
	std::array<Scene::Transform *, 3> color_options;
	std::array<glm::vec3, 3> options_base_position;
	std::array<float, 3> options_move_speed;

	//----- game state -----
	std::array<glm::vec3, 7> color_array = {
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(0.5f, 0.0f, 0.5f),
		glm::vec3(1.0f, 0.65f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
	};
	size_t getCurrentColor();
	glm::vec3 current_bg_color { glm::vec3(1.0f, 1.0f, 1.0f) };
	std::array<bool, 3> current_options;
	size_t current_state = 0;
	size_t current_format = 0;
	std::vector<size_t> previous_states;
	std::vector<size_t> previous_colors;
	bool isPressSpace { false };
	std::array<size_t, 3> final_score = {3, 2, 0};


	struct State {
		State(std::string type_, std::string result, std::string guide, 
		size_t s0, size_t s1, size_t s2, size_t s3, size_t s4, size_t s5, size_t s6):
		type(type_), result_hint(result), guide_hint(guide){
			std::array<size_t, 7> values = {s0, s1, s2, s3, s4, s5, s6};
			next_state = values;
		}
		std::string type;
		std::string result_hint;
		std::string guide_hint;
		std::array<size_t, 7> next_state;
	};

	// Blue Red Yellow Purple Organge Green Black
	std::array<std::string, 7> color_string_array = { 
		"blue", "red", "yellow", "purple", "orange", "green", "black"
	};
	std::array<std::size_t, 7>color_contrast_array = {
		4, 5, 3, 2, 0, 1, 6
	};
	
	std::array<State, 27> state_array = {
		// Start 0
		State("","", "", 1, 3, 3, 3, 3, 2, 3),
		
		// Ocean 1
		State("ocean", "You paint the beutiful ocean.", "sky background.", 4, 5, 5, 7, 5, 7, 6),
		// Grass 2
		State("ground", "You paint thecozy green space.", "sky background.", 4, 5, 5, 7, 5, 7, 6),
		// Floor 3
		State("floor", "You paint the interior space.\nYou choose a great color for the floor. ", "wall background.", 9, 8, 8, 9, 8, 10, 11),
		
		// Clean sky 4
		State("sky", "It's the clean summer sky!", "sky!", 12, 18, 18, 12, 18, 14, 14),
		// Dusk sky 5
		State("sky", "What the romantic dusk!", "sky!", 12, 18, 18, 12, 18, 14, 14),
		// Night sky 6
		State("sky", "Many stories would happen at night - good choice!", "sky!", 12, 13, 13, 12, 13, 14, 14),
		// Weird sky 7
		State("sky", "You are creating a mysterious world.", "sky!", 12, 18, 18, 12, 18, 14, 14),
		// Warm Room 8
		State("wall", "It would be cozy\nif stay this room in the winter~", "wall!", 15, 15, 15, 15, 15, 15, 15),
		// Ocean Room 9
		State("wall", "It's a ocean style room!", "wall!", 16, 16, 16, 16, 16, 16, 16),
		// Forest Room 10
		State("wall", "It's a forest style room!", "wall!", 14, 13, 13, 14, 13, 14, 14),
		// Universe Room 11
		State("wall", "It's a universe style room!", "wall!", 17, 18, 18, 17, 18, 17, 17),

		// Airplane 12 - sky
		State("airplane", "Oh! A cool airplane in the sky!", "", 19, 20, 21, 22, 23, 24, 25),
		// Star 13 - sky
		State("star", "Starry style is your preference.", "", 19, 20, 21, 22, 23, 24, 25),
		// Bird 14 - sky, wall
		State("bird", "I could hear the bird you draw is singing...", "", 19, 20, 21, 22, 23, 24, 25),
		// Flower 15 - wall
		State("flower", "Beautiful flowers fit the space.", "", 19, 20, 21, 22, 23, 24, 25),
		// Fish 16 - wall
		State("fish", "Adorable fish is swimming in the space.", "", 19, 20, 21, 22, 23, 24, 25),
		// Planet 17 - wall
		State("planet", "You draw a mysterious planet.", "", 19, 20, 21, 22, 23, 24, 25),
		// Sun 18 - sky, wall
		State("sun", "What a dazzling sun!", "", 19, 20, 21, 22, 23, 24, 25),

		// Shirt Blue 19
		State("shirt", "The character wears the calm blue shirt.", "", 26, 26, 26, 26, 26, 26, 26),
		// Shirt Red 20
		State("shirt", "The character wears the happy scarlet.", "", 26, 26, 26, 26, 26, 26, 26),
		// Shirt Yellow 21
		State("shirt", "The character wears the energetic yellow.", "", 26, 26, 26, 26, 26, 26, 26),
		// Shirt Purple 22
		State("shirt", "The character wears the fashion violet.", "", 26, 26, 26, 26, 26, 26, 26),
		// Shirt Orange 23
		State("shirt", "The character wears the sunny orange.", "", 26, 26, 26, 26, 26, 26, 26),
		// Shirt Green 24
		State("shirt", "The character wears the relaxed green.", "", 26, 26, 26, 26, 26, 26, 26),
		// Shirt Black 25
		State("shirt", "The character wears the dull black.", "", 26, 26, 26, 26, 26, 26, 26),

		// End 26
		State("pattern", "", "", 0, 0, 0, 0, 0, 0, 0),
	};

	std::array<std::string, 6> format_array = {

		"Now you have a white canvas in front of you.\nYou would choose a color to paint the bottom area.",
		"\nYou are looking for a color for the ",
		"\nLet's add something to the ",
		"\nThen you put a standing character in the scene.\nYou would draw a shirt for him/her.\nYou would choose a color contrast with the background.",
		"\nFinally, you would choose a color you have not use\nto add the patterns for the shirt.",
		"Let's see want you draw:\n"
	};
};
