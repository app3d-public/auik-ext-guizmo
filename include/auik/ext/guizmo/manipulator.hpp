#pragma once

#include <amal/matrix.hpp>
#include <array>
#include <auik-ext-guizmo/symbol_export.h>
#include <auik/pipelines.hpp>
#include <auik/widgets/widget.hpp>
#include <optional>

namespace auik::guizmo
{
    enum class Operation
    {
        move,
        rotate,
        scale
    };

    struct Options
    {
        Operation operation = Operation::move;
        u32 enabled_axes = 7u;
        u32 reversed_handles = 0u;   // Reverse these handles relative to the camera-facing orientation.
        amal::vec3 view_direction{}; // From the camera toward the pivot, or parallel view direction.
        bool show_offscreen = false;
        bool show_guides = false;
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
        StyleUpdateFlags update_style() override;
        u32 signature() const noexcept override { return 0x8be55c13u; }
        void draw(DrawCtx &ctx) override;
        void reset_draw_records() override;

    private:
        struct Palette
        {
            u32 normal = 0u, hover = 0u;
            bool operator==(const Palette &) const = default;
        };

        struct PlaneAppearance
        {
            f32 size = 0.f, rounding = 0.f, border_width = 0.f;
            Palette fill{}, border{};
            bool operator==(const PlaneAppearance &) const = default;
        };

        struct Appearance
        {
            f32 reach = 0.f, shaft_width = 0.f;
            amal::vec2 arrow_size{};
            std::array<PlaneAppearance, 3> planes{};
            PlaneAppearance locked_plane{};
            std::array<Palette, 3> axes{};
            Palette locked{};
            f32 guide_width = 0.f;
            u32 guide_alpha = 0u;
            f32 rotation_width = 0.f, scale_size = 0.f;
            amal::vec2 edge_size{};
            f32 edge_padding = 0.f, edge_width = 0.f;
            Palette edge{};
            bool operator==(const Appearance &) const = default;
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

        struct Segment
        {
            amal::vec2 a, b;
            f32 width;
            Palette color;
            u32 handle;
        };
        struct Triangle
        {
            amal::vec2 a, b, c;
            Palette color;
            u32 handle;
        };
        struct Projection
        {
            amal::mat4 matrix;
            amal::vec3 pivot;
            amal::rect viewport, bounds;
            Options options;
        };
        std::optional<Projection> _projection;
        std::optional<Appearance> _appearance;
        Frame _frame, _drag_frame;
        Appearance read_appearance() const;
        bool project_frame(const Projection &projection);
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
