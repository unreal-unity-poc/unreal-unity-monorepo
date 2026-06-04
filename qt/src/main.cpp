#include "rust_engine.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QLibrary>
#include <QPainter>
#include <QRadialGradient>
#include <QSet>
#include <QTimer>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <stdexcept>

using RustEngineCreate = RustEngine* (*)();
using RustEngineDestroy = void (*)(RustEngine*);
using RustEngineSetControlInput = void (*)(RustEngine*, ControlInput);
using RustEngineTick = void (*)(RustEngine*, float);
using RustEngineRenderState = EarthRenderState (*)(const RustEngine*);
using RustEngineSurfacePatches = SurfacePatchView (*)(const RustEngine*);

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct RustApi {
    QLibrary library;
    RustEngineCreate create = nullptr;
    RustEngineDestroy destroy = nullptr;
    RustEngineSetControlInput setControlInput = nullptr;
    RustEngineTick tick = nullptr;
    RustEngineRenderState renderState = nullptr;
    RustEngineSurfacePatches surfacePatches = nullptr;

    RustApi()
        : library(QStringLiteral(RUST_ENGINE_LIBRARY_PATH))
    {
        if (!library.load()) {
            throw std::runtime_error(library.errorString().toStdString());
        }

        create = resolve<RustEngineCreate>("rust_engine_create");
        destroy = resolve<RustEngineDestroy>("rust_engine_destroy");
        setControlInput = resolve<RustEngineSetControlInput>("rust_engine_set_control_input");
        tick = resolve<RustEngineTick>("rust_engine_tick");
        renderState = resolve<RustEngineRenderState>("rust_engine_render_state");
        surfacePatches = resolve<RustEngineSurfacePatches>("rust_engine_surface_patches");
    }

    template <typename T>
    T resolve(const char* symbol)
    {
        T fn = reinterpret_cast<T>(library.resolve(symbol));
        if (!fn) {
            throw std::runtime_error(QString("Missing Rust symbol: %1").arg(symbol).toStdString());
        }

        return fn;
    }
};

class RustQtRenderer final : public QWidget {
public:
    explicit RustQtRenderer(QWidget* parent = nullptr)
        : QWidget(parent)
        , engine(api.create())
    {
        setWindowTitle(QStringLiteral("Rust Qt Earth Renderer"));
        setMinimumSize(900, 700);
        setFocusPolicy(Qt::StrongFocus);
        frameClock.start();

        connect(&timer, &QTimer::timeout, this, [this] {
            const float dt = std::clamp(frameClock.restart() / 1000.0f, 0.0f, 0.1f);
            api.setControlInput(engine, readInput());
            api.tick(engine, dt);
            update();
        });

        timer.start(16);
    }

    ~RustQtRenderer() override
    {
        if (engine) {
            api.destroy(engine);
        }
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), QColor(8, 11, 18));

        const EarthRenderState state = api.renderState(engine);
        const QPointF center(width() * 0.5, height() * 0.5);
        const double globeRadius = std::min(width(), height()) * 0.32 * (4.2 / state.camera_distance);

        QRadialGradient ocean(center.x() - globeRadius * 0.35, center.y() - globeRadius * 0.35, globeRadius * 1.2);
        ocean.setColorAt(0.0, QColor(55, 122, 210));
        ocean.setColorAt(0.55, QColor(18, 82, 170));
        ocean.setColorAt(1.0, QColor(4, 18, 62));

        painter.setPen(QPen(QColor(130, 185, 255), 2.0));
        painter.setBrush(ocean);
        painter.drawEllipse(center, globeRadius, globeRadius);
        drawSurfacePatches(painter, state, center, globeRadius);

        painter.setPen(QPen(QColor(142, 203, 255, 96), 4.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(center, globeRadius * state.atmosphere_radius, globeRadius * state.atmosphere_radius);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        pressedKeys.insert(event->key());
    }

    void keyReleaseEvent(QKeyEvent* event) override
    {
        pressedKeys.remove(event->key());
    }

private:
    RustApi api;
    RustEngine* engine = nullptr;
    QElapsedTimer frameClock;
    QTimer timer;
    QSet<int> pressedKeys;

    ControlInput readInput() const
    {
        ControlInput input {};

        if (pressedKeys.contains(Qt::Key_Up)) {
            input.rotate_x += 1.0f;
        }

        if (pressedKeys.contains(Qt::Key_Down)) {
            input.rotate_x -= 1.0f;
        }

        if (pressedKeys.contains(Qt::Key_Left)) {
            input.rotate_y += 1.0f;
        }

        if (pressedKeys.contains(Qt::Key_Right)) {
            input.rotate_y -= 1.0f;
        }

        if (pressedKeys.contains(Qt::Key_Plus) || pressedKeys.contains(Qt::Key_Equal) || pressedKeys.contains(Qt::Key_PageUp)) {
            input.zoom += 1.0f;
        }

        if (pressedKeys.contains(Qt::Key_Minus) || pressedKeys.contains(Qt::Key_PageDown)) {
            input.zoom -= 1.0f;
        }

        input.reset = pressedKeys.contains(Qt::Key_R) ? 1u : 0u;
        return input;
    }

    void drawSurfacePatches(
        QPainter& painter,
        const EarthRenderState& state,
        const QPointF& center,
        double globeRadius) const
    {
        const SurfacePatchView patches = api.surfacePatches(engine);
        if (!patches.ptr || patches.len == 0) {
            return;
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(40, 132, 70));

        for (size_t index = 0; index < patches.len; ++index) {
            const SurfacePatch& patch = patches.ptr[index];
            const Vec3 rotated = rotate(sphericalNormal(patch.lat_degrees, patch.lon_degrees), state);
            if (rotated.z < -0.08f) {
                continue;
            }

            const double patchRadius = globeRadius * patch.radius_degrees / 90.0;
            const QPointF patchCenter(
                center.x() + rotated.x * globeRadius,
                center.y() - rotated.y * globeRadius);

            painter.drawEllipse(
                patchCenter,
                patchRadius * patch.stretch_x,
                patchRadius * patch.stretch_y);
        }
    }

    static Vec3 sphericalNormal(float latDegrees, float lonDegrees)
    {
        const float lat = latDegrees * static_cast<float>(M_PI) / 180.0f;
        const float lon = lonDegrees * static_cast<float>(M_PI) / 180.0f;
        const float cosLat = std::cos(lat);
        return {
            cosLat * std::sin(lon),
            std::sin(lat),
            cosLat * std::cos(lon),
        };
    }

    static Vec3 rotate(Vec3 v, const EarthRenderState& state)
    {
        const float cosX = std::cos(state.rotation_x);
        const float sinX = std::sin(state.rotation_x);
        const float y = v.y * cosX - v.z * sinX;
        const float z = v.y * sinX + v.z * cosX;
        v.y = y;
        v.z = z;

        const float cosY = std::cos(state.rotation_y);
        const float sinY = std::sin(state.rotation_y);
        const float x = v.x * cosY + v.z * sinY;
        v.z = -v.x * sinY + v.z * cosY;
        v.x = x;

        return v;
    }
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    RustQtRenderer renderer;
    renderer.show();

    return app.exec();
}

