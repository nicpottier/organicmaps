#pragma once

#include "drape_frontend/gui/skin.hpp"
#include "drape_frontend/render_state.hpp"

#include "shaders/program_manager.hpp"

#include "drape/batcher.hpp"
#include "drape/glsl_types.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/texture_manager.hpp"
#include "drape/vertex_array_buffer.hpp"

#include <functional>
#include <vector>

namespace gui
{
class Handle : public dp::OverlayHandle
{
public:
  Handle(uint32_t id, dp::Anchor anchor, m2::PointF const & pivot,
         m2::PointF const & size = m2::PointF::Zero());

  gpu::GuiProgramParams const & GetParams() const { return m_params; }

  bool Update(ScreenBase const & screen) override;

  virtual bool IsTapped(m2::RectD const & /* touchArea */) const { return false; }
  virtual void OnTapBegin() {}
  virtual void OnTap() {}
  virtual void OnTapEnd() {}

  bool IndexesRequired() const override;
  m2::RectD GetPixelRect(ScreenBase const & screen, bool perspective) const override;
  void GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const override;

  m2::PointF GetSize() const { return m_size; }
  virtual void SetPivot(glsl::vec2 const & pivot) { m_pivot = pivot; }

protected:
  gpu::GuiProgramParams m_params;
  glsl::vec2 m_pivot;
  mutable m2::PointF m_size;
};

class TappableHandle : public Handle
{
public:
  TappableHandle(uint32_t id, dp::Anchor anchor, m2::PointF const & pivot, m2::PointF const & size)
    : Handle(id, anchor, pivot, size)
  {}

  bool IsTapped(m2::RectD const & touchArea) const override;
};

struct ShapeControl
{
  ShapeControl() = default;
  ShapeControl(ShapeControl const & other) = delete;
  ShapeControl(ShapeControl && other) = default;

  ShapeControl & operator=(ShapeControl const & other) = delete;
  ShapeControl & operator=(ShapeControl && other) = default;

  struct ShapeInfo
  {
    ShapeInfo() : m_state(df::CreateGLState(gpu::Program::TexturingGui, df::RenderState::GuiLayer)) {}
    ShapeInfo(dp::GLState const & state, drape_ptr<dp::VertexArrayBuffer> && buffer,
              drape_ptr<Handle> && handle);

    void Destroy();

    dp::GLState m_state;
    drape_ptr<dp::VertexArrayBuffer> m_buffer;
    drape_ptr<Handle> m_handle;
  };

  void AddShape(dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket);

  std::vector<ShapeInfo> m_shapesInfo;
};

class ShapeRenderer final
{
public:
  using TShapeControlEditFn = std::function<void(ShapeControl &)>;

  ~ShapeRenderer();

  void Build(ref_ptr<gpu::ProgramManager> mng);
  void Render(ScreenBase const & screen, ref_ptr<gpu::ProgramManager> mng);
  void AddShape(dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket);
  void AddShapeControl(ShapeControl && control);

  void SetPivot(m2::PointF const & pivot);

  ref_ptr<Handle> ProcessTapEvent(m2::RectD const & touchArea);
  ref_ptr<Handle> FindHandle(FeatureID const & id);

private:
  void ForEachShapeControl(TShapeControlEditFn const & fn);

  using TShapeInfoEditFn = std::function<void(ShapeControl::ShapeInfo &)>;
  void ForEachShapeInfo(TShapeInfoEditFn const & fn);

private:
  std::vector<ShapeControl> m_shapes;
};

class Shape
{
public:
  explicit Shape(gui::Position const & position)
    : m_position(position)
  {}

  using TTapHandler = std::function<void()>;

protected:
  gui::Position m_position;
};
}  // namespace gui
