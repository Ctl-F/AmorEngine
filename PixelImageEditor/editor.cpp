#pragma once
#include "editor.h"
#include "tools.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

ImageEditor::ImageEditor() :
	graphics::WindowBase(&m_Renderer, "IMGui Renderer", graphics::Resolution{ 640, 480, SCALE, SCALE }),
	m_Renderer{ 640, 480, SCALE, SCALE } {

	m_Tools.push_back(new Brush());
	m_Tools.push_back(new EyeDropper());
	m_Tools.push_back(new FloodFill());
	m_Tools.push_back(new Blend());

	m_CurrentTool = 0;
	m_Texture = new graphics::Texture{ 32, 32 };

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
}

bool ImageEditor::mouse_on_canvas() {
	math::Vec3f center = m_Size.size() / 2.0f / SCALE;
	float texWidth = m_Texture->width() * m_Zoom;
	float texHeight = m_Texture->height() * m_Zoom;

	center.x -= texWidth / 2;
	center.y -= texHeight / 2;

	math::Rect canvas{ (i32)center.x, (i32)center.y, (i32)texWidth, (i32)texHeight };
	return canvas.contains(m_Input->mouse_position());
}

math::Vec3f ImageEditor::get_pixel() {
	math::Vec3f center = m_Size.size() / 2.0f / SCALE;
	math::Vec3f mouse = m_Input->mouse_position() / SCALE;

	float texWidth = m_Texture->width() * m_Zoom;
	float texHeight = m_Texture->height() * m_Zoom;

	center.x -= texWidth / 2;
	center.y -= texHeight / 2;

	math::Rect canvas{ (i32)center.x, (i32)center.y, (i32)texWidth, (i32)texHeight };

	if (canvas.contains(mouse)) {
		return ((mouse - canvas.origin()) / m_Zoom).floor();
	}
	else {
		return math::Vec3f{ 0, 0, -1 };
	}
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
	ImGui::End();

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
	}
	else {
		m_Renderer.BlitUpscaled((i32)center.x, (i32)center.y, *m_Texture, m_Zoom, m_Zoom);
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

void Brush::update(ImageEditor& editor, input::Input& input) {
	math::Vec3f cell = editor.get_pixel();
	graphics::PrimitiveContext2D ctx = editor.get_tex()->GetContext();
	graphics::Color color;

	if (cell.z == 0) {
		if (input.mouse_check_pressed(input::MouseButton::Left) && (action_detail == input::MouseButton::FIMKEY || action_detail == input::MouseButton::Left)) {
			if (input.mouse_check_just_pressed(input::MouseButton::Left)) {
				action_just_started = false;
				action_detail = input::MouseButton::Left;
			}

			color = graphics::Color::from_rgb_vec({ editor.m_PrimaryRgbColor[0],  editor.m_PrimaryRgbColor[1],  editor.m_PrimaryRgbColor[2] });
			
			ctx.Draw((i32)cell.x, (i32)cell.y, color);
		}
		else if (input.mouse_check_pressed(input::MouseButton::Right) && (action_detail == input::MouseButton::FIMKEY || action_detail == input::MouseButton::Right)) {
			if (input.mouse_check_just_pressed(input::MouseButton::Right)) {
				action_just_started = false;
				action_detail = input::MouseButton::Right;
			}

			color = graphics::Color::from_rgb_vec({ editor.m_SecondaryRgbColor[0],  editor.m_SecondaryRgbColor[1],  editor.m_SecondaryRgbColor[2] });

			ctx.Draw((i32)cell.x, (i32)cell.y, color);
		}
	}

	if (input.mouse_check_just_released(action_detail)) {
		action_just_started = true;
		editor.push_undo_step();
	}
}

void Brush::render_tool_options(ImageEditor& editor) {
	ImGui::Begin("Brush");

	ImGui::ColorPicker3("Primary Color", editor.m_PrimaryRgbColor);
	ImGui::ColorPicker3("Secondary Color", editor.m_SecondaryRgbColor);

	ImGui::End();
}

const char* Brush::get_name() {
	return "Brush";
}

void FloodFill::update(ImageEditor& editor, input::Input& input) {

	math::Vec3f cell = editor.get_pixel();
	graphics::PrimitiveContext2D ctx = editor.get_tex()->GetContext();
	
	bool doAction = false;
	graphics::Color color;
	graphics::Color baseColor = ctx.Get((i32)cell.x, (i32)cell.y);
	if (cell.z == 0) {
		if (input.mouse_check_just_released(input::MouseButton::Left)) {
			color = graphics::Color::from_rgb_vec({ editor.m_PrimaryRgbColor[0],  editor.m_PrimaryRgbColor[1],  editor.m_PrimaryRgbColor[2] });
			doAction = true;
		}
		else if (input.mouse_check_just_released(input::MouseButton::Right)) {
			color = graphics::Color::from_rgb_vec({ editor.m_SecondaryRgbColor[0],  editor.m_SecondaryRgbColor[1],  editor.m_SecondaryRgbColor[2] });
			doAction = true;
		}
	}

	if (doAction) {
		editor.push_undo_step();
		flood_fill(ctx, (i32)cell.x, (i32)cell.y, color, baseColor);
	}
}

void FloodFill::render_tool_options(ImageEditor& editor) {
	ImGui::Begin("Bucket Fill");

	ImGui::ColorPicker3("Primary Color", editor.m_PrimaryRgbColor);
	ImGui::ColorPicker3("Secondary Color", editor.m_SecondaryRgbColor);

	ImGui::End();
}

const char* FloodFill::get_name() {
	return "Bucket Fill";
}

void FloodFill::flood_fill(graphics::PrimitiveContext2D& ctx, i32 x, i32 y, const graphics::Color& color, const graphics::Color& baseColor)
{
	if (color == baseColor) return;
	if (ctx.Get(x, y) != baseColor) return;

	ctx.Draw(x, y, color);

	if (x - 1 >= 0) flood_fill(ctx, x - 1, y, color, baseColor);
	if (x + 1 < ctx.width()) flood_fill(ctx, x + 1, y, color, baseColor);
	if (y - 1 >= 0) flood_fill(ctx, x, y - 1, color, baseColor);
	if (y + 1 < ctx.height()) flood_fill(ctx, x, y + 1, color, baseColor);
}
