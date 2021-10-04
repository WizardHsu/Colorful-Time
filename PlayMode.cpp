#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <array>
#include <cstddef>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <random>

unsigned int index_buffer_content[] {0, 1, 2, 1, 2, 3};
GLuint PlayMode::index_buffer {0};
Load<void> load_index_buffer(LoadTagEarly, [](){

	glGenBuffers(1, &PlayMode::index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PlayMode::index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), index_buffer_content, GL_STATIC_DRAW);
});


GLuint meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("game4.pnct"));
	meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});
Load< Scene > game4_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("game4.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});


// load the sound
Load< Sound::Sample > brush(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("./brush.opus"));
});
Load< Sound::Sample > background(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("./background.opus"));
});


PlayMode::PlayMode() : scene(*game4_scene){
	
	camera = &scene.cameras.front();
	for (auto &transform : scene.transforms)
	{
		if (transform.name == "blue")
		{
			color_options[0] = &transform;
			current_options[0] = false;
			options_base_position[0] = transform.position;
			options_move_speed[0] = 0.1f;
		}
		else if (transform.name == "red")
		{
			color_options[1] = &transform;
			current_options[1] = false;
			options_base_position[1] = transform.position;
			options_move_speed[1] = 0.1f;
		}
		else if (transform.name == "yellow")
		{
			color_options[2] = &transform;
			current_options[2] = false;
			options_base_position[2] = transform.position;
			options_move_speed[2] = 0.1f;
		}
	}

	FT_Error error_init_FreeType = FT_Init_FreeType(&ft_library);
	if (error_init_FreeType) 
	{
		std::cout << "Error code: " << error_init_FreeType << std::endl;
		throw std::runtime_error("Cannot initialize the library");
	}

	std::string font_path = data_path("The Lost Paintings.ttf");
	FT_Error error_new_face = FT_New_Face(ft_library, &(font_path[0]), 0, &ft_face);
	if (error_new_face) 
	{
		std::cout << "Error code: " << error_new_face << std::endl;
		throw std::runtime_error("Cannot access the font file");
	}

	FT_Error error_set_char_size = FT_Set_Char_Size(ft_face, 0, 7000, 0, 0);
	if (error_set_char_size)
	{
		std::cout << "Error code: " << error_set_char_size << std::endl;
		throw std::runtime_error("Cannot set the char size");
	}

	text_anchor = glm::vec2(-0.4f, 0.6f);

	
	// sound
	Sound::loop(*background, 0.8f, 0.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) 
	{
		if (evt.key.keysym.sym == SDLK_SPACE) 
		{
			isPressSpace = true;
			return true;
		} 
		else if (evt.key.keysym.sym == SDLK_b) 
		{
			current_options[0] = !current_options[0];
			return true;
		} 
		else if (evt.key.keysym.sym == SDLK_r) 
		{
			current_options[1] = !current_options[1];
			return true;
		} 
		else if (evt.key.keysym.sym == SDLK_y) 
		{
			current_options[2] = !current_options[2];
			return true;
		} 
	} 
	return false;
}

void PlayMode::update(float elapsed) {

	if (current_time < fadeIn_time) current_time += elapsed;
	text_color = glm::u8vec4(0xf0, 0xf0, 0xf0, 0xf0);
	text_color.w = std::min((current_time / fadeIn_time), 1.0f) * 255.0f;

	if (text == "")
	{
		text = state_array[current_state].result_hint + 
			format_array[current_format] + state_array[current_state].guide_hint;
	}
	
	// submit color
	if (isPressSpace)
	{
		isPressSpace = false;
		size_t color_index = getCurrentColor();
		if (color_index < 7)
		{
			current_bg_color = color_array[color_index];
			for (size_t i = 0; i < current_options.size(); i++)
			{
				current_options[i] = false;
			}
		}
		
		current_state = state_array[current_state].next_state[color_index];
		current_format++;
		current_time = 0.0f;
		text = state_array[current_state].result_hint + 
			format_array[current_format] + state_array[current_state].guide_hint;
		Sound::play(*brush, 1.0f, 0.0f);

		if (current_format >= format_array.size())
		{
			previous_states.clear();
			previous_colors.clear();
			current_format = 0;
			current_state = 0;
			current_bg_color = glm::vec3(1.0f, 1.0f, 1.0f);
		}
		else
		{
			previous_states.emplace_back(current_state);
			previous_colors.emplace_back(color_index);
		}
	}

	// set color options
	for (size_t i = 0; i < current_options.size(); i++)
	{
		if (current_options[i])
		{
			color_options[i]->position.z += options_move_speed[i];
			if (color_options[i]->position.z >= options_base_position[i].z + 1.0f ||
				color_options[i]->position.z < options_base_position[i].z)
			{
				options_move_speed[i] *= -1.0f;
			}
		}
		else
		{
			color_options[i]->position.z = options_base_position[i].z ;
		}
	}
	
	// update data for the final scene
	if (current_format == format_array.size() - 1)
	{
		for (size_t i = 0; i < current_options.size(); i++)
		{
			color_options[i]->scale = glm::vec3(0.0f, 0.0f, 0.0f);
		}

		// calculate final score
		final_score[0] = 3;
		for (auto &state: previous_states)
		{
			// creative
			if (state == 7 || state == 9 || state == 11 || state == 12 || state == 13 || state == 14 || state == 15)
			{
				final_score[0] ++;
			}
		}
		final_score[0] = std::min(final_score[0], static_cast<size_t>(5));

		final_score[1] = 2;
		if (previous_colors[1] != previous_colors[2]) final_score[1]++;
		if (previous_colors[3] == color_contrast_array[previous_colors[1]]) final_score[1] += 2;
		else if (previous_colors[1] == 6 && previous_colors[3] != 6) final_score[1] += 2;
		if (previous_colors[3] != previous_colors[4]) final_score[1]++;

		std::sort(previous_colors.begin(), previous_colors.end());
		final_score[2] = std::unique(previous_colors.begin(), previous_colors.end()) - previous_colors.begin();


		text += "The main theme is " + color_string_array[previous_colors[0]] + " " + state_array[previous_states[0]].type + ".\n";
		text += "The " + color_string_array[previous_colors[1]] + " " + state_array[previous_states[1]].type + 
				" decorated with " + color_string_array[previous_colors[2]] + " " + state_array[previous_states[2]].type + ".\n";
		text += "The main character wears " + color_string_array[previous_colors[3]] + " " + state_array[previous_states[3]].type + "\n" +
				"with " + color_string_array[previous_colors[4]] + " " + state_array[previous_states[4]].type + " standing in the scene.\n";
		text += "Creative: " +  std::to_string(final_score[0]) + " / 5\n" + 
				"Skill: " +  std::to_string(final_score[1]) + " / 5\n" + 
				"Colorful: " +  std::to_string(final_score[2]) + " / 5 ";
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	
	//draw the mesh
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(current_bg_color.x, current_bg_color.y, current_bg_color.z, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it

	scene.draw(*camera);
	
	// draw the text
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LESS);

	{
		glDisable(GL_DEPTH_TEST);

		//clear text
		for (GLuint texture_id : texture_ids)
		{
			glDeleteTextures(1, &texture_id);
		}
		texture_ids.clear();
		if (hb_buffer) 
		{
			hb_buffer_destroy(hb_buffer);
			hb_buffer = nullptr;
		}
		if (hb_font) 
		{
			hb_font_destroy(hb_font);
			hb_font = nullptr;
		}
		
		// set text
		hb_font = hb_ft_font_create(ft_face, nullptr);
		hb_buffer = hb_buffer_create();
		hb_buffer_add_utf8(hb_buffer, &text[0], -1, 0, -1);
		hb_buffer_set_direction(hb_buffer, HB_DIRECTION_LTR);
		hb_buffer_set_script(hb_buffer, HB_SCRIPT_LATIN);
		hb_buffer_set_language(hb_buffer, hb_language_from_string("en", -1));
		hb_shape(hb_font, hb_buffer, nullptr, 0);
		glyph_info = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
		glyph_pos = hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

		FT_GlyphSlot slot = ft_face->glyph;

		for (size_t i = 0; i < glyph_count; i++) 
		{
			FT_Error error = FT_Load_Char(ft_face, text[i], FT_LOAD_RENDER);
			if (error) continue;

			texture_ids.emplace_back(0);
			GLuint &texture_id = texture_ids.back();

			auto &bitmap = slot->bitmap;

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glGenTextures(1, &texture_id);
			glBindTexture(GL_TEXTURE_2D, texture_id);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap.width, bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.buffer);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		//draw the text	
		float current_x = (text_anchor.x + 1.0f) * drawable_size.x / 2.0f;
		float current_y = (text_anchor.y + 1.0f) * drawable_size.y / 2.0f;
		size_t current_line = 0;

		glUseProgram(texture2d_program->program);
		for (size_t i = 0; i < glyph_count; ++i) 
		{
			if (text[i] == '\n') 
			{
				current_line++;
				current_x = (text_anchor.x + 1.0f) * drawable_size.x / 2.0f;
				current_y = (text_anchor.y + 1.0f) * drawable_size.y / 2.0f - (float)(current_line * line_height);
				continue;
			}
		
			FT_Load_Char(ft_face, text[i], FT_LOAD_RENDER);
			
			auto& bitmap = slot->bitmap;
			glm::vec2 start = {
				2 * (current_x + slot->bitmap_left) / drawable_size.x - 1,
				2 * (current_y - bitmap.rows + slot->bitmap_top) / drawable_size.y - 1
			};
			glm::vec2 end = {
				start[0] + bitmap.width * 2.0f / drawable_size.x,
				start[1] + bitmap.rows * 2.0f / drawable_size.y
			};
			Texture2DProgram::Vertex vertexes[] {
				{{start.x, start.y}, text_color, {0, 1}},
				{{end.x, start.y}, text_color, {1, 1}},
				{{start.x, end.y}, text_color, {0, 0}},
				{{end.x, end.y}, text_color, {1, 0}}
			};

			GLuint vertex_buffer, vertex_array;
			glGenBuffers(1, &vertex_buffer);
			vertex_array = texture2d_program->GetVao(vertex_buffer);

			glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Texture2DProgram::Vertex), static_cast<const void*>(vertexes), GL_STATIC_DRAW);
			glBindVertexArray(vertex_array);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture_ids[i]);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, static_cast<const void*>(0));
			glDeleteBuffers(1, &vertex_buffer);
			glDeleteVertexArrays(1, &vertex_array);

			current_x += glyph_pos[i].x_advance / 64.0f;;
			current_y += glyph_pos[i].y_advance / 64.0f;
		}
	}

	GL_ERRORS();
}


size_t PlayMode::getCurrentColor() {

	size_t chosen_color = 0;
	size_t sum = 0;
	for (size_t i = 0; i < 3; i++)
	{
		if (!current_options[i]) continue;
		chosen_color++;
		sum += i;
	}

	if (chosen_color == 1)
	{
		return sum;
	}
	else if (chosen_color == 2)
	{
		return sum + 2;
	}
	else if (chosen_color == 3)
	{
		return 6;
	}
	
	return 7;
};

