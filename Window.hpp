#ifndef SCENEGRAPH_WINDOW_HPP
#define SCENEGRAPH_WINDOW_HPP
#include <QElapsedTimer>
#include <QQuickItem>
#include <QQuickView>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "Item.hpp"

class QOpenGLTexture;

namespace SceneGraph {

class Renderer;

class Window : public QQuickView {
 private:
  friend class Item;
  friend class RootItem;
  friend class Renderer;

  class RootItem : public QQuickItem {
   private:
    Window* m_window;

   protected:
    void touchEvent(QTouchEvent*);

   public:
    RootItem(Window*, QQuickItem* = nullptr);
  } m_rootItem;

  std::unique_ptr<Renderer> m_renderer;
  QMatrix4x4 m_projection;

  Item m_root;
  Item* m_focusItem;
  std::vector<Item*> m_updateItem;
  std::vector<std::unique_ptr<Node>> m_destroyedItemNode;
  std::vector<std::unique_ptr<Node>> m_destroyedNode;

  std::unordered_map<int, Item*> m_timerItem;
  std::unordered_map<Item*, std::unordered_set<int>> m_timerMap;

  bool m_lockedCursor;
  bool m_allowLockCursor;

  std::string m_glVersion;

  qreal m_fps;
  QElapsedTimer m_fpscounter;

  void onSceneGraphInitialized();
  void onSceneGraphInvalidated();
  void onBeforeRendering();
  void onBeforeSynchronizing();
  void onItemDestroyed(Item*);
  void destroyNode(Item*);

  void scheduleUpdate(Item*);
  void cancelUpdate(Item*);

  int installTimer(Item*, int interval);
  void removeTimer(Item*, int timerId);

  bool lockCursor();
  bool unlockCursor();

  void fixCursor();

 protected:
  void keyPressEvent(QKeyEvent*);
  void keyReleaseEvent(QKeyEvent*);
  void touchEvent(QTouchEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent*);
  void timerEvent(QTimerEvent*);
  void resizeEvent(QResizeEvent*);

 public:
  enum class System { Android, Unix, Win32, Unknown };

  Window(QWindow* window = nullptr);
  ~Window();

  inline Item* rootItem() { return &m_root; }
  inline Item* focusItem() const { return m_focusItem; }

  inline const QMatrix4x4& projection() const { return m_projection; }
  void setProjection(const QMatrix4x4& m);

  void scheduleSynchronize();

  QOpenGLTexture* texture(const char* path);

  inline bool lockedCursor() const { return m_lockedCursor; }
  void setLockedCursor(bool);

  inline bool allowLockCursor() const { return m_allowLockCursor; }
  void setAllowLockCursor(bool);

  void setFullScreen(bool);
  bool fullscreen() const;

  System system() const;
  inline qreal fps() const { return m_fps; }

  static std::string systemToString(System);
};
}  //  namespace SceneGraph

#endif  // SCENEGRAPH_WINDOW_HPP
