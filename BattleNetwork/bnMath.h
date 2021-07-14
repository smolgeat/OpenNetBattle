#pragma once

#include <SFML/System.hpp>
#include <cmath>

inline float Hypotenuse(sf::Vector2f a) {
  return std::sqrt(a.x * a.x + a.y * a.y);
}

inline float Distance(sf::Vector2f a, sf::Vector2f b) {
  auto delta = a - b;
  return Hypotenuse(delta);
}

inline sf::Vector2f Floor(sf::Vector2f a) {
  return {
    std::floor(a.x),
    std::floor(a.y)
  };
}

inline sf::Vector3f Floor(sf::Vector3f a) {
  return {
    std::floor(a.x),
    std::floor(a.y),
    std::floor(a.z)
  };
}

inline sf::Vector2f ToVector2f(sf::Vector3f a) {
  return { a.x, a.y };
}
