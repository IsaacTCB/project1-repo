#pragma once

class Viewport {
public:
    float x;
    float y;
    float width;
    float height;

public:
    [[nodiscard]]
    float get_aspect_ratio() const {
        return height / width;
    }
};
