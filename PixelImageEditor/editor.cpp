#pragma once
#include "editor.h"
#include "tools.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


Dialog::Dialog(const std::string& title, const std::string& confirmString,
	std::function<void()> confirmAction,
	const std::string& declineString,
	std::function<void()> declineAction,
	const math::Rect& position,
	bool allowEscapeDecline) : m_title(title), m_confirm(confirmString), m_decline(declineString),
	m_confirmAction(confirmAction), m_declineAction(declineAction),
	m_location(position),
	m_allowEscape(allowEscapeDecline) {

}
Dialog::~Dialog(){

}

FileDialog::FileDialog(const std::string& title, const std::string& confirmString,
	std::function<void()> confirmAction,
	const std::string& declineString,
	std::function<void()> declineAction,
	const math::Rect& position,
	bool allowEscapeDecline) : Dialog(title, confirmString, confirmAction, declineString, declineAction, position, allowEscapeDecline) {

}
void FileDialog::build_content() {
	ImGui::InputText("Filename: ", m_FileBuffer, BUFFER_SIZE);
}

NewDialog::NewDialog(const std::string& title, const std::string& confirmString,
	std::function<void()> confirmAction,
	const std::string& declineString,
	std::function<void()> declineAction,
	const math::Rect& position,
	bool allowEscapeDecline) : Dialog(title, confirmString, confirmAction, declineString, declineAction, position, allowEscapeDecline) {

}
void NewDialog::build_content() {
	ImGui::InputInt2("Size", m_size);
	ImGui::ColorPicker3("Background Color", m_color);
}

bool Dialog::update(input::Input& m_Input) {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

	ImGui::SetNextWindowPos({ (float)m_location.x, (float)m_location.y });
	ImGui::SetNextWindowSize({ (float)m_location.width, (float)m_location.height });

	if (ImGui::Begin(m_title.c_str(), (bool*)nullptr, flags)) {
		bool ret = false;
		build_content();

		if (ImGui::Button(m_confirm.c_str())) {
			m_confirmAction();
			ret = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel") || (m_allowEscape && m_Input.key_check_just_pressed(input::Key::Escape))) {
			m_declineAction();
			ret = true;
		}
		ImGui::End();

		return ret;
	}

	return false;
}


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

	input::Action undo;
	undo.begin_key_combo_option();
	undo.add_key_option(input::Key::Z, input::ActionStateType::JustPressed);
	undo.add_key_option(input::Key::LeftControl);
	undo.end_key_combo_option();

	input::Action redo;
	redo.begin_key_combo_option();
	redo.add_key_option(input::Key::Z, input::ActionStateType::JustPressed);
	redo.add_key_option(input::Key::LeftShift);
	redo.add_key_option(input::Key::LeftControl);
	redo.end_key_combo_option();


	input::Action save;
	save.begin_key_combo_option();
	save.add_key_option(input::Key::S, input::ActionStateType::JustPressed);
	save.add_key_option(input::Key::LeftControl);
	save.end_key_combo_option();

	input::Action load;
	load.begin_key_combo_option();
	load.add_key_option(input::Key::O, input::ActionStateType::JustPressed);
	load.add_key_option(input::Key::LeftControl);
	load.end_key_combo_option();

	input::Action _new;
	_new.begin_key_combo_option();
	_new.add_key_option(input::Key::N, input::ActionStateType::JustPressed);
	_new.add_key_option(input::Key::LeftControl);
	_new.end_key_combo_option();

	m_actions.add_action("undo", undo);
	m_actions.add_action("redo", redo);
	m_actions.add_action("save", save);
	m_actions.add_action("load", load);
	m_actions.add_action("new", _new);
	m_actions.add_action("ch_brush", { { input::Key::P, input::ActionStateType::JustPressed }, { input::Key::K1, input::ActionStateType::JustPressed } });
	m_actions.add_action("ch_eyedropper", { { input::Key::I, input::ActionStateType::JustPressed }, { input::Key::K2, input::ActionStateType::JustPressed } });
	m_actions.add_action("ch_flood", { {input::Key::F, input::ActionStateType::JustPressed }, { input::Key::K3, input::ActionStateType::JustPressed } });
	m_actions.add_action("ch_blend", { {input::Key::B, input::ActionStateType::JustPressed }, { input::Key::K4, input::ActionStateType::JustPressed } });
	m_actions.add_action("ch_mask", { {input::Key::M, input::ActionStateType::JustPressed }, { input::Key::K5, input::ActionStateType::JustPressed } });
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

	m_Tools[m_CurrentTool]->start(*this);

	return true;
}

void ImageEditor::set_tool(u32 tool) {
	m_Tools[m_CurrentTool]->end(*this);
	m_Tools[tool]->start(*this);
	m_CurrentTool = tool;
}

bool ImageEditor::OnUserUpdate(double delta) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	m_actions.update(*m_Input);

	i32 width = 400;
	i32 height = 200;
	i32 center_x = m_Size.width / 2;
	i32 center_y = m_Size.height / 2;

	if (m_actions.is_activated("save")) {
		m_dialog = new FileDialog("Save", "Save", [&]() {
			m_Texture->save(((FileDialog*)m_dialog)->m_FileBuffer);
			}, "Cancel", []() {

			},
			{ center_x, center_y, width, height });
	}
	else if (m_actions.is_activated("load")) {
		m_dialog = new FileDialog("Load", "Load", [&]() {
			m_Texture->load(((FileDialog*)m_dialog)->m_FileBuffer);
			}, "Cancel", []() {

			},
			{ center_x, center_y, width, height });
	}
	else if (m_actions.is_activated("new")) {
		width = 600;
		height = 600;

		m_dialog = new NewDialog("New", "Create", [&]() {

			NewDialog* dialog = (NewDialog*)m_dialog;

			delete m_Texture;
			m_Texture = new graphics::Texture{ (u32)dialog->m_size[0], (u32)dialog->m_size[1]};
			graphics::PrimitiveContext2D ctx = m_Texture->GetContext();

			ctx.Clear(graphics::Color::from_rgb_vec({ dialog->m_color[0], dialog->m_color[1], dialog->m_color[2] }));

			delete m_Mask;
			m_Mask = new graphics::Texture{ (u32)dialog->m_size[0], (u32)dialog->m_size[1] };
			ctx = m_Mask->GetContext();

			ctx.Clear({ 255, 255, 255, 64 });

			}, "Cancel", []() {

			},
			{ center_x, center_y, width, height });

		NewDialog* d = (NewDialog*)m_dialog;
		d->m_size[0] = (i32)m_Texture->width();
		d->m_size[1] = (i32)m_Texture->height();
	}


	if (m_dialog != nullptr) {
		if (m_dialog->update(*m_Input)) {
			delete m_dialog;
			m_dialog = nullptr;
		}
	}
	else {
		if (m_actions.is_activated("redo")) {
			redo();
		}

		if (m_actions.is_activated("undo")) {
			undo();
		}

		m_Tools[m_CurrentTool]->update(*this, *this->input());

		if (m_Input->mouse_wheel() != 0) {
			m_Zoom += m_Input->mouse_wheel();
			if (m_Zoom < 1) m_Zoom = 1;
			if (m_Zoom > 10) m_Zoom = 10;
		}

		//if (m_Input->key_check_just_pressed(input::Key::F)) {
		if (m_actions.is_activated("ch_flood")) {
			set_tool(2);
		}
		else if (m_actions.is_activated("ch_brush")) {
			set_tool(0);
		}
		else if (m_actions.is_activated("ch_eyedropper")) {
			set_tool(1);
		}
		else if (m_actions.is_activated("ch_blend")) {
			set_tool(3);
		}
		else if (m_actions.is_activated("ch_mask")) {
			set_tool(4);
		}
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
			set_tool(i);
		}
	}
	ImGui::Checkbox("Show Grid", &m_showGrid);
	ImGui::Checkbox("Show Mask", &m_showMask);
	ImGui::Checkbox("Tile Draw", &m_tileDraw);
	ImGui::End();

	std::string zoom = std::to_string(m_Zoom * 100) + "%";
	m_Renderer.DrawText(m_Size.width / 2 - 100, m_Size.height / 2 - 20, zoom, graphics::PixelFont::font_default());

	m_Renderer.Blit(m_Size.width / 2 - m_Texture->width() - 10, 10, *m_Texture);

	m_Tools[m_CurrentTool]->render_tool_info(*this, m_Renderer);
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

