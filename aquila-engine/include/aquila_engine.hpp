#include "render_engine.hpp"
#include "aq_camera.hpp"

namespace aq {

    class AquilaEngine {
    public:
        AquilaEngine();
        ~AquilaEngine();

        void update();
        void draw(AbstractCamera* camera);

        glm::ivec2 get_render_window_size() {return render_engine.get_render_window_size();}

    private:
        RenderEngine render_engine;
    };

}