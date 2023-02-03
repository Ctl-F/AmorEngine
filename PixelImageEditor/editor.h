#pragma once

#include "Core.h"
#include "Common.h"
#include "Graphics.h"
#include "PixelRenderer.h"

#include "tools.h"
#include <functional>

using namespace amor;

constexpr u32 SCALE = 2;

class Tool;

class Dialog {
public:
	Dialog(const std::string& title, const std::string& confirmString,
		std::function<void()> confirmAction,
		const std::string& declineString,
		std::function<void()> declineAction,
		const math::Rect& position,
		bool allowEscapeDecline = true);
	~Dialog();

	// defaults to a file dialog
	virtual void build_content() = 0;

	// returns true if dialog is closed (the respective action will have been executed already)
	bool update(input::Input&);

protected:
	std::string m_title, m_confirm, m_decline;
	std::function<void()> m_confirmAction, m_declineAction;
	math::Rect m_location;
	bool m_allowEscape;
};

class FileDialog : public Dialog {
public:
	FileDialog(const std::string& title, const std::string& confirmString,
		std::function<void()> confirmAction,
		const std::string& declineString,
		std::function<void()> declineAction,
		const math::Rect& position,
		bool allowEscapeDecline = true);

	static constexpr u32 BUFFER_SIZE = 256;

	void build_content() override;

public:
	char m_FileBuffer[BUFFER_SIZE]{ '\0' };
};

class NewDialog : public Dialog {
public:
	NewDialog(const std::string& title, const std::string& confirmString,
		std::function<void()> confirmAction,
		const std::string& declineString,
		std::function<void()> declineAction,
		const math::Rect& position,
		bool allowEscapeDecline = true);
	void build_content() override;

public:
	i32 m_size[2]{ 32, 32 };
	float m_color[3]{ 1.0f, 1.0f, 1.0f };
};

class ImageEditor : public graphics::WindowBase {
public:

	ImageEditor();
	~ImageEditor();

	bool mouse_on_canvas();

	math::Vec3f get_pixel();

	graphics::Texture* get_tex();
	inline graphics::Texture* get_mask() { return m_Mask; }

	void push_undo_step();
	void undo();
	void redo();

	bool in_mask(i32 x, i32 y);

	float m_PrimaryRgbColor[3]{ 0.0f, 0.0f, 0.0f };
	float m_SecondaryRgbColor[3]{ 1.0f, 1.0f, 1.0f };

protected:
	bool OnUserInit() override;
	bool OnUserUpdate(double delta) override;
	void OnUserRender(graphics::RendererBase* _) override;

	void OnUserDeinit() override;

	void draw_tex_center_zoomed();

	void draw_grid();

	void set_tool(u32 tool);

public:
	math::Rect calculate_image_location();
	inline i32 get_zoom() { return m_Zoom; }
	inline bool& show_mask_setting() { return m_showMask; }
private:
	graphics::PixelRenderer m_Renderer;
	float m_EditorColor[3];
	std::vector<Tool*> m_Tools;
	u64 m_CurrentTool;
	graphics::Texture* m_Texture = nullptr;
	graphics::Texture* m_Mask = nullptr;
	bool m_showGrid = true;
	bool m_showMask = false;
	bool m_tileDraw = false;
	i32 m_Zoom = 1;

	input::ActionsManager m_actions;

	std::vector<graphics::Color*> m_undoSteps;
	i32 m_undoStep = 0;

	bool m_isSaved = false;
	Dialog *m_dialog = nullptr;
};
