# AmorEngine
Light weight C++ game framework designed around flexibility and ease of use.

* Open Renderer Interface: While providing several opt-in renderers out of the box, this means that a renderer for any framework (that is compatible with glfw) can be implemented. Need a vulkan implmentation? No problem! Want to implement a metal renderer? You got it!
* Opt-In renderers: The actual renderer implementations are done in single headerfile format so as to minimize bloat. If you want to work with the pixel renderer without worrying about 3d, you only include that renderer and the 3d renderer won't add any additional bloat to your application.
* ~~Unified GUI framework: Design your gui once and have it work on all the out-of-the-box renderers provided without having to change your code each time making guis reusible.~~ Frameworks such as DearImGui are perfectly integratible with the renderers therefore there is no need to reinvent the wheel with a gui framework.


## The Renderers:
### Pixel Renderer
This is the simplest in terms of understanding and using. This renderer provides a frame buffer allowing you to draw pixels and shapes directly to the frame buffer. You also have control of the resolution of your frame buffer allowing you to easily create that "retro" feel. The renderer also allows for aditional shader effects to be implemented in the fragment shader allowing for some neat "whole-frame" effects at the spead of OpenGL.

![PixelFontTool](https://github.com/Ctl-F/AmorEngine/blob/master/content/pixeltool.screenshot.png?raw=true)

### Renderer2D (future development)
A step up from the Pixel Renderer this introduces vertices and 2d meshes into the mix while distancing slightly from the more direct pixel approach. This renderer provides access directly to the shaders and is a more accelerated 2d renderer than the Pixel Renderer.

### Renderer3D (future development)
Similar to the Renderer2D but expanded into the third dimension, this renderer is great for 3d scenes, and provides a framework for 3d models loading and generation, shader access, lighting, and camera control.


## Example
### Pixel Renderer
```cpp
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
        // constructor
    }

protected:
    bool OnUserInit() override {
        // initialization logic
        return true;
    }
  
    bool OnUserUpdate(double delta) override {
      // game logic
      return true;
    }
  
    // rendering logic
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

```


### Renderer2D
Todo


### Renderer3D
Todo

## Dependancies:
Another goal to the AmorEngine is to be dependancy lite. The main dependancies are:
* **GLFW3**  [window and event management]
* **GLAD**   [opengl loader]
* **stbi_image** (header file included) [texture loading from png/jpeg]
* **zlib** [used for compression in custom texture format (texture.save/load)
