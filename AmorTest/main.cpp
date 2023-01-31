#include "Common.h"
#include "Core.h"

#include "Input.h"
#include "Graphics.h"
#define DEFINE_RENDERER_PIXEL
#include "PixelRenderer.h"

using namespace amor;
using amor::graphics::WindowBase;

class MainWindow : public WindowBase {
public:
	MainWindow(graphics::RendererBase* renderer, const char* string, u32 width, u32 height) :
		m_TestTexture(width, height),
		WindowBase(renderer, string, width, height), m_Effect(0.15f, 1.0f, 1.0f) {
		p0.x = 0;
		p0.y = 0;
		p1.x = 100;
		p1.y = 100;


		m_TestTexture.load("test.pix");

		m_Contrast = 1.0f;
		m_Brightness = 0.15f;
		m_EnableEffect = false;
	}

	inline bool CanClick(const math::Vec3f& point) {
		return (point - (m_Input->mouse_position() /** 0.5*/)).len_squared() <= (8 * 8);
	}

	bool OnUserUpdate(double delta) override {
		auto mouse = m_Input->mouse_position() /** 0.5*/;

		if (selected == nullptr) {
			if (m_Input->mouse_check_pressed(amor::input::MouseButton::Left)) {
				if (CanClick(p0)) {
					selected = &p0;
				}
				else if (CanClick(p1)) {
					selected = &p1;
				}
			}
		}
		else {
			*selected = mouse;
			if (m_Input->mouse_check_released(amor::input::MouseButton::Left)) {
				selected = nullptr;
			}
		}

		return true;
	}

	void OnUserRender(graphics::RendererBase* renderer) override {
		graphics::PixelRenderer* gfx = dynamic_cast<graphics::PixelRenderer*>(renderer);
		if (gfx == nullptr) return;
		
		if (m_Input->key_check_pressed(amor::input::Key::Space) && m_spReleased) {
			m_spReleased = false;
			m_EnableEffect = !m_EnableEffect;

			if (m_EnableEffect) {
				gfx->AddEffect(m_Effect);
			}
			else {
				gfx->RemoveEffect(m_Effect);
			}
			gfx->UpdateShader();
		}

		if (m_Input->key_check_pressed(amor::input::Key::Up) && m_upReleased) {
			m_upReleased = false;
			m_Contrast += 0.1f;

			logging::GetInstance()->info(std::string("contrast set to: ") + std::to_string(m_Contrast));

			m_Effect = { m_Brightness, m_Contrast, 1.0f };
			gfx->UpdateShader();
		}

		if (m_Input->key_check_pressed(amor::input::Key::Down) && m_dnReleased) {
			m_dnReleased = false;
			m_Contrast -= 0.1f;

			logging::GetInstance()->info(std::string("contrast set to: ") + std::to_string(m_Contrast));

			m_Effect = { m_Brightness, m_Contrast, 1.0f };
			gfx->UpdateShader();
		}
		
		if (m_Input->key_check_pressed(amor::input::Key::Left) && m_lfReleased) {
			m_lfReleased = false;
			m_Brightness -= 0.1f;

			logging::GetInstance()->info(std::string("brightness set to: ") + std::to_string(m_Brightness));

			m_Effect = { m_Brightness, m_Contrast, 1.0f };
			gfx->UpdateShader();
		}

		if (m_Input->key_check_pressed(amor::input::Key::Right) && m_rtReleased) {
			m_rtReleased = false;
			m_Brightness += 0.1f;

			logging::GetInstance()->info(std::string("brightness set to: ") + std::to_string(m_Brightness));

			m_Effect = { m_Brightness, m_Contrast, 1.0f };
			gfx->UpdateShader();
		}

		if (m_Input->key_check_released(amor::input::Key::Space)) m_spReleased = true;
		if (m_Input->key_check_released(amor::input::Key::Up)) m_upReleased = true;
		if (m_Input->key_check_released(amor::input::Key::Down)) m_dnReleased = true;
		if (m_Input->key_check_released(amor::input::Key::Left)) m_lfReleased = true;
		if (m_Input->key_check_released(amor::input::Key::Right)) m_rtReleased = true;
		
		
		//gfx->Clear({ 0, 0, 0, 0 });

		gfx->Blit(0, 0, m_TestTexture);

		//gfx->DrawLine(p0.x, p0.y, p1.x, p1.y, { 255, 255, 255, 255 });

		u32 radius = (u32)(p0 - p1).len();

		gfx->FillCircle(p0.x, p0.y, radius, graphics::Color{ 100, 255, 100, 255 });
		gfx->DrawCircle(p0.x, p0.y, radius, graphics::Color{ 200, 0, 200, 255 });

		gfx->DrawCircle(p0.x, p0.y, 8, (&p0 == selected) ? amor::graphics::Color{255, 0, 0, 255} : amor::graphics::Color{ 255, 255, 0, 255 });
		gfx->DrawCircle(p1.x, p1.y, 8, (&p1 == selected) ? amor::graphics::Color{255, 0, 0, 255} : amor::graphics::Color{ 255, 255, 0, 255 });
	
		//gfx->FillCircle(300, 50, 25, graphics::Color{ 100, 255, 100, 255 });
		//gfx->DrawCircle(300, 50, 25, graphics::Color{ 200, 0, 200, 255 });
	}

private:
	math::Vec3f p0, p1;
	math::Vec3f* selected = nullptr;
	graphics::Texture m_TestTexture;


	bool m_spReleased = true;
	bool m_upReleased = true;
	bool m_dnReleased = true;
	bool m_lfReleased = true;
	bool m_rtReleased = true;

	amor::graphics::BCSEffect m_Effect;
	bool m_EnableEffect;
	float m_Contrast;
	float m_Brightness;
};


int main(int argc, char** argv) {
	logging::GetInstance()->OutputMode() = logging::LOG_STDOUT;
	amor::graphics::PixelRenderer mainRenderer(800, 600, 1, 1);

	MainWindow mainWin(&mainRenderer, "Hello World", 800, 600);
	mainWin.show();

	logging::GetInstance()->info("Application exited successfully");
	return 0;
}