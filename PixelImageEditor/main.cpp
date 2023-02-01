#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Core.h"
#include "Common.h"
#include "Graphics.h"
#include "Input.h"
#include "PixelRenderer.h"


#include "editor.h"
#include "tools.h"

using namespace amor;

int main(int argc, char** argv) {
	ImageEditor window;
	window.show();
	return 0;
}