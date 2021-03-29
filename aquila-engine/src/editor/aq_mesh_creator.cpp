#include "editor/aq_mesh_creator.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#define PI (glm::pi<float>())

namespace aq::mesh_creator {

    // Calculates smooth normals
    // The every normal in verts should be all 0
    void calculate_normals(std::vector<Vertex>& verts, const std::vector<Index>& inds, bool assert_normals_point_outside=true) {
        for (auto& vert : verts) assert(vert.normal == glm::vec4(0.0f));
        assert(inds.size() % 3 == 0);

        for (size_t i=0; i<inds.size(); i+=3) {
            glm::vec3 v01 = glm::normalize(glm::vec3(verts[inds[i+1]].position) - glm::vec3(verts[inds[i+0]].position));
            glm::vec3 v02 = glm::normalize(glm::vec3(verts[inds[i+2]].position) - glm::vec3(verts[inds[i+0]].position));

            glm::vec3 v10 = glm::normalize(glm::vec3(verts[inds[i+0]].position) - glm::vec3(verts[inds[i+1]].position));
            glm::vec3 v12 = glm::normalize(glm::vec3(verts[inds[i+2]].position) - glm::vec3(verts[inds[i+1]].position));

            // Triangle angles add up to pi
            float theta[3];
            theta[0] = glm::acos(glm::dot(v01, v02));
            theta[1] = glm::acos(glm::dot(v10, v12));
            theta[2] = PI - theta[0] - theta[1];

            // Winding order should be counter-clockwise
            glm::vec3 face_normal = glm::normalize(glm::cross(v01, v02));

            if (assert_normals_point_outside)
                assert(glm::dot(face_normal, glm::vec3(verts[inds[i+0]].position)) >= 0.0f);

            for (uint j=0; j<3; ++j) {
                // Smooth normal is the weighted average of normals:
                // N = (t0/T)n0 + (t1/T)n1 + ... + (tk/T)nk
                // T = t0 + t1 + ... + tk
                // So N = (t0*n0 + t1*n1 + ... + tk*nk) / (t0 + t1 + ... + tk)
                // The first running sum is stored in vert.norm.xyz
                // The second one is stored in vert.norm.w
                verts[inds[i+j]].normal += glm::vec4(face_normal*theta[j], theta[j]);
            }
        }

        for (auto& vert : verts) {
            if (vert.normal.w != 0.0f) {
                // Calculate N:
                // (t0*n0 + t1*n1 + ... + tk*nk) is stored in vert.norm.xyz
                // T = (t0 + t1 + ... + tk) is stored in vert.norm.w
                // Also normalize N
                vert.normal = glm::vec4(glm::normalize(glm::vec3(vert.normal) / vert.normal.w), 1.0f);
            }
        }
    }

    std::shared_ptr<Mesh> create_cube() {
        static const float left  = -1.0f;
        static const float right =  1.0f;
        static const float top    = -1.0f;
        static const float bottom =  1.0f;
        static const float front = -1.0f;
        static const float back  =  1.0f;

        std::vector<Vertex> verts = {
            // Front Face
            {{left,  top,    front, 1.0f}, {}, {}},
            {{right, top,    front, 1.0f}, {}, {}},
            {{right, bottom, front, 1.0f}, {}, {}},
            {{left,  bottom, front, 1.0f}, {}, {}},
            // Right Face
            {{right, top,    front, 1.0f}, {}, {}},
            {{right, top,    back,  1.0f}, {}, {}},
            {{right, bottom, back,  1.0f}, {}, {}},
            {{right, bottom, front, 1.0f}, {}, {}},
            // Back face
            {{right, top,    back,  1.0f}, {}, {}},
            {{left,  top,    back,  1.0f}, {}, {}},
            {{left,  bottom, back,  1.0f}, {}, {}},
            {{right, bottom, back,  1.0f}, {}, {}},
            // Left Face
            {{left,  top,    back,  1.0f}, {}, {}},
            {{left,  top,    front, 1.0f}, {}, {}},
            {{left,  bottom, front, 1.0f}, {}, {}},
            {{left,  bottom, back,  1.0f}, {}, {}},
            // Top Face
            {{left,  bottom, front, 1.0f}, {}, {}},
            {{right, bottom, front, 1.0f}, {}, {}},
            {{right, bottom, back,  1.0f}, {}, {}},
            {{left,  bottom, back,  1.0f}, {}, {}},
            // Bottom Face
            {{left,  top,    back,  1.0f}, {}, {}},
            {{right, top,    back,  1.0f}, {}, {}},
            {{right, top,    front, 1.0f}, {}, {}},
            {{left,  top,    front, 1.0f}, {}, {}},
        };

        std::vector<Index> inds;
        for (size_t i=0; i<verts.size(); i+=4) {
            inds.push_back(i);
            inds.push_back(i+3);
            inds.push_back(i+2);

            inds.push_back(i+2);
            inds.push_back(i+1);
            inds.push_back(i);
        }

        calculate_normals(verts, inds);

        return std::make_shared<Mesh>("Cube", verts, inds);
    }

    // rho is the radius
    // theta is the polar angle
    // phi is the azimuthal angle
    glm::vec3 spherical_to_cartesian(const float& rho, const float& theta, const float& phi) {
        return rho * glm::vec3(glm::sin(phi)*glm::cos(theta), -glm::cos(phi), glm::sin(phi)*glm::sin(theta));
    }

    std::shared_ptr<Mesh> create_sphere(uint xres, uint yres) {
        assert(xres >= 3); assert(yres >= 3);

        float phi_step = PI / (yres - 1);
        float theta_step = 2*PI / (xres);

        std::vector<Vertex> verts;
        std::vector<Index> inds;

        verts.push_back({{0.0f,-1.0f,0.0f,1.0f}, {}, {}}); // Top vertex

        // Rings
        for (uint i=1; i<=yres-2; ++i) { // horizontal rings
            float phi = i*phi_step;
            for (uint j=0; j<xres; ++j) { // vertices in those rings
                float theta = j*theta_step;
                verts.push_back({glm::vec4(spherical_to_cartesian(1, theta, phi), 1.0f)});
            }
        }

        verts.push_back({{0.0f,1.0f,0.0f,1.0f}, {}, {}}); // Bottom vertex

        // A quicker way of calculating normals for a sphere
        for (auto& vert : verts) vert.normal = vert.position;

        // Connect rings
        for (uint i=0; i<yres-3; ++i) {
            // std::cout << "ring_offset " << ring_offset << std::endl;
            uint ring_offset = i*xres + 1;
            uint next_ring_offset = ring_offset + xres;
            for (uint j=0; j<xres; ++j) {
                uint k = (j+1) % xres;

                inds.push_back(ring_offset+j);
                inds.push_back(next_ring_offset+j);
                inds.push_back(next_ring_offset+k);

                inds.push_back(next_ring_offset+k);
                inds.push_back(ring_offset+k);
                inds.push_back(ring_offset+j);
            }
        }

        // Connect top & bottom vertices to first and last rings
        uint first_ring_offset = 1;
        uint last_ring_offset = verts.size()-1-xres;
        for (uint i=0; i<xres; ++i) {
            uint j = (i+1) % xres;

            inds.push_back(0); // top vert
            inds.push_back(first_ring_offset+i);
            inds.push_back(first_ring_offset+j);

            inds.push_back(verts.size()-1); // bottom vert
            inds.push_back(last_ring_offset+j);
            inds.push_back(last_ring_offset+i);
        }

        return std::make_shared<Mesh>("Sphere", verts, inds);
    }

}