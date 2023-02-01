#pragma once

#include "Core.h"
#include "Common.h"
#include "Graphics.h"
#include "PixelRenderer.h"

#include "tools.h"

using namespace amor;

constexpr u32 SCALE = 2;

class Tool;

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

	float m_PrimaryRgbColor[3];
	float m_SecondaryRgbColor[3];

protected:
	bool OnUserInit() override;
	bool OnUserUpdate(double delta) override;
	void OnUserRender(graphics::RendererBase* _) override;

	void OnUserDeinit() override;

	void draw_tex_center_zoomed();

	void draw_grid();

private:
	math::Rect calculate_image_location();

private:
	graphics::PixelRenderer m_Renderer;
	float m_EditorColor[3];
	std::vector<Tool*> m_Tools;
	u64 m_CurrentTool;
	graphics::Texture* m_Texture = nullptr;
	graphics::Texture* m_Mask = nullptr;
	bool m_showGrid = true;
	bool m_showMask = true;
	bool m_tileDraw = false;
	i32 m_Zoom = 1;

	std::vector<graphics::Color*> m_undoSteps;
	i32 m_undoStep = 0;
};
