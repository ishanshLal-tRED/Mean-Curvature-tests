#pragma once

#include <GLCore.h>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "Utilities/utility.h"

class SqrShader_Base: public GLCore::TestBase
{
public:
	SqrShader_Base (const char* name, const char* discription = "just a base"
							 , const char* default_sqr_shader_vert_src = s_default_sqr_shader_vert
							 , const char* default_sqr_shader_frag_src = s_default_sqr_shader_frag
	);
	virtual ~SqrShader_Base () = default;

	virtual void OnDetach         () = 0;
	virtual void OnAttach         () = 0;
	virtual void OnUpdate (GLCore::Timestep ts) = 0;
	virtual void OnImGuiRender    () = 0;
	virtual void ImGuiMenuOptions () = 0;

	virtual void OnSquareShaderReload  () {};
protected:
	void OnImGuiSqureShaderSource ()
	{
		ImVec2 contentRegion = ImGui::GetContentRegionAvail ();
		contentRegion.y /= 2.0f;
		ImVec2 textbox_contentRegion (contentRegion.x, contentRegion.y - 60);
		ImVec2 button_contentRegion (contentRegion.x, 60);

		ImGui::Text ("Vertex SRC:");
		ImGui::InputTextMultiline ("Vert SRC", m_SquareShaderTXT_vert.raw_data (), m_SquareShaderTXT_vert.size (), textbox_contentRegion
								   , ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_AllowTabInput, buff_resize_callback, &m_SquareShaderTXT_vert);

		ImGui::Separator ();

		ImGui::Text ("Fragment SRC:");
		ImGui::InputTextMultiline ("Frag SRC", m_SquareShaderTXT_frag.raw_data (), m_SquareShaderTXT_frag.size (), textbox_contentRegion
								   , ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_AllowTabInput, buff_resize_callback, &m_SquareShaderTXT_frag);

		if (ImGui::Button ("Reload Square Shader", button_contentRegion))
			ReloadSquareShader ();
	}
	void ReloadSquareShader ()
	{
		std::optional<GLuint> shader_program = Helper::SHADER::CreateProgram (m_SquareShaderTXT_vert.data (), GL_VERTEX_SHADER, m_SquareShaderTXT_frag.data (), GL_FRAGMENT_SHADER);
		if (shader_program.has_value ()) {
			DeleteSquareShader ();
			{// check whether last bound program is this, if yes then update it
				// TODO: fix me
				//GLuint last_program;
				//glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *)&last_program);
				//if (m_SquareShaderProgID == last_program)
					glUseProgram (shader_program.value ());
			}
			m_SquareShaderProgID = shader_program.value ();
			OnSquareShaderReload ();
		}
	}
	void DeleteSquareShader ()
	{
		if (m_SquareShaderProgID) {
			glDeleteProgram (m_SquareShaderProgID);
			m_SquareShaderProgID = 0;
		}
	}

	//////////////
	// Structures
protected:
	struct Buffer
	{
	public:
		static Buffer Create (const char *default_data, const uint32_t min_size = 1024)
		{
			uint32_t size_str = 0;
			while (default_data[size_str] != '\0')
				size_str++;

			const uint32_t new_size_str = (size_str > min_size ? size_str + 100 : min_size);

			char *data = new char[new_size_str];
			size_str++;
			
			memcpy_s (data, new_size_str, default_data, size_str);
			data[size_str] = '\0';

			return Buffer (data, new_size_str);
		}
		Buffer (const uint32_t size = 1537)
			:_size (size)
		{
			_data = new char[size];
			_data[0] = '\0';
		}
		void resize (uint32_t new_size, uint32_t insert_gap_at = 0, uint32_t gap_size = 0)
		{
			new_size++; // 1-more
			if (new_size > _size) {
				char *data = new char[new_size];

				if (insert_gap_at > 0 && gap_size > 0 && insert_gap_at + gap_size < new_size)
				{
					memcpy_s (data + insert_gap_at + gap_size, new_size - insert_gap_at - gap_size, _data + insert_gap_at, _size - insert_gap_at);
					memcpy_s (data, new_size, _data, insert_gap_at);
					for (uint32_t i = 0; i < gap_size; i++)
						data[insert_gap_at + i] = ' ';
				}
				else memcpy_s (data, new_size, _data, _size);
				delete[] _data;
				_data = data;
				_size = new_size;
				_data[new_size - 1] = '\0';
			}
		}
		~Buffer ()
		{
			delete[] _data;
			_data = 0;
		}
		const char *data () const { return _data; }
		char *raw_data () { return _data; }
		const uint32_t size () const { return _size > 0 ? _size - 1 : 0; }
	private:
		Buffer (char *data, const uint32_t size = 1537)
			:_data (data), _size (size)
		{}
	private:
		char *_data;
		uint32_t _size;
	};
	//////////////
	// Variables
protected:
	static const char *s_default_sqr_shader_vert;
	static const char *s_default_sqr_shader_frag;
	GLuint m_SquareShaderProgID = 0;

	Buffer m_SquareShaderTXT_vert, m_SquareShaderTXT_frag;
private:
	static int buff_resize_callback (ImGuiInputTextCallbackData *data)
	{
		if (data->EventFlag = ImGuiInputTextFlags_CallbackResize)
		{
			Buffer* buff = (Buffer *)data->UserData;
			buff->resize (data->BufTextLen);
			data->Buf = buff->raw_data ();
		}
		return 0;
	}
};
/*
*/