#pragma once

#include <cstdint>
#include <glm/vec2.hpp>

constexpr std::uint32_t C_RENDER_TARGET_RESOLUTION_X    = 1200;
constexpr std::uint32_t C_RENDER_TARGET_RESOLUTION_Y    = 800;
const     glm::uvec2    C_RENDER_TARGET_RESOLUTION      = glm::uvec2(C_RENDER_TARGET_RESOLUTION_X, C_RENDER_TARGET_RESOLUTION_Y);
const     glm::vec2     C_RENDER_TARGET_RESOLUTION_FL   = glm::vec2(static_cast<float>(C_RENDER_TARGET_RESOLUTION_X), static_cast<float>(C_RENDER_TARGET_RESOLUTION_Y));

