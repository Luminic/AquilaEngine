#ifndef SANDBOX_CAMERA_CONTROLLER_HPP
#define SANDBOX_CAMERA_CONTROLLER_HPP

#include "aq_camera.hpp"

enum class CameraMovementFlags {
    NONE = 0,
    FORWARD = 1,
    BACKWARD = 1 << 1,
    LEFTWARD = 1 << 2,
    RIGHTWARD = 1 << 3,
    UPWARD = 1 << 4,
    DOWNWARD = 1 << 5
};

inline CameraMovementFlags operator |(CameraMovementFlags lhs, CameraMovementFlags rhs) {
    return static_cast<CameraMovementFlags>(
        static_cast<std::underlying_type<CameraMovementFlags>::type>(lhs) |
        static_cast<std::underlying_type<CameraMovementFlags>::type>(rhs)
    );
}

inline void operator |=(CameraMovementFlags& lhs, CameraMovementFlags rhs) { lhs = lhs | rhs; }

inline CameraMovementFlags operator &(CameraMovementFlags lhs, CameraMovementFlags rhs) {
    return static_cast<CameraMovementFlags>(
        static_cast<std::underlying_type<CameraMovementFlags>::type>(lhs) &
        static_cast<std::underlying_type<CameraMovementFlags>::type>(rhs)
    );
}

inline void operator &=(CameraMovementFlags& lhs, CameraMovementFlags rhs) { lhs = lhs & rhs; }

class CameraController {
public:
    CameraController(aq::DefaultCamera* camera);

    void move(CameraMovementFlags movement_flags);
    void rotate(int dx, int dy); // Rotate based off of mouse movement
    
private:
    aq::DefaultCamera* camera;
};

#endif