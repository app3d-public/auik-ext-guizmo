#pragma once

#include <amal/matrix.hpp>
#include <array>
#include <auik-ext-guizmo/symbol_export.h>
#include <auik/pipelines.hpp>
#include <auik/widgets/widget.hpp>

namespace auik::guizmo
{
    enum class Operation
    {
        move,
        rotate,
        scale
    };

    struct Appearance
    {
        f32 reach = 108.f;
        f32 shaft_width = 2.5f;
        amal::vec2 arrow_size{15.f, 19.f};
        f32 center_radius = 10.f;
        f32 center_width = 1.5f;
        u32 center_color = 0xffff7248u;
        f32 plane_size = 16.f;
        f32 plane_rounding = 4.f;
        bool operator==(const Appearance &) const = default;
    };

    struct Options
    {
        Operation operation = Operation::move;
        u32 enabled_axes = 7u;
        u32 reversed_handles = 0u; // Reverse these handles relative to the camera-facing orientation.
        Appearance appearance;
        amal::vec3 view_direction{}; // From the camera toward the pivot, or parallel view direction.
        bool show_offscreen = false;
        bool show_guides = false;
    };

    struct Frame
    {
        Operation operation = Operation::move;
        std::array<amal::vec2, 3> axes{}; // projected world axes in logical pixels
        u32 enabled = 7;
        u32 reversed_handles = 0u;
        Appearance appearance;
        u32 guides = 0;
        amal::vec3 view_direction{};
        f32 guide_length = 0;
        amal::vec2 edge_direction{};
        bool operator==(const Frame &) const = default;
    };

    class AUIK_EXT_GUIZMO_EXPORT Manipulator final : public Widget
    {
    public:
        explicit Manipulator(u32 id);
        // The camera matrix maps world coordinates to homogeneous clip coordinates.
        bool project(const amal::mat4 &matrix, const amal::vec3 &pivot, const amal::rect &viewport,
                     const amal::rect &clip_bounds, const Options &options);
        u32 begin_drag(const amal::vec2 &screen, bool drag_background = false);
        // Absolute from press: world translation, rotation in degrees, or scale factors.
        bool drag_delta(const amal::vec2 &screen, amal::vec3 &result) const;
        void end_drag();
        Operation operation() const { return _frame.operation; }
        // Returns an axis mask, or edge_handle for the offscreen indicator.
        static constexpr u32 edge_handle = 8;
        void set_hover(u32 handle);
        u32 pick(const amal::vec2 &screen) const;
        StyleUpdateFlags update_style() override { return {}; }
        u32 signature() const noexcept override { return 0x8be55c13u; }
        void draw(DrawCtx &ctx) override;
        void reset_draw_records() override;

    private:
        struct Segment
        {
            amal::vec2 a, b;
            f32 width;
            u32 color, handle;
        };
        struct Triangle
        {
            amal::vec2 a, b, c;
            u32 color, handle;
        };
        Frame _frame, _drag_frame;
        f32 _world_radius = 1.f, _drag_world_radius = 1.f;
        amal::vec2 _start{}, _center{};
        u32 _drag_mask = 0, _hover = 0;
        void place(const amal::vec2 &pivot, const Frame &frame);
        acul::vector<Segment> _segments;
        acul::vector<Triangle> _faces;
        u32 hit_handle(const amal::vec2 &screen) const;
        acul::vector<VertexStreamVertex> _vertices;
        acul::vector<VertexStreamIndex> _indices;
        DrawDataID _record{};
        f32 _depth = -1;
        u32 _clip = ~0u;
        bool _dirty = true;
        void build_segments();
    };
} // namespace auik::guizmo
