#include "MyGame.h"
#include "Scenes/BattleScene.h"
#include "Scenes/IceScene.h"
#include "Scenes/SettingsScene.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QDebug>
#include <QMetaEnum>

MyGame::MyGame(QWidget *parent) : QMainWindow(parent)
{
    // 初始化时先加载一个默认场景 BattleScene
    currentScene = new BattleScene(this);  // 默认启动 BattleScene
    currentSceneId = SceneID::BattleScene_ID; // 初始场景ID为 BattleScene
    previousActiveScene = nullptr; // 初始化上一个活动场景为空
    previousSceneId = SceneID::BattleScene_ID; // 初始时，上一个场景就是战斗场景（如果这是入口）
    view = new QGraphicsView(this);
    view->setScene(currentScene);

    view->setFixedSize((int) view->scene()->width(), (int) view->scene()->height());
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setCentralWidget(view);
    // Adjust the QMainWindow size to tightly wrap the QGraphicsView
    setFixedSize(view->sizeHint());
    currentScene->startLoop();

    // 在初始场景设置后，立即连接信号
    connect(currentScene, &Scene::requestSceneChange, this, &MyGame::handleSceneChangeRequest);

    // 菜单栏设置 (用于测试和导航)
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *gameMenu = menuBar->addMenu("游戏");
    QAction *settingsAction = gameMenu->addAction("设置");
    QAction *exitAction = gameMenu->addAction("退出");

    connect(settingsAction, &QAction::triggered, this, &MyGame::showSettings);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    QMenu *sceneMenu = menuBar->addMenu("场景");
    QAction *battleSceneAction = sceneMenu->addAction("战斗场景");
    connect(battleSceneAction, &QAction::triggered, [this]() {
        this->switchScene(SceneID::BattleScene_ID);
    });
    QAction *iceSceneAction = sceneMenu->addAction("冰雪场景");
    connect(iceSceneAction, &QAction::triggered, [this]() {
        this->switchScene(SceneID::IceScene_ID);
    });
}

// MyGame 析构函数
MyGame::~MyGame()
{
    if (currentScene)
    {
        currentScene->stopLoop();
        delete currentScene;
        currentScene = nullptr;
    }
    if (previousActiveScene)
    {
        delete previousActiveScene;
        previousActiveScene = nullptr;
    }
}

void MyGame::switchScene(SceneID id)
{
    // 如果要切换到SettingsScene，则不通过此函数，而是通过showSettings()处理
    if (id == SceneID::SettingsScene_ID)
    {
        showSettings();
        return;
    }

    // 如果当前场景存在且不是要SettingsScene，停止其循环并删除它
    if (currentScene&&currentSceneId != SceneID::SettingsScene_ID)
    {
        currentScene->stopLoop(); // 停止旧场景的游戏循环
        disconnect(currentScene, &Scene::requestSceneChange, this, &MyGame::handleSceneChangeRequest); // 断开旧场景的所有连接
        currentScene->deleteLater(); // 使用 deleteLater 来安全地删除对象
        currentScene = nullptr; // 将指针置空
    }

    // 设置新的当前场景
    Scene* newScene = nullptr;
    switch (id)
    {
        case SceneID::BattleScene_ID: // BattleScene
            newScene = new BattleScene(this);
            break;
        case SceneID::IceScene_ID: // IceScene
            newScene = new IceScene(this);
            break;
        default:
            qWarning("未知场景ID: %d", id);
            return;
    }

    // 新场景成功创建后
    if (newScene)
    {
        // 在切换场景之前，更新 previousSceneId
        // 只有当切换到的新场景不是设置场景时，才更新 previousSceneId
        // 这样，从设置场景返回时，可以回到进入设置场景之前的那个场景
        if (id != SceneID::SettingsScene_ID)
        {
            previousSceneId = id;
        }

        currentScene = newScene; // 设置新的当前场景
        view->setScene(currentScene); // 更新视图以显示新场景
        currentScene->startLoop(); // 启动新场景的游戏循环

        // 调整视图和主窗口大小以适应视图，这里使用 currentScene 的尺寸
        // 注意：currentScene->width() 和 currentScene->height() 应该返回 QGraphicsSceneRect 的宽度和高度
        view->setFixedSize(static_cast<int>(currentScene->width()), static_cast<int>(currentScene->height()));
        setFixedSize(view->sizeHint()); // 调整主窗口以适应视图
    }
}


void MyGame::showSettings()
{
    // 如果当前场景已经是 SettingsScene，则不需要再次切换
    if (currentSceneId == SceneID::SettingsScene_ID)
    {
        qDebug() << "当前场景已经是 SettingsScene，无需切换。";
        return;
    }

    if (currentScene)
    {
        // 停止当前场景的循环
        currentScene->stopLoop();
        // 将currentScene保存起来
        previousActiveScene = currentScene;
        // 断开与旧场景的连接，防止返回后重复触发
        disconnect(currentScene, &Scene::requestSceneChange, this, &MyGame::handleSceneChangeRequest);
    }
    else
    {
        qWarning() << "尝试进入设置时 currentScene 为空。将默认创建 BattleScene 作为上一个场景。";
        // 异常情况处理：如果 currentScene 为空，则创建默认场景作为 previousActiveScene
        previousActiveScene = new BattleScene(this);
        previousSceneId = SceneID::BattleScene_ID;
    }

    // 创建设置场景实例
    SettingsScene *settingsScene = new SettingsScene(this);
    // 连接 SettingsScene 的 backToGame 信号到 MyGame 的 returnFromSettings 槽函数
    connect(settingsScene, &SettingsScene::backToGame, this, &MyGame::returnFromSettings);

    // 设置新的场景ID
    currentScene = settingsScene; // 设置新的当前场景为 SettingsScene
    currentSceneId = SceneID::SettingsScene_ID; // 更新当前场景ID为 SettingsScene_ID

    view->setScene(currentScene); // 更新视图
    currentScene->startLoop();    // 启动 SettingsScene 的循环

    // 调整视图和主窗口大小以适应视图
    view->setFixedSize(static_cast<int>(currentScene->width()), static_cast<int>(currentScene->height()));
    setFixedSize(view->sizeHint());
}

void MyGame::showBattleScene()
{
    switchScene(SceneID::BattleScene_ID); // 切换回战斗场景
}

void MyGame::handleSceneChangeRequest(SceneID id)
{
    // 根据请求的场景ID切换场景
    switchScene(id);
}

void MyGame::returnFromSettings()
{
    // 确保当前场景是 SettingsScene
    if (currentSceneId != SceneID::SettingsScene_ID || !currentScene)
    {
        qWarning() << "当前场景不是 SettingsScene，无法返回游戏。";
        return;
    }

    // 停止并删除当前的 SettingsScene
    // 在showSettings函数中，已经把 currentScene 更新为 SettingsScene
    if (currentScene)
    {
        currentScene->stopLoop();
        disconnect(static_cast<SettingsScene*>(currentScene), &SettingsScene::backToGame, this, &MyGame::returnFromSettings);
        currentScene->deleteLater();
        currentScene = nullptr;
    }

    // 恢复之前保存的场
    // 在showSettings函数中，已经把当时的 currentScene 保存到 previousActiveScene
    if (previousActiveScene)
    {
        currentScene = previousActiveScene;
        previousActiveScene = nullptr; // 清空 previousActiveScene，因为它已经被恢复
        currentSceneId = previousSceneId; // 恢复之前的场景ID

        view->setScene(currentScene);
        currentScene->startLoop();

        // 重新连接场景切换信号
        connect(currentScene, &Scene::requestSceneChange, this, &MyGame::handleSceneChangeRequest);

        // 调整视图和主窗口大小以适应恢复的场景
        view->setFixedSize(static_cast<int>(currentScene->width()), static_cast<int>(currentScene->height()));
        setFixedSize(view->sizeHint());
    }
    else
    {
        // 如果 previousActiveScene 为空，说明没有上一个场景可以返回
        // 这种情况下，默认回到BattleScene。
        qDebug() << "没有场景可以返回，切换到 BattleScene";
        switchScene(SceneID::BattleScene_ID);
    }
}

// 处理键盘事件
void MyGame::keyPressEvent(QKeyEvent *event)
{
    // 检查是否是 Esc 键
    if (event->key() == Qt::Key_Escape)
    {
        // 阻止事件传播，防止同时触发 SettingsScene 内部的 Esc 返回功能（如果SettingsScene也处理了Esc）
        event->accept();
        qDebug() << "Esc pressed in MyGame. Switching to SettingsScene.";
        // 切换到设置场景
        switchScene(SceneID::SettingsScene_ID);
    }
    else
    {
        // 对于其他按键事件，调用基类的 keyPressEvent，
        // 这样可以确保 QMainWindow 默认的按键处理（例如 Tab 键切换焦点）仍然有效。
        QMainWindow::keyPressEvent(event);
    }
}
