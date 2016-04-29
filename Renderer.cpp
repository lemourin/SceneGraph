#include "Renderer.hpp"
#include <QColor>
#include <QOpenGLTexture>
#include <QThread>
#include <cassert>
#include "Geometry.hpp"
#include "Material.hpp"
#include "Node.hpp"
#include "Shader.hpp"
#include "Window.hpp"

namespace SceneGraph {

Renderer::Renderer() : m_root(), m_frame(1) {}

Renderer::~Renderer() {}

void Renderer::updateItem(Item* item) {
  if (item->m_state & Item::ModelMatrixChanged) {
    item->m_itemNode->setMatrix(item->matrix());
    item->m_state &= ~Item::ModelMatrixChanged;

    item->matrixChanged();
  }

  if (item->m_state & Item::ParentChanged) {
    Node* current = item->m_itemNode.get();
    Node* parent = item->parent() ? item->parent()->m_itemNode.get() : nullptr;
    if (current->parent() != parent && item->visible()) {
      if (current->parent()) current->parent()->removeChild(current);
      if (parent) parent->appendChild(current);
    }

    item->m_state &= ~Item::ParentChanged;
  }

  if (item->m_state & Item::VisibleChanged) {
    item->m_state &= ~Item::VisibleChanged;

    if (item->parent()) {
      assert(item->parent()->m_itemNode);
      if (!item->visible())
        item->parent()->m_itemNode->removeChild(item->m_itemNode.get());
      else
        item->parent()->m_itemNode->appendChild(item->m_itemNode.get());
    }
  }

  if (item->visible()) {
    item->m_node = item->synchronize(std::move(item->m_node));
    if (item->m_node && item->m_node->parent() == nullptr)
      item->m_itemNode->appendChild(item->m_node.get());
  }
}

void Renderer::updateNodes(Window* window) {
  if (!window->m_updateItem.empty()) window->update();

  for (Item* item : window->m_updateItem) {
    if (item->m_itemNode == nullptr) {
      item->m_itemNode = std::make_unique<TransformNode>();
      item->m_itemNode->setRenderer(this);

      item->m_state |= Item::ModelMatrixChanged | Item::ParentChanged;
    }
  }

  for (size_t i = 0; i < window->m_updateItem.size(); i++) {
    Item* item = window->m_updateItem[i];
    item->m_state &= ~Item::ScheduledUpdate;
    updateItem(item);

    assert(!(item->m_state & Item::ScheduledUpdate));
  }

  window->m_updateItem = {};
}

void Renderer::destroyNodes(Window* window) {
  for (auto& itemNode : window->m_destroyedItemNode) {
    assert(itemNode);
    while (itemNode->firstChild())
      itemNode->removeChild(itemNode->firstChild());
  }
  window->m_destroyedItemNode.clear();
  window->m_destroyedNode.clear();
}

void Renderer::render(Node* root, RenderState state) {
  if (root->type() == Node::Type::GeometryNode) {
    renderGeometryNode(static_cast<GeometryNode*>(root), state);
  } else if (root->type() == Node::Type::TransformNode) {
    TransformNode* node = static_cast<TransformNode*>(root);
    state.setMatrix(state.matrix() * node->matrix());
  }

  for (Node* node = root->firstChild(); node; node = node->next())
    render(node, state);
}

void Renderer::nodeAdded(Node* node) {
  if (node->flag() & Node::UsePreprocess) m_preprocess.insert(node);
}

void Renderer::nodeDestroyed(Node* node) {
  if (node->flag() & Node::UsePreprocess) {
    assert(m_preprocess.find(node) != m_preprocess.end());
    m_preprocess.erase(node);
  }
  if (node == m_root) m_root = nullptr;
}

void Renderer::render() {
  for (Node* node : m_preprocess) node->preprocess();

  render(m_root, m_state);
  m_frame++;
}

void Renderer::synchronize(Window* window) {
  if (m_size != window->size()) {
    setSize(window->size());
    window->update();
  }

  m_state.setMatrix(window->projection());

  updateNodes(window);
  destroyNodes(window);
}

void Renderer::setSize(QSize size) { m_size = size; }

void Renderer::setRoot(Item* item) {
  if (item == nullptr) {
    if (m_root) m_root->setRenderer(nullptr);

    m_root = nullptr;
    return;
  }

  if (!item->m_itemNode) {
    item->m_itemNode = std::make_unique<TransformNode>();
    item->m_itemNode->setRenderer(this);
  }

  m_root = item->m_itemNode.get();
}

QOpenGLTexture* Renderer::texture(const char* path) {
  if (m_texture.find(path) == m_texture.end()) {
    QImage image(path);
    assert(!image.isNull());

    auto texture = std::make_unique<QOpenGLTexture>(image);
    texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    m_texture[path] = std::move(texture);
  }

  return m_texture[path].get();
}

RenderState::RenderState(QMatrix4x4 m) : m_matrix(m) {}
}  // namespace SceneGraph
