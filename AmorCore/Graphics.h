/*
	Main Graphics Core, Note that this doesn't include by default any renderers those must either be included or defined separately
	Classes included in this file:
		Color -> 0-255 rgba format color struct
		Resolution -> Struct to represent a given spatial size with a resolution (width/height pixelwidth/height)
		PrimitiveContext2D -> Software Rasterizer for Primitives Shapes
		Texture -> A Bitmap image
		WindowBase -> A Base class for window creation
		RendererBase -> A Base class for renderer implementation

	A quick example of what a startup project might look like (using the PixelRenderer as the renderer for the example) is as follows:


	#include "Core.h"
	#include "Common.h"
	#include "Graphics.h"

	#define DEFINE_RENDERER_PIXEL
	#include "PixelRenderer.h"

	#include <cstdlib>

	class MyWindow : public amor::graphics::WindowBase {
	public:
		MyWindow(const std::string& title, int width, int height) :
				m_Renderer(width, height, 1, 1),
				WindowBase(&m_Renderer, title, width, height) {

		}

	protected:
		void OnUserRender(amor::graphics::RendererBase* _) override {
			const auto& size = this->size();

			for(int i=0; i<size.width; i++){
				for(int j=0; j<size.height; j++){
					byte r, g, b;
					r = rand() % 256;
					g = rand() % 256;
					b = rand() % 256;

					m_Renderer.Draw(i, j, { r, g, b, 255 });
				}
			}
		}

	private:
		amor::graphics::PixelRenderer m_Renderer;
	};


	int main(int argc, char** argv){
		MyWindow window("hello world", 640, 480);
		window.show();

		return 0;
	}

*/


#pragma once
#include "Common.h"
#include "Core.h"
#include "Util.h"

struct GLFWwindow;
struct GLFWmonitor;

namespace amor {

	namespace input {
		class Input;
	}


	namespace graphics {
		class RendererBase;
		class Texture;

		struct Color {
			byte r, g, b, a;

			inline math::Vec3f to_rgb_vec() const {
				return { r / 255.0, g / 255.0, b / 255.0 };
			}
			static Color from_rgb_vec(const math::Vec3f& vec);

			inline bool operator==(const Color& o) {
				return r == o.r && g == o.g && b == o.b && a == o.a;
			}
		};

		struct Resolution {
			u32 width, height, pixelWidth, pixelHeight;

			Resolution(u32 w, u32 h);
			Resolution(u32 w, u32 h, u32 px, u32 py);
		};

		enum class BlendMode {
			None = 0,
			Normal,
		};

		class PrimitiveContext2D {
			friend void Plot8(PrimitiveContext2D*, i32, i32, i32, i32, const Color&);
		public:
			PrimitiveContext2D(u32 width, u32 height, Color* buffer);
			~PrimitiveContext2D();

			void Clear(const Color& col);
			void FillRect(i32 x, i32 y, i32 width, i32 height, const Color& color);
			void FillCircle(i32 x, i32 y, i32 radius, const Color& color);
			void DrawRect(i32 x, i32 y, i32 width, i32 height, const Color& color);
			void DrawCircle(i32 x, i32 y, i32 radius, const Color& color);
			void DrawLine(i32 x1, i32 y1, i32 x2, i32 y2, const Color& color);
			void Draw(i32 x, i32 y, const Color& col);
			void Blit(i32 x, i32 y, const Texture& tex);
			void BlitUpscaled(i32 x, i32 y, const Texture& tex, i32 scaleX, i32 scaleY);

			void SetBlending(BlendMode mode);

			Color Get(i32 x, i32 y);

			inline Color* Data() { return m_Pixels; }
			inline u32 width() const { return m_Width; }
			inline u32 height() const { return m_Height; }

		private:
			// draw blended
			void DrawB(i32 x, i32 y, const Color& color);

		private:
			u32 Index(i32 x, i32 y);
			void Coordinate(u32 index, i32& x, i32& y);

		private:
			Color Blend(const Color& src, const Color& dest);

		protected:
			Color* m_Pixels;

		private:
			u32 m_Width, m_Height;
			u32 m_BufferLength;
			
			BlendMode m_BlendMode = BlendMode::None;
			Color(amor::graphics::PrimitiveContext2D::*m_BlendFunc)(const Color&, const Color&);
			void(amor::graphics::PrimitiveContext2D::*DrawI)(i32, i32, const Color&) = &PrimitiveContext2D::Draw;
		};


		class Texture {
		public:
			Texture();
			Texture(u32 width, u32 height);
			Texture(const char* path);
			~Texture();

			PrimitiveContext2D GetContext();

			u32 width() const;
			u32 height() const;
			Color* data() const;

			void save(const char* filename);
			void load(const char* filename);

		private:
			bool m_ImageLoaded;
			u32 m_Width, m_Height;
			Color* m_Pixels;
		};

		// Base class for window creation, note that technically this is a valid stand alone window
		// however the only interaction by default that is implemented is the closing. This is fine on its own
		// for testing renderer implementation or even for some instances of static drawing but when any actual logic
		// is required then you'll have to implement a base class and the given virtual methods:
		// OnUserInit
		// OnUserDeinit
		// OnUserUpdate
		// OnUserRender
		class WindowBase {
		public:
			static constexpr i32 PRIMARY_MONITOR = -1;

			WindowBase(RendererBase* renderer, const std::string& title, u32 width, u32 height);

			// for lazy resolution caclulation. Sets widow size to resolution * pixelSize;
			WindowBase(RendererBase* renderer, const std::string& title, const Resolution& res);
			~WindowBase();

			void show();

			i32 get_display_count() const;
			void center_on_display(i32 displayNo = PRIMARY_MONITOR);
			void fullscreen_on_display(bool fullscreen, i32 displayNo = PRIMARY_MONITOR);

			double fps() const;

			const amor::math::Rect& size() const;
			GLFWwindow* internal_ptr() const;
			input::Input* input() const;
		protected:
			virtual bool OnUserInit();
			virtual void OnUserDeinit();
			virtual bool OnUserUpdate(double);
			virtual void OnUserRender(RendererBase*);

		protected:
			bool m_CloseOnExit = true;
			std::string m_Title;
			amor::math::Rect m_Size;
			input::Input* m_Input;

		protected:
			void refresh_window_title();

		private:
			void main_loop();

		private:
			GLFWmonitor* select_monitor(i32 monitor) const;

		private:
			GLFWwindow* m_WindowHandle;
			RendererBase* m_RendererHandle;
			util::Timer* m_Timer, *m_FpsTimer;
			double m_Fps;
			
		};

		// base class for all renderers. This allows us to support a variety of renderers and rendering apis without having to change
		// the rest of the engine just to change the renderer. I'm projecting to include 3 opt-in renderer implementations with the
		// library that will be single-header implementations for the PixelRenderer, Renderer2D, and Renderer3D. the PixelRenderer will
		// implement a software rasterizer and a frame buffer, and will support customizable resolution. It will be the least-efficient
		// (by nature) of the renderers, but you should still be able to do some cool stuff with it. The renderer2D will be an actual
		// Mesh2D/Polygonal renderer that will enable easy sprite transformations, shader implemntation and will be a lot more flexible 
		// as far as what you can do with it. And of course Renderer3D will be similar but in the 3rd dimension for rendering 3d scenes.
		// the opt-in renderers will follow the single-header style where you define a <RendererName>_DEFINITION macro. This way the renderer
		// code only exists if you are using it. And if you decide to implement your own, or are using Renderer3D for example, then the
		// code for PixelRenderer and Renderer2D won't be bloating your project
		class RendererBase {
			friend class WindowBase;
		protected:
			// initialize the pipeline (prewindow). Note that glfwInit and glfwTerminate
			// are both called by the window class. This is because the window class needs to
			// be able to create a window, and to create a window glfwInit needs to have already been
			// called. That said the InitializeGraphicsPipeline will be called before the window has been
			// created allowing you to set any windowHints that you need
			virtual void InitializeGraphicsPipeline() = 0;

			// initialize the pipeline having a window. The GLFWwindow* can be accessed with window->internal_ptr().
			// it's at this stage where you probably will initialize the majority of your pipeline (depending on which
			// rendering api you decide to use)
			virtual void InitializeWindowGraphicsPipeline(WindowBase* window) = 0;
			virtual void DeinitializeGraphicsPipeline(WindowBase* mainWindowHandle) = 0;

			// prepare frame is called before the client OnUserRender method is called. It defaults to clearing the color buffer to
			// black zero-alpha. For 3d renderers this will need to be changed to also include the depth buffer, or any other pre-render
			// setup you may need
			virtual void PrepareFrame(WindowBase* win);

			// render frame is called after OnUserRender. So it's here that you need to make any needed draw calls to render your scene
			virtual void RenderFrame(WindowBase* win) = 0;

			// this defaults to no action being called. But if for any reason you want to perform any operation after the render frame you have the
			// option to do it here. Note that this still happens before the window swaps the buffers. So this is more of an organizational option
			// to separate certain logic from RenderFrame but functionally there is no difference between putting something into RenderFrame
			virtual void PostRenderFrame(WindowBase* win);

		};

	}
}