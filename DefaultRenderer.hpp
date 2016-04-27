#ifndef DEFAULTRENDERER_HPP
#define DEFAULTRENDERER_HPP
#include <QOpenGLFunctions>
#include "Renderer.hpp"

namespace SceneGraph {

class DefaultRenderer : public Renderer, public QOpenGLFunctions {
 protected:
  void renderGeometryNode(GeometryNode *node, const RenderState &);

 public:
  DefaultRenderer();

  void render();
};
}  // namespace SceneGraph

#endif  // DEFAULTRENDERER_HPP
