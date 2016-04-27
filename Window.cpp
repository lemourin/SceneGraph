#include "Window.hpp"
#include <QOpenGLFramebufferObject>
#include <QQuickItem>
#include <QThread>
#include <cassert>
#include <functional>
#include "DefaultRenderer.hpp"
#include "Node.hpp"

#if defined(Q_OS_LINUX) and not defined(Q_OS_ANDROID)
#define USE_X11
#endif

#ifdef USE_X11
#include <X11/Xlib.h>
#include <unistd.h>
#include <QX11Info>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace SceneGraph {

Window::Window(QWindow* parent)
    : QQuickView(parent),
      m_rootItem(this, contentItem()),
      m_renderer(),
      m_focusItem(),
      m_lockedCursor() {
  m_root.setWindow(this);

  connect(this, &QQuickWindow::sceneGraphInitialized, this,
          &Window::onSceneGraphInitialized, Qt::DirectConnection);
  connect(this, &QQuickWindow::sceneGraphInvalidated, this,
          &Window::onSceneGraphInvalidated, Qt::DirectConnection);
  connect(this, &QQuickWindow::beforeRendering, this,
          &Window::onBeforeRendering, Qt::DirectConnection);
  connect(this, &QQuickWindow::beforeSynchronizing, this,
          &Window::onBeforeSynchronizing, Qt::DirectConnection);

  connect(this, &QWindow::activeChanged, this, &Window::onActiveChanged);

  setResizeMode(SizeRootObjectToView);
  setClearBeforeRendering(false);
  setPersistentSceneGraph(false);
}

Window::~Window() {
  onSceneGraphInvalidated();
  m_root.setWindow(nullptr);
  disconnect(this, &QQuickWindow::sceneGraphInitialized, this,
             &Window::onSceneGraphInitialized);
  disconnect(this, &QQuickWindow::sceneGraphInvalidated, this,
             &Window::onSceneGraphInvalidated);
  disconnect(this, &QQuickWindow::beforeRendering, this,
             &Window::onBeforeRendering);
  disconnect(this, &QQuickWindow::beforeSynchronizing, this,
             &Window::onBeforeSynchronizing);
  unlockCursor();
}

void Window::setProjection(const QMatrix4x4& m) {
  m_projection = m;
  scheduleSynchronize();
}

void Window::scheduleSynchronize() { update(); }

QOpenGLTexture* Window::texture(const char* path) {
  return m_renderer->texture(path);
}

void Window::onSceneGraphInitialized() {
  m_renderer = std::make_unique<DefaultRenderer>();
  m_renderer->setRoot(rootItem());
  rootItem()->updateSubtree();
}

void Window::onSceneGraphInvalidated() {
  if (!m_renderer) return;
  rootItem()->invalidateSubtree();
  m_renderer->synchronize(this);

  m_renderer = nullptr;
}

void Window::onBeforeRendering() {
  resetOpenGLState();
  m_renderer->render();
}

void Window::onBeforeSynchronizing() { m_renderer->synchronize(this); }

void Window::onItemDestroyed(Item* item) {
  cancelUpdate(item);

  if (m_timerMap.find(item) != m_timerMap.end()) {
    std::unordered_set<int> timer = m_timerMap[item];
    for (int t : timer) removeTimer(item, t);
  }

  destroyNode(item);

  if (m_focusItem == item) m_focusItem = nullptr;

  scheduleSynchronize();
}

void Window::destroyNode(Item* item) {
  if (item->m_itemNode)
    m_destroyedItemNode.push_back(std::move(item->m_itemNode));
  if (item->m_node) m_destroyedNode.push_back(std::move(item->m_node));
}

void Window::scheduleUpdate(Item* item) {
  if (!(item->m_state & Item::ScheduledUpdate)) {
    m_updateItem.push_back(item);
    item->m_state |= Item::ScheduledUpdate;
  } else {
    assert(std::find(m_updateItem.begin(), m_updateItem.end(), item) !=
           m_updateItem.end());
  }
}

void Window::cancelUpdate(Item* item) {
  if (item->m_state & Item::ScheduledUpdate) {
    auto it = std::find(m_updateItem.begin(), m_updateItem.end(), item);
    if (it != m_updateItem.end()) m_updateItem.erase(it);

    item->m_state &= ~Item::ScheduledUpdate;
  }
}

int Window::installTimer(Item* item, int interval) {
  int id = startTimer(interval);
  m_timerMap[item].insert(id);
  m_timerItem[id] = item;

  return id;
}

void Window::removeTimer(Item* item, int timerId) {
  std::unordered_set<int>& set = m_timerMap[item];
  assert(set.find(timerId) != set.end());

  set.erase(set.find(timerId));
  killTimer(timerId);

  if (set.empty()) m_timerMap.erase(m_timerMap.find(item));
}

void Window::keyPressEvent(QKeyEvent* event) {
  QQuickView::keyPressEvent(event);
  if (event->isAccepted()) return;

  Item* item = focusItem();
  while (item) {
    event->accept();
    item->keyPressEvent(event);
    if (event->isAccepted()) break;
    item = item->parent();
  }

  if (!item) event->ignore();
}

void Window::keyReleaseEvent(QKeyEvent* event) {
  QQuickView::keyReleaseEvent(event);
  if (event->isAccepted()) return;

  Item* item = focusItem();
  while (item) {
    event->accept();
    item->keyReleaseEvent(event);
    if (event->isAccepted()) break;
    item = item->parent();
  }

  if (!item) event->ignore();
}

void Window::touchEvent(QTouchEvent* e) {
  QQuickView::touchEvent(e);
  if (e->isAccepted()) return;

  Item* item = focusItem();
  while (item) {
    e->accept();
    item->touchEvent(e);
    if (e->isAccepted())
      break;
    else {
      bool accepted = false;
      if (e->touchPoints().size() == 1) {
        QTouchEvent::TouchPoint p = e->touchPoints().front();
        if (e->touchPointStates() & Qt::TouchPointPressed) {
          QMouseEvent t(QEvent::MouseButtonPress, p.pos(), Qt::LeftButton,
                        Qt::LeftButton, Qt::NoModifier);
          item->mousePressEvent(&t);
          accepted |= t.isAccepted();
        }
        if (e->touchPointStates() & Qt::TouchPointMoved) {
          QMouseEvent t(QEvent::MouseMove, p.pos(), Qt::NoButton, Qt::NoButton,
                        Qt::NoModifier);
          item->mouseMoveEvent(&t);
          accepted |= t.isAccepted();
        }
        if (e->touchPointStates() & Qt::TouchPointReleased) {
          QMouseEvent t(QEvent::MouseButtonRelease, p.pos(), Qt::LeftButton,
                        Qt::LeftButton, Qt::NoModifier);
          item->mouseReleaseEvent(&t);
          accepted |= t.isAccepted();
        }
      }

      if (accepted) break;
    }
    item = item->parent();
  }

  if (!item) e->ignore();
}

void Window::mousePressEvent(QMouseEvent* event) {
  QQuickView::mousePressEvent(event);
  if (event->isAccepted()) return;

  Item* item = focusItem();
  while (item) {
    event->accept();
    item->mousePressEvent(event);
    if (event->isAccepted()) break;
    item = item->parent();
  }

  if (!item) event->ignore();

  if (rootObject()) rootObject()->forceActiveFocus();
}

void Window::mouseReleaseEvent(QMouseEvent* event) {
  QQuickView::mouseReleaseEvent(event);
  if (event->isAccepted()) return;

  Item* item = focusItem();
  while (item) {
    event->accept();
    item->mouseReleaseEvent(event);
    if (event->isAccepted()) break;
    item = item->parent();
  }

  if (!item) event->ignore();
}

void Window::mouseMoveEvent(QMouseEvent* event) {
  QQuickView::mouseMoveEvent(event);
  if (event->isAccepted()) return;

  Item* item = focusItem();
  while (item) {
    event->accept();
    item->mouseMoveEvent(event);
    if (event->isAccepted()) break;
    item = item->parent();
  }

  if (!item) event->ignore();
}

void Window::wheelEvent(QWheelEvent* event) {
  QQuickView::wheelEvent(event);
  if (event->isAccepted()) return;

  Item* item = focusItem();
  while (item) {
    event->accept();
    item->wheelEvent(event);
    if (event->isAccepted()) break;
    item = item->parent();
  }

  if (!item) event->ignore();
}

void Window::timerEvent(QTimerEvent* event) {
  QQuickView::timerEvent(event);

  auto it = m_timerItem.find(event->timerId());
  if (it != m_timerItem.end()) it->second->timerEvent(event);
}

void Window::resizeEvent(QResizeEvent* event) {
  QQuickView::resizeEvent(event);
  m_rootItem.setSize(event->size());
  scheduleSynchronize();
#ifdef Q_OS_WIN
  if (lockedCursor()) lockCursor();
#endif
}

Window::RootItem::RootItem(Window* w, QQuickItem* parent)
    : QQuickItem(parent), m_window(w) {}

void Window::RootItem::touchEvent(QTouchEvent* e) {
  m_window->touchEvent(e);
  e->accept();
}

void Window::setLockedCursor(bool e) {
  if (m_lockedCursor == e) return;

  if (e) {
    if (lockCursor()) {
      m_lockedCursor = true;
    }
  } else {
    if (unlockCursor()) {
      m_lockedCursor = false;
    }
  }
}

bool Window::lockCursor() {
#ifdef USE_X11
  while (XGrabPointer(QX11Info::display(), winId(), true, 0, GrabModeAsync,
                      GrabModeAsync, winId(), None, CurrentTime) != GrabSuccess)
    sleep(1);
  return true;
#endif
#ifdef Q_OS_WIN
  QPoint p1 = mapToGlobal(QPoint(0, 0));
  QPoint p2 = mapToGlobal(QPoint(width(), height()));
  RECT rect;
  rect.left = p1.x();
  rect.top = p1.y();
  rect.right = p2.x();
  rect.bottom = p2.y();
  ClipCursor(&rect);
  return true;
#endif
}

bool Window::unlockCursor() {
#ifdef USE_X11
  XUngrabPointer(QX11Info::display(), CurrentTime);
  XFlush(QX11Info::display());
  return true;
#endif
#ifdef Q_OS_WIN
  ClipCursor(0);
  return true;
#endif
}

void Window::onActiveChanged() {
  if (lockedCursor()) {
    if (isActive())
      assert(lockCursor());
    else
      unlockCursor();
  }
}
}  // namespace SceneGraph
