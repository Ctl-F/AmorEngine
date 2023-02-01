#pragma once
#include "editor.h"
#include "tools.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


// TODO: Fix Undo Steps
ImageEditor::ImageEditor() :
	graphics::WindowBase(&m_Renderer, "IMGui Renderer", graphics::Resolution{ 640, 480, SCALE, SCALE }),
	m_Renderer{ 640, 480, SCALE, SCALE } {

	m_Tools.push_back(new Brush());
	m_Tools.push_back(new EyeDropper());
	m_Tools.push_back(new FloodFill());
	m_Tools.push_back(new Blend());
	m_Tools.push_back(new MaskBrush());

	m_CurrentTool = 0;
	m_Texture = new graphics::Texture{ 32, 32 };
	m_Mask = new graphics::Texture{ 32, 32 };

	for (u32 i = 0; i < 32 * 32; i++) {
		m_Mask->data()[i] = {255, 255, 255, 64};
	}

	m_Texture->GetContext().Clear({ 255, 255, 255, 255 });

}
ImageEditor::~ImageEditor() {
	for (u64 i = 0; i < m_Tools.size(); i++) {
		delete m_Tools[i];
	}

	for (u64 i = 0; i < m_undoSteps.size(); i++) {
		delete[] m_undoSteps[i];
	}

	if (m_Texture != nullptr) {
		delete m_Texture;
	}
	if (m_Mask != nullptr) {
		delete m_Mask;
	}
}

math::Rect ImageEditor::calculate_image_location() {
	i32 texWidth = m_Texture->width() * m_Zoom;
	i32 texHeight = m_Texture->height() * m_Zoom;
	i32 originX = (m_Size.width / (2 * SCALE)) - (texWidth / 2);
	i32 originY = (m_Size.height / (2 * SCALE)) - (texHeight / 2);
	return { originX, originY, texWidth, texHeight };
}

bool ImageEditor::mouse_on_canvas() {
	math::Rect canvas = calculate_image_location();
	return canvas.contains(m_Input->mouse_position());
}

math::Vec3f ImageEditor::get_pixel() {
	math::Vec3f mouse = m_Input->mouse_position() / SCALE;
	math::Rect canvas = calculate_image_location();

	if (canvas.contains(mouse)) {
		return ((mouse - canvas.origin()) / (float)m_Zoom).floor();
	}
	else {
		return math::Vec3f{ 0, 0, -1 };
	}
}

bool ImageEditor::in_mask(i32 x, i32 y) {
	if (x < 0 || x >= m_Texture->width() || y < 0 || y >= m_Texture->height()) {
		return false;
	}
	return m_Mask->data()[x + y * m_Texture->width()].r == 255;
}

void ImageEditor::push_undo_step() {
	logging::GetInstance()->info("Push undo step");
	if (m_undoStep != m_undoSteps.size() - 1 && m_undoSteps.size() > 0) {
		for (u32 i = m_undoSteps.size() - 1; i > m_undoStep; i--) {
			delete[] m_undoSteps[i];
			m_undoSteps.erase(m_undoSteps.begin() + i);
		}
		logging::GetInstance()->info("Erasing future");
	}


	graphics::Color* frame = new graphics::Color[m_Texture->width() * m_Texture->height()];
	std::copy(m_Texture->data(), m_Texture->data() + (m_Texture->width() * m_Texture->height()), frame);

	m_undoSteps.push_back(frame);

	if (m_undoSteps.size() > 1) {
		m_undoStep++;
	}
}

void copy_buffer(graphics::Color* source, graphics::Color* dest, u64 size) {

	std::copy(source, source + size, dest);

}

void ImageEditor::undo() {

	logging::GetInstance()->info(std::string("Undo[") + std::to_string(m_undoStep - 1) + "/" + std::to_string(m_undoSteps.size()) + "]");

	if (m_undoStep-1 > 0 || (m_undoSteps.size() == 1 && m_undoStep == 0)) {

		if (m_undoSteps.size() > 1) {
			m_undoStep--;
		}

		graphics::Color* data = m_Texture->data();
		u64 size = m_Texture->width() * m_Texture->height();

		copy_buffer(m_undoSteps[m_undoStep], data, size);
	}
}
void ImageEditor::redo() {
	logging::GetInstance()->info(std::string("Redo[") + std::to_string(m_undoStep) + "/" + std::to_string(m_undoSteps.size()) + "]");


	if (m_undoStep + 1 < m_undoSteps.size()) {

		graphics::Color* data = m_Texture->data();
		u64 size = m_Texture->width() * m_Texture->height();

		copy_buffer(m_undoSteps[m_undoStep], data, size);

		m_undoStep++;
	}
}

graphics::Texture* ImageEditor::get_tex() {
	return m_Texture;
}

bool ImageEditor::OnUserInit() {

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(this->internal_ptr(), true);
	ImGui_ImplOpenGL3_Init("#version 330 core");


	m_Renderer.SetPostRenderCallback([](graphics::WindowBase* window, graphics::PixelRenderer*) {
		ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		window->refresh_context();
	}
		});


	push_undo_step();

	return true;
}
bool ImageEditor::OnUserUpdate(double delta) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if (m_Input->key_check_just_pressed(input::Key::Z) && m_Input->key_check_pressed(input::Key::LeftControl)) {
		if (m_Input->key_check_pressed(input::Key::LeftShift)) {
			redo();
		}
		else {
			undo();
		}
	}

	m_Tools[m_CurrentTool]->update(*this, *this->input());

	if (m_Input->mouse_wheel() != 0) {
		m_Zoom += m_Input->mouse_wheel();
		if (m_Zoom < 1) m_Zoom = 1;
		if (m_Zoom > 10) m_Zoom = 10;
	}

	if (m_Input->key_check_just_pressed(input::Key::F)) {
		m_CurrentTool = 2;
	}
	else if (m_Input->key_check_just_pressed(input::Key::P)) {
		m_CurrentTool = 0;
	}
	else if (m_Input->key_check_just_pressed(input::Key::I)) {
		m_CurrentTool = 1;
	}
	else if (m_Input->key_check_just_pressed(input::Key::B)) {
		m_CurrentTool = 3;
	}

	return true;
}
void ImageEditor::OnUserRender(graphics::RendererBase* _) {
	m_Renderer.Clear({ 0, 0, 0, 0 });

	draw_tex_center_zoomed();

	if (m_Zoom > 2 && m_showGrid) {
		draw_grid();
	}

	ImGui::Begin("Tools");
	for (u32 i = 0; i < m_Tools.size(); i++) {
		if (ImGui::Button(m_Tools[i]->get_name())) {
			m_CurrentTool = i;
		}
	}
	ImGui::Checkbox("Show Grid", &m_showGrid);
	ImGui::Checkbox("Show Mask", &m_showMask);
	ImGui::Checkbox("Tile Draw", &m_tileDraw);
	ImGui::End();

	std::string zoom = std::to_string(m_Zoom * 100) + "%";
	m_Renderer.DrawText(m_Size.width / 2 - 100, m_Size.height / 2 - 20, zoom, graphics::PixelFont::font_default());

	m_Renderer.Blit(m_Size.width / 2 - m_Texture->width() - 10, 10, *m_Texture);


	m_Tools[m_CurrentTool]->render_tool_options(*this);

}

void ImageEditor::OnUserDeinit() {

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void ImageEditor::draw_tex_center_zoomed() {
	math::Vec3f center = m_Size.size() / 2.0f / SCALE;
	float texWidth = m_Texture->width() * m_Zoom;
	float texHeight = m_Texture->height() * m_Zoom;

	center.x -= texWidth / 2;
	center.y -= texHeight / 2;

	if (m_Zoom == 1) {
		m_Renderer.Blit((i32)center.x, (i32)center.y, *m_Texture);

		if (m_showMask) {
			m_Renderer.SetBlending(graphics::BlendMode::Normal);
			m_Renderer.Blit((i32)center.x, (i32)center.y, *m_Mask);
			m_Renderer.SetBlending(graphics::BlendMode::None);
		}

		if (m_tileDraw) {
			m_Renderer.Blit((i32)center.x - texWidth, (i32)center.y - texHeight, *m_Texture);
			m_Renderer.Blit((i32)center.x , (i32)center.y - texHeight, *m_Texture);
			m_Renderer.Blit((i32)center.x + texWidth, (i32)center.y - texHeight, *m_Texture);
			m_Renderer.Blit((i32)center.x - texWidth, (i32)center.y, *m_Texture);
			m_Renderer.Blit((i32)center.x + texWidth, (i32)center.y, *m_Texture);
			m_Renderer.Blit((i32)center.x - texWidth, (i32)center.y + texHeight, *m_Texture);
			m_Renderer.Blit((i32)center.x, (i32)center.y + texHeight, *m_Texture);
			m_Renderer.Blit((i32)center.x + texWidth, (i32)center.y + texHeight, *m_Texture);
		}
	}
	else {
		m_Renderer.BlitUpscaled((i32)center.x, (i32)center.y, *m_Texture, m_Zoom, m_Zoom);
		if (m_showMask) {
			m_Renderer.SetBlending(graphics::BlendMode::Normal);
			m_Renderer.BlitUpscaled((i32)center.x, (i32)center.y, *m_Mask, m_Zoom, m_Zoom);
			m_Renderer.SetBlending(graphics::BlendMode::None);
		}

		if (m_tileDraw) {
			m_Renderer.BlitUpscaled((i32)center.x - texWidth, (i32)center.y - texHeight, *m_Texture, m_Zoom, m_Zoom);
			m_Renderer.BlitUpscaled((i32)center.x, (i32)center.y - texHeight, *m_Texture, m_Zoom, m_Zoom);
			m_Renderer.BlitUpscaled((i32)center.x + texWidth, (i32)center.y - texHeight, *m_Texture, m_Zoom, m_Zoom);
			m_Renderer.BlitUpscaled((i32)center.x - texWidth, (i32)center.y, *m_Texture, m_Zoom, m_Zoom);
			m_Renderer.BlitUpscaled((i32)center.x + texWidth, (i32)center.y, *m_Texture, m_Zoom, m_Zoom);
			m_Renderer.BlitUpscaled((i32)center.x - texWidth, (i32)center.y + texHeight, *m_Texture, m_Zoom, m_Zoom);
			m_Renderer.BlitUpscaled((i32)center.x, (i32)center.y + texHeight, *m_Texture, m_Zoom, m_Zoom);
			m_Renderer.BlitUpscaled((i32)center.x + texWidth, (i32)center.y + texHeight, *m_Texture, m_Zoom, m_Zoom);
		}
	}
}

void ImageEditor::draw_grid() {
	math::Vec3f center = m_Size.size() / 2.0f / SCALE;
	float texWidth = m_Texture->width() * m_Zoom;
	float texHeight = m_Texture->height() * m_Zoom;

	center.x -= texWidth / 2;
	center.y -= texHeight / 2;

	m_Renderer.SetBlending(graphics::BlendMode::Normal);

	const graphics::Color color{ 0, 255, 255, 255 };

	m_Renderer.DrawRect((i32)center.x, (i32)center.y, (i32)texWidth, (i32)texHeight, color);

	for (int i = center.x; i < center.x + texWidth; i += m_Zoom) {
		m_Renderer.DrawLine(i, center.y, i, center.y + texHeight, color);
	}
	for (int j = center.y; j < center.y + texHeight; j += m_Zoom) {
		m_Renderer.DrawLine(center.x, j, center.x + texWidth, j, color);
	}

	m_Renderer.SetBlending(graphics::BlendMode::None);
}

