#include "ShaderSource.hpp"
#include <QTime>
#include <cassert>
#include "Renderer.hpp"
#include "Window.hpp"

namespace SceneGraph {

ShaderSource::ShaderSource(Item* parent)
    : SceneGraph::Item(parent),
      m_sourceItem(this),
      m_sourceRect(-1, -1, 2, 2),
      m_background(Qt::white),
      m_node() {
  m_sourceItem.setVisible(false);

  setBackground(Qt::transparent);
}

ShaderSource::~ShaderSource() {}

void ShaderSource::setSourceRect(QRectF rect) {
  m_sourceRect = rect;
  update();
}

void ShaderSource::setTextureSize(QSize size) {
  m_textureSize = size;
  update();
}

void ShaderSource::setBackground(QColor color) {
  m_background = color;
  update();
}

void ShaderSource::invalidate() {
  Item::invalidate();
  m_node = nullptr;
}

std::unique_ptr<Node> ShaderSource::synchronize(std::unique_ptr<Node> node) {
  if (!node) {
    node = std::make_unique<ShaderNode>(textureSize());
    m_node = static_cast<ShaderNode*>(node.get());
  }
  static_cast<ShaderNode*>(node.get())->update(this);
  return node;
}

ShaderSource::ShaderNode::ShaderNode(QSize size, Node* parent)
    : Node(parent), m_fbo(), m_capturedNode(), m_size(size), m_lastUpdate(-1) {
  initializeOpenGLFunctions();
}

ShaderSource::ShaderNode::~ShaderNode() {}

void ShaderSource::ShaderNode::updateTexture() {
  assert(renderer());

  if (m_lastUpdate == renderer()->frame()) return;
  m_lastUpdate = renderer()->frame();

  assert(m_size.isValid());
  if (!m_fbo || m_fbo->size() != m_size)
    m_fbo = std::make_unique<QOpenGLFramebufferObject>(m_size);

  m_fbo->bind();

  if (m_capturedNode) {
    GLint view[4];
    glGetIntegerv(GL_VIEWPORT, view);

    glViewport(0, 0, m_fbo->width(), m_fbo->height());
    glClearColor(m_background.redF(), m_background.greenF(),
                 m_background.blueF(), m_background.alphaF());
    glClear(GL_COLOR_BUFFER_BIT);

    QMatrix4x4 matrix;
    matrix.ortho(m_viewport.left(), m_viewport.right(), m_viewport.top(),
                 m_viewport.bottom(), -1, 1);
    renderer()->render(m_capturedNode, RenderState(matrix));

    glViewport(view[0], view[1], view[2], view[3]);
  }

  m_fbo->release();
}

void ShaderSource::ShaderNode::update(ShaderSource* i) {
  m_capturedNode = i->m_sourceItem.m_itemNode.get();
  m_background = i->m_background;
  m_viewport = i->m_sourceRect;
  m_size = i->m_textureSize;
}
}  // namespace SceneGraph
