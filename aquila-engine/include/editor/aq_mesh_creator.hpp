#ifndef EDITOR_AQUILA_MESH_CREATOR_HPP
#define EDITOR_AQUILA_MESH_CREATOR_HPP

#include <memory>

#include "scene/aq_mesh.hpp"

namespace aq::mesh_creator {

    std::shared_ptr<Mesh> create_cube();

    // `yres` is the number of vertical disc (including top and bottom verts)
    // `xres` is the number of vertices in every horizontal disc (excluding top and botton verts)
    // `yres` and `xres` must be >= 3
    std::shared_ptr<Mesh> create_sphere(uint xres, uint yres);

}

#endif