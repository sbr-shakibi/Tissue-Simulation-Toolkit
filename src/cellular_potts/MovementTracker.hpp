#ifndef MOVEMENT_TRACKER_HPP
#define MOVEMENT_TRACKER_HPP

#include <deque>
#include <utility>  // std::pair
#include <cmath>    // std::sqrt

class MovementTracker {
public:
    using Coordinate = std::pair<double, double>;

    // Constructor: specify how many steps back to compute the movement vector
    explicit MovementTracker(size_t window_size) : n(window_size) {}

    // Add a new 2D coordinate
    void add_position(double x, double y) {
        if (n == 0) {
            // For backwards compatibility: track only the latest point
            positions.clear();
            positions.emplace_back(x, y);
            return;
        }

        if (positions.size() >= n + 1) {
            positions.pop_front();
        }
        positions.emplace_back(x, y);
    }

    // Get movement vector from time t-n to t
    Coordinate movement_vector() const {
        if (n == 0 || positions.size() <= n) {
            return {0.0, 0.0};  // Not enough data or disabled
        }

        const auto& from = positions.front();
        const auto& to = positions.back();
        return {to.first - from.first, to.second - from.second};
    }

    // Get the magnitude of the movement vector
    double movement_magnitude() const {
        auto [dx, dy] = movement_vector();
        return std::sqrt(dx * dx + dy * dy);
    }

private:
    size_t n;
    std::deque<Coordinate> positions;
};

#endif // MOVEMENT_TRACKER_HPP

