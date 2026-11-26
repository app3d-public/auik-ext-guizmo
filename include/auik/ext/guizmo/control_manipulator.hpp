#pragma once

#include "manipulator.hpp"

namespace auik::guizmo
{
    enum class ControlLineVisibility
    {
        always,
        during_drag
    };

    struct ControlManipulatorOptions
    {
        std::optional<amal::vec3> line_anchor; // Same coordinate space as the control position.
        ControlLineVisibility line_visibility = ControlLineVisibility::always;
    };

    class AUIK_EXT_GUIZMO_EXPORT ControlManipulator final : public detail::ManipulatorWidget
    {
    public:
        // Choose exactly one of the shared axis flags to constrain the drag.
        ControlManipulator(u32 id, ManipulatorCreateFlags flags);
        bool project(const amal::mat4 &matrix, const amal::vec3 &point, const amal::rect &viewport,
                     const amal::rect &bounds, const ControlManipulatorOptions &options = {});
        bool pick(amal::vec2 screen) const;
        void set_hover(bool hovered);
        bool begin_drag(amal::vec2 screen);
        // Displacement from press in control coordinates, measured along the chosen axis.
        bool drag_delta(amal::vec2 screen, f32 &delta);
        void end_drag();
        StyleUpdateFlags update_style() override;
        u32 signature() const noexcept override { return 0x74a56c92u; }

    private:
        const u32 _axis_mask;
        amal::vec2 _projected_axis{}, _press{}, _drag_axis{};
        std::optional<amal::vec2> _anchor;
        ControlLineVisibility _line_visibility = ControlLineVisibility::always;
        f32 _diameter = 0.f, _line_width = 0.f;
        u32 _color = 0u, _hover_color = 0u, _active_color = 0u, _line_color = 0u;
        bool _hovered = false, _pressed = false, _dragging = false;

        void rebuild_geometry();
    };
} // namespace auik::guizmo
