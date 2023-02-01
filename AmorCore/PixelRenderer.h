#pragma once

#include "Graphics.h"
#include "ShaderFactory.h"
#include <functional>

#ifdef DEFINE_RENDERER_PIXEL
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif 

namespace amor {
	namespace graphics {


		/*
			A Pixel Shader Effect allows you to specify new functionality to the shader before it is compiled.
			The effect setup string should specify a function that accepts a pixel and outputs a pixel
		*/
		class PixelShaderEffect {
		public:
			virtual void WriteEffectFunction(amor::graphics::utils::opengl::ShaderFactory&) = 0;
			virtual const std::string GetFunctionName() const = 0;
		};

		class PixelTintEffect : public PixelShaderEffect {
		public:
			PixelTintEffect(const Color& color, float strength);
			void WriteEffectFunction(amor::graphics::utils::opengl::ShaderFactory&) override;
			const std::string GetFunctionName() const override;
		private:
			Color m_TintColor;
			float m_TintStrength;
			std::string m_FnName;
		};

		// brightness contrast effect
		class BCSEffect : public PixelShaderEffect {
		public:
			BCSEffect(float, float, float);

			void WriteEffectFunction(amor::graphics::utils::opengl::ShaderFactory&) override;
			const std::string GetFunctionName() const override;

		private:
			float m_Brightness, m_Contrast, m_Saturation;
			std::string m_Name;
		};

		class PixelRenderer : public RendererBase, public PrimitiveContext2D {
		public:
			PixelRenderer(u32 width, u32 height, u32 pixelWidth, u32 pixelHeight);
			PixelRenderer(const Resolution& res);
			~PixelRenderer();

		public:
			void AddEffect(PixelShaderEffect&);
			void RemoveEffect(PixelShaderEffect&);
			void UpdateShader();
			void SetPostRenderCallback(std::function<void(WindowBase*, PixelRenderer*)> callback);
		public:
			void InitializeGraphicsPipeline() override;
			void InitializeWindowGraphicsPipeline(WindowBase* window) override;
			void DeinitializeGraphicsPipeline(WindowBase* window) override;

			void PrepareFrame(WindowBase* win) override;
			void RenderFrame(WindowBase* win) override;
			void PostRenderFrame(WindowBase* win) override;
		private:
			void CompileShader();

		private:
			u32 m_VAO, m_VBO;
			u32 m_ProgramID;
			u32 m_glFrameTexture;
			Color* m_PixelData;
			size_t m_PixelDataLength;
			u32 m_Width, m_Height, m_PixWidth, m_PixHeight;
			u32 m_uniTexture;
			std::vector<PixelShaderEffect*> m_Effects;
			std::function<void(WindowBase*, PixelRenderer*)> m_PostRenderCallback;
		};



#ifdef DEFINE_RENDERER_PIXEL
#undef DEFINE_RENDERER_PIXEL
		using amor::graphics::utils::opengl::ShaderFactory;

		PixelTintEffect::PixelTintEffect(const Color& color, float strength) {
			m_TintColor = color;
			m_TintStrength = strength;
			m_FnName = "custom_tint_" + std::to_string((size_t)(void*)this);
		}

		void PixelTintEffect::WriteEffectFunction(ShaderFactory& shader) {
			float r, g, b, a;
			r = (float)m_TintColor.r / 255.0f;
			g = (float)m_TintColor.g / 255.0f;
			b = (float)m_TintColor.b / 255.0f;
			a = (float)m_TintColor.a / 255.0f;
			
			shader.RawStatement(("vec4 " + m_FnName + "(vec4 pixel){\n").c_str());
			shader.RawStatement(("float strength = " + std::to_string(m_TintStrength) + ";\n").c_str());
			shader.RawStatement(("vec4 tintColor = vec4(" + std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ", " + std::to_string(a) + ") * strength;\n").c_str());
			shader.RawStatement(("vec4 inverseTint = vec4(" + std::to_string(1.0 - r) + ", " + std::to_string(1.0 - g) + ", " + std::to_string(1.0 - b) + ", " + std::to_string(1.0 - a) + ") * (1.0 - strength);\n").c_str());
			shader.RawStatement("return pixel * tintColor + pixel * inverseTint;\n");
			shader.RawStatement("}\n");
		}
		const std::string PixelTintEffect::GetFunctionName() const {
			return m_FnName;
		}


		BCSEffect::BCSEffect(float brightness, float contrast, float saturation) {
			m_Name = "custom_bcs_" + std::to_string((size_t)(void*)this);
			m_Brightness = brightness;
			m_Contrast = contrast;
			m_Saturation = saturation;
		}
		void BCSEffect::WriteEffectFunction(ShaderFactory& shader) {
		
			shader.RawStatement(("vec4 " + m_Name + "(vec4 pixel){\n").c_str());

			shader.RawStatement(("float brightness = " + std::to_string(m_Brightness) + ";\n").c_str());
			shader.RawStatement(("float contrast = " + std::to_string(m_Contrast) + ";\n").c_str());
			shader.RawStatement(("float saturation = " + std::to_string(m_Saturation) + ";\n").c_str());

			shader.RawStatement("mat4 brightnessMat = mat4(1, 0, 0, 0,    0, 1, 0, 0,    0, 0, 1, 0,    brightness, brightness, brightness, 1);\n");
			shader.RawStatement("float c_T = (1.0 - contrast) / 2.0;\n");
			shader.RawStatement("mat4 contrastMat = mat4(contrast, 0, 0, 0,    0, contrast, 0, 0,    0, 0, contrast, 0,    c_T, c_T, c_T, 1);\n");
			shader.RawStatement("vec3 lumi = vec3(0.3086, 0.6094, 0.0820); float oneMinusSat = 1.0 - saturation;\n");
			shader.RawStatement("vec3 red = vec3(lumi.x * oneMinusSat); red += vec3(saturation, 0, 0);\n");
			shader.RawStatement("vec3 green = vec3(lumi.y * oneMinusSat); green += vec3(0, saturation, 0);\n");
			shader.RawStatement("vec3 blue = vec3(lumi.z * oneMinusSat); blue += vec3(0, 0, saturation);\n");
			shader.RawStatement("mat4 saturationMat = mat4( red, 0,   green, 0,   blue, 0,   0, 0, 0, 1);\n");

			shader.RawStatement("return brightnessMat * contrastMat * saturationMat * pixel;\n");

			shader.RawStatement("}\n");
		}
		const std::string BCSEffect::GetFunctionName() const {
			return m_Name;
		}

		PixelRenderer::PixelRenderer(u32 width, u32 height, u32 pixelWidth, u32 pixelHeight) :
			m_Width(width), m_Height(height), m_PixWidth(pixelWidth), m_PixHeight(pixelHeight),
			m_VAO(0), m_VBO(0), m_glFrameTexture(0), m_uniTexture(0),
			PrimitiveContext2D(width, height, nullptr), m_PostRenderCallback{ [](WindowBase*,PixelRenderer*) {} } {

			m_PixelDataLength = width * height;
			m_PixelData = new Color[m_PixelDataLength];

			// PrimitiveContext2D handle to pixel data.
			// this is an unowned pointer, m_PixelData is owned
			m_Pixels = m_PixelData;
		}

		PixelRenderer::PixelRenderer(const Resolution& res) :
			m_Width(res.width), m_Height(res.height), m_PixWidth(res.pixelWidth), m_PixHeight(res.pixelHeight),
			m_VAO(0), m_VBO(0), m_glFrameTexture(0), m_uniTexture(0),
			PrimitiveContext2D(res.width, res.height, nullptr), m_PostRenderCallback{ [](WindowBase*,PixelRenderer*) {} } {

			m_PixelDataLength = res.width * res.height;
			m_PixelData = new Color[m_PixelDataLength];

			// PrimitiveContext2D handle to pixel data.
			// this is an unowned pointer, m_PixelData is owned
			m_Pixels = m_PixelData;
		}

		PixelRenderer::~PixelRenderer() {
			if (m_PixelData != nullptr) {
				delete[] m_PixelData;
				m_PixelData = nullptr;
			}
		}

		void PixelRenderer::AddEffect(PixelShaderEffect& effect) {
			m_Effects.push_back(&effect);
		}
		void PixelRenderer::RemoveEffect(PixelShaderEffect& effect) {
			for (size_t i = 0; i < m_Effects.size(); ++i) {
				if (m_Effects[i] == &effect) {
					m_Effects.erase(m_Effects.begin() + i);
					return;
				}
			}
		}
		void PixelRenderer::UpdateShader() {
			logging::GetInstance()->info("Recompiling shader", "PixelRenderer");
			
			u32 currentProgramID = m_ProgramID;

			try {
				CompileShader();
			}
			catch (std::runtime_error& e) {
				logging::GetInstance()->error("Failed to recompile shader, falling back", "PixelRenderer");
				m_ProgramID = currentProgramID;
				return;
			}

			if (currentProgramID != 0) {
				glDeleteProgram(currentProgramID);
			}
		}

		void PixelRenderer::SetPostRenderCallback(std::function<void(WindowBase*, PixelRenderer*)> callback) {
			m_PostRenderCallback = callback;
		}


		void PixelRenderer::InitializeGraphicsPipeline() {
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
			glfwWindowHint(GLFW_RESIZABLE, false);
		}
		void PixelRenderer::InitializeWindowGraphicsPipeline(WindowBase* window) {
			static float s_vertices[]{
				-1.0f, -1.0f, 0.0f,    0.0f, 1.0f,
				-1.0f,  1.0f, 0.0f,    0.0f, 0.0f,
				 1.0f, -1.0f, 0.0f,    1.0f, 1.0f,

				 1.0f, -1.0f, 0.0f,    1.0f, 1.0f,
				-1.0f,  1.0f, 0.0f,    0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f,    1.0f, 0.0f,
			};

			glfwMakeContextCurrent(window->internal_ptr());
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
				logging::GetInstance()->fail("ProcLoad failed", "GLAD");
				throw std::runtime_error("GLAD load");
			}
			auto& rect = window->size();
			glViewport(rect.x, rect.y, rect.width, rect.height);


			// initialize array objects and buffers
			glGenVertexArrays(1, &m_VAO);
			glBindVertexArray(m_VAO);

			glGenBuffers(1, &m_VBO);
			glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(s_vertices), s_vertices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);

			CompileShader();

			// init frame texture
			glGenTextures(1, &m_glFrameTexture);
			glBindTexture(GL_TEXTURE_2D, m_glFrameTexture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			// initialize 
		}

		void PixelRenderer::CompileShader() {
			using Prim = amor::graphics::utils::opengl::GlPrimitive;

			ShaderFactory pixelShader;

			// vertex shader
			pixelShader.Version("330 core");

			pixelShader.InParam("vPos", Prim::Vec3);
			pixelShader.InParam("vUv", Prim::Vec2);
			pixelShader.OutParam("fragCoord", Prim::Vec2);
			pixelShader.Main();
			pixelShader.RawStatement("gl_Position = vec4(vPos, 1.0);\n");
			pixelShader.RawStatement("fragCoord = vUv;\n");
			pixelShader.EndMain();

			pixelShader.WriteVertex();

			// fragment shader
			pixelShader.Version("330 core");

			pixelShader.OutParam("fragColor", Prim::Vec4);
			pixelShader.InParam("fragCoord", Prim::Vec2, amor::graphics::utils::opengl::NO_LAYOUT);
			pixelShader.UniformParam("framebuffer", Prim::Sampler2D);

			std::string callValue = "texture(framebuffer, fragCoord)";
			for (size_t index = 0; index < m_Effects.size(); ++index) {
				m_Effects[index]->WriteEffectFunction(pixelShader);

				callValue = m_Effects[index]->GetFunctionName() + "(" + callValue + ")";
			}
			callValue = "fragColor = " + callValue + ";\n";

			pixelShader.Main();
			pixelShader.RawStatement(callValue.c_str());
			pixelShader.EndMain();

			pixelShader.WriteFragment();

			m_ProgramID = pixelShader.CompileGlProgram();
			if (m_ProgramID == 0) {
				logging::GetInstance()->fail("Failed to compile default shader", "PixelRenderer");
				throw std::runtime_error("comiler error");
			}
			logging::GetInstance()->info(std::string("Applied ") + std::to_string(m_Effects.size()) + std::string(" custom effects to the pixel shader"), "PixelRenderer");
			logging::GetInstance()->info("Pixel Shader compiled", "PixelRenderer");

			glUseProgram(m_ProgramID);

			m_uniTexture = glGetUniformLocation(m_ProgramID, "framebuffer");
		}

		void PixelRenderer::DeinitializeGraphicsPipeline(WindowBase* window) {
			glDeleteVertexArrays(1, &m_VAO);
			glDeleteBuffers(1, &m_VBO);

			glDeleteProgram(m_ProgramID);
		}

		void PixelRenderer::PrepareFrame(WindowBase* win) {
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		void PixelRenderer::RenderFrame(WindowBase* win) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)m_Width, (GLsizei)m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_PixelData);
			glGenerateMipmap(GL_TEXTURE_2D);

			glBindTexture(GL_TEXTURE_2D, m_glFrameTexture);
			glUseProgram(m_ProgramID);
			glUniform1i(m_uniTexture, 0);
			glBindVertexArray(m_VAO);

			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		void PixelRenderer::PostRenderFrame(WindowBase* win) {
			m_PostRenderCallback(win, this);
		}

		

#pragma endregion
#endif // DEFINE_RENDERER_PIXEL

	}
}