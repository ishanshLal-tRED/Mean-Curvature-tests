#pragma once
#include <optional>
#include <tuple>
#include <string>
#include <future>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <type_traits>

#define MIN(x,y) (x > y ? y :  x)
#define MAX(x,y) (x > y ? x :  y)
#define ABS(x)   (x > 0 ? x : -x)
#define MOD(x,y) (x - ((int)(((float)x) / y) * y))

namespace Helper
{
	// delete after use
	std::string ReadFileAsString (const char *filepath, char ignore_until = 0);
	namespace SHADER
	{
		std::optional<GLuint> CreateProgram (const char *source, GLenum shaderTyp);
		std::optional<GLuint> CreateProgram (const char *source1, GLenum shaderTyp1, const char *source2, GLenum shaderTyp2);
		std::optional<GLuint> CreateProgram (const char *source1, GLenum shaderTyp1, const char *source2, GLenum shaderTyp2, const char *source3, GLenum shaderTyp3);
	}
	namespace TEXTURE_2D
	{
		enum class MAPPING
		{
			CUBIC = 0,
			MERCATOR
		};
		void SetData (GLuint ID, uint32_t width, uint32_t height, GLenum src_format, GLenum src_type, const void *data, uint8_t level);
		GLuint Upload (const uint8_t *data, uint32_t width, uint32_t height, uint8_t channels);
		GLuint Upload (const void *data, uint32_t width, uint32_t height, GLenum internal_format, GLenum src_format, GLenum src_type, GLenum min_filter = GL_NEAREST, GLenum mag_filter = GL_NEAREST);
		std::optional<std::tuple<GLuint, uint32_t, uint32_t>> LoadFromDiskToGPU ();
		std::optional<std::tuple<GLuint, uint32_t, uint32_t>> LoadFromDiskToGPU (const char *location);
		std::optional<std::tuple<GLuint, uint32_t, uint32_t>> LoadFromDiskToGPU (const char *location, const MAPPING loadAs, const MAPPING storeAs);
	}
	namespace ASSET_LOADER
	{
		bool LoadOBJ_meshOnly (const char *path, std::vector<std::pair<glm::vec3, glm::vec3>> &out_verticeDatas, std::vector<uint32_t> &out_indices);
		bool LoadOBJ_basic_VertexOnly (const char *path, std::vector<glm::vec3> &out_vertices, std::vector<glm::vec2> &out_uvs, std::vector<glm::vec3> &out_normals);
	}
	namespace MATH
	{
		glm::mat3 MakeRotationX (float radians);
		glm::mat3 MakeRotationY (float radians);
		glm::mat3 MakeRotationZ (float radians);
	}
	namespace IMGUI
	{
		bool DrawVec3Control (const char *label, glm::vec3 &values, float resetValue = 0.0f, float columnWidth = 100.0f);
	}

	// ADDING as a node has to wait until later
	namespace Noise
	{
		enum class NOISE_TYP
		{
			SIMPLEX = 0,
			FRACTAL_BROWNIM_MOTION,
			TURBULANCE,
		};
		// 2D simplex noise
		float Snoise2 (float x, float y);
		
		//Fbm2 is Fractal Brownim Motion type noise
		float Fbm2 (float x, float y, float freq, float lac, float gain, int octaves);

		//Turbulance is turbulent fractal type noise
		float Turbulance (float x, float y, float freq, float lac, float gain, int octaves);

		// For Simplex Noise oly freq is required
		template<typename channel_type = glm::vec3>
		GLuint MakeTexture (uint32_t width, uint32_t height, NOISE_TYP noise_type, std::vector<channel_type> gradient, float freq = 0.01f, float lac = 2.0f, float gain = 0.5f, int octaves = 5)
		{
			if (gradient.size () < 2) {
				gradient.insert (gradient.begin (), channel_type (0));
			}
			if (gradient.size () < 2) {
				gradient.insert (gradient.end (), channel_type (1));
			}
			
			uint8_t num_of_channels;
			if (std::is_base_of<glm::vec3, channel_type> ()) {
				num_of_channels = 3;
			} 
			else if (std::is_base_of<glm::vec4, channel_type> ()) {
				num_of_channels = 4;
			} else {
				// num_of_channels = 0;
				LOG_ASSERT (false);
			}

			static constexpr uint32_t NumOfThreads = 4; // Mine is quad core CPU (logical)
			std::vector<float> noise_data;
			noise_data.resize (width*height);

			std::vector<std::future<glm::vec2>> futures;
			futures.resize (NumOfThreads);

			// Note Batch starts with 0;
			auto func = [&](const uint32_t batch_num) -> glm::vec2 {
					const uint32_t batches_size = NumOfThreads;

					uint32_t wid_x = width/batches_size + 1;
					const uint32_t offset = batch_num*wid_x;
					if (batch_num == batches_size-1)
						wid_x -= (batches_size*wid_x) % width;

					float min_val = 1.0f, max_val = 0.0f;
					for (uint32_t Y = 0; Y < height; Y++) {
						for (uint32_t X = 0; X < wid_x; X++) {
							uint32_t index = offset + X + (Y*width);
							float noise;

							switch (noise_type) {
								case NOISE_TYP::SIMPLEX:
									noise = Snoise2 ((X + offset)*freq, (Y)*freq);
									break;
								case NOISE_TYP::FRACTAL_BROWNIM_MOTION:
									noise = Fbm2 (X + offset, Y, freq, lac, gain, octaves);
									break;
								case NOISE_TYP::TURBULANCE:
									noise = Turbulance (X + offset, Y, freq, lac, gain, octaves);
									break;
								default:
									LOG_ASSERT (false);
							}

							min_val = MIN (min_val, noise);
							max_val = MAX (max_val, noise);

							noise_data[index] = noise; // create greyscale
						}
					}

					return glm::vec2 (min_val, max_val);
				};
			for (uint32_t i = 0; i < NumOfThreads; i++)
			{
				futures[i] = std::async (func, i);
			}

			glm::vec2 min_max = futures[0].get();
			for (uint32_t i = 1; i < futures.size (); i++)
			{
				glm::vec2 _min_max = futures[i].get ();

				min_max[0] = MIN (min_max[0], _min_max[0]);
				min_max[1] = MAX (min_max[1], _min_max[1]);
			}
			
			// uint8_t[sizeof (channel_type)/sizeof (float)]
			std::vector<std::array<uint8_t, sizeof (channel_type)/sizeof (float)>> pixel_data;
			pixel_data.resize (width*height);
			
			// returns dummy glm::vec2 (0) to reuse futures;
			auto func2 = [&](const uint32_t batch_num) {
				const uint32_t batches_size = NumOfThreads;

				uint32_t wid_x = width/batches_size + 1;
				const uint32_t offset = batch_num*wid_x;
				if (batch_num == batches_size-1)
					wid_x -= (batches_size*wid_x) % width;

				for (uint32_t Y = 0; Y < height; Y++) {
					for (uint32_t X = 0; X < wid_x; X++) {
						uint32_t index = offset + X + (Y*width);
						float factor = (noise_data[index] - min_max[0]) / (min_max[1] - min_max[0]);
						channel_type color;
						{
							uint32_t region = std::min(uint32_t(factor*(gradient.size () - 1)), uint32_t (gradient.size () - 2));
							float grad_del_wid = 1.0 / (gradient.size () - 1);
							factor = MOD (factor, grad_del_wid)*(gradient.size () - 1);
							color = gradient[region] + (gradient[region + 1] - gradient[region])*factor;
						}
						for (uint8_t i = 0; i < num_of_channels; i++)
						{
							pixel_data[index][i] = 255.999*color[i];
						}
					}
				}

				return glm::vec2 (0);
			};

			for (uint32_t i = 0; i < NumOfThreads; i++) {
				futures[i] = std::async (func2, i);
			}

			for (uint32_t i = 1; i < futures.size (); i++) {
				glm::vec2 _dummy = futures[i].get ();
			}

			return TEXTURE_2D::Upload ((uint8_t *)(pixel_data.data ()), width, height, num_of_channels);
		}
	};
}
std::ostream &operator<<(std::ostream &cout, const glm::mat4 &matrix);
std::ostream &operator<<(std::ostream &cout, const glm::vec3 &vec);