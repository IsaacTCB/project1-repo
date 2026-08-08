#pragma once

#include <algorithm>
#include <vector>

template <typename T>
class GridMap {
public:
    GridMap() = default;

    GridMap(size_t row, size_t col)
        : m_pitch(row) {
        size_t sz = m_pitch * col;
        m_data.resize(sz);
    }

    [[nodiscard]]
    size_t width() const {
        return m_pitch;
    }

    [[nodiscard]]
    size_t height() const {
        if (m_pitch == 0) {
            return 0;
        }
        return m_data.size() / m_pitch;
    }

    void fill_rect(size_t xpos, size_t ypos, size_t width, size_t height, T value = T{}) {
        /* TODO: check boundaries */

        size_t xstart = std::max(xpos, static_cast<size_t>(0));
        size_t ystart = std::max(ypos, static_cast<size_t>(0));
        size_t xend = std::min(xpos + width, m_pitch);
        size_t yend = std::min(ypos + height, this->height());

        for (size_t y = ystart; y < yend; ++y) {
            for (size_t x = xstart; x < xend; ++x) {
                m_data[(y * m_pitch) + x] = value;
            }
        }
    }

    const T *data() const {
        return m_data.data();
    }

    T *data() {
        return m_data.data();
    }

private:
    size_t m_pitch;
    std::vector<T> m_data;
};
