# Auik Guizmo Extension

`auik-ext-guizmo` is an extension for the [Auik](https://repos.wusikijeronii.me/app3d/auik) library for viewport transform handles and curve editing.
The move, rotate and scale handles follow [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) interaction conventions. Pass the camera matrix, pivot and viewport to `Manipulator::project`. Forward pointer input to `begin_drag`, `drag_delta` and `end_drag`. The host supplies the enabled-axis mask and can enable full-axis guides or the offscreen indicator.

The extension also provides an interactive curve editor with canvas navigation, curve and control point editing, selection, and customizable grid visualization.

[<img src="./screenshots/curve_canvas.png" alt="screenshot" width="350"/>](./screenshots/curve_canvas.png)

## Usage

```cpp
amal::bezier_curve2 curve;
curve.points.push_back({{-1.0f, -0.5f}, {-1.0f, -0.5f}, {-0.7f, -0.5f}});
curve.points.push_back({{ 0.0f,  0.5f}, {-0.3f,  0.2f}, { 0.3f,  0.8f}});
curve.points.push_back({{ 1.0f, -0.5f}, { 0.7f, -0.5f}, { 1.0f, -0.5f}});

auto* curve_widget = auik::guizmo::make_control_curve(curve_id, std::move(curve));
curve_widget->set_view({{-1.0f, -1.0f}, {2.0f, 2.0f}});

auto* grid = auik::guizmo::make_grid(grid_id, AUIK_SIZE_FILL, 10u, amal::axis::x);
auto *canvas = auik::guizmo::make_curve_canvas(canvas_id, grid, curve_widget,
                                              auik::guizmo::CurveCanvasFlagBits::rubber_band,
                                              AUIK_SIZE_FILL
                                              );
window->add_child(canvas);
```

## Building
Auik is distributed as a CMake project and is intended to be integrated as a submodule.

### Supported compilers:
- GNU GCC
- Clang

### Supported OS:
- Linux
- Microsoft Windows

### Bundled submodules
The following dependencies are included as git submodules and must be checked out when cloning:
- [acbt](https://repos.wusikijeronii.me/app3d/acbt)
- [auik](https://repos.wusikijeronii.me/app3d/auik)

## License
This project is licensed under the  [MIT License](LICENSE).

## Contacts
For any questions or feedback, you can reach out via [email](mailto:wusikijeronii@gmail.com) or open a new issue.