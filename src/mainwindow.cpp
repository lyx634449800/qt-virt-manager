#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    setMinimumSize(100, 100);
    setContentsMargins(0, 0, 0, 5);
    setWindowTitle("Qt VirtManager");
    QStringList searchThemePath;
    // TODO: make a cross platform here
    searchThemePath.append(QIcon::themeSearchPaths());
    searchThemePath.insert(0, "/usr/share/qt-virt-manager/");
    QIcon::setThemeSearchPaths(searchThemePath);
    //
    QIcon::setThemeName("icons");
    setWindowIcon(QIcon::fromTheme("virtual-engineering"));
    setMouseTracking(true);
    setDockOptions(QMainWindow::AnimatedDocks|
                   QMainWindow::ForceTabbedDocks
    );
    restoreGeometry(settings.value("Geometry").toByteArray());
    initTrayIcon();
    initConnListWidget();
    initToolBar();
    initDockWidgets();
    restoreState(settings.value("State").toByteArray());
    this->setVisible(!settings.value("Visible", false).toBool());
    changeVisibility();
}
MainWindow::~MainWindow()
{
  disconnect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, \
                    SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
  disconnect(trayIcon->hideAction, SIGNAL(triggered()), this, SLOT(changeVisibility()));
  disconnect(trayIcon->logUpAction, SIGNAL(triggered()), this, SLOT(changeLogViewerVisibility()));
  disconnect(trayIcon->closeAction, SIGNAL(triggered()), this, SLOT(closeEvent()));
  disconnect(connListWidget, SIGNAL(removeConnect(QString&)), this, SLOT(removeConnectItem(QString&)));
  disconnect(connListWidget, SIGNAL(messageShowed()), this, SLOT(mainWindowUp()));
  disconnect(connListWidget, SIGNAL(warning(QString&)), this, SLOT(writeToErrorLog(QString&)));
  disconnect(connListWidget, SIGNAL(connPtr(virConnect*, QString&)), this, SLOT(receiveConnPtr(virConnect*, QString&)));
  disconnect(connListWidget, SIGNAL(connectClosed(virConnect*)), this, SLOT(stopConnProcessing(virConnect*)));
  disconnect(toolBar->_hideAction, SIGNAL(triggered()), this, SLOT(changeVisibility()));
  disconnect(toolBar->_createAction, SIGNAL(triggered()), this, SLOT(createNewConnect()));
  disconnect(toolBar->_editAction, SIGNAL(triggered()), this, SLOT(editCurrentConnect()));
  disconnect(toolBar->_deleteAction, SIGNAL(triggered()), this, SLOT(deleteCurrentConnect()));
  disconnect(toolBar->_openAction, SIGNAL(triggered()), this, SLOT(openCurrentConnect()));
  disconnect(toolBar->_showAction, SIGNAL(triggered()), this, SLOT(showCurrentConnect()));
  disconnect(toolBar->_closeAction, SIGNAL(triggered()), this, SLOT(closeCurrentConnect()));
  disconnect(toolBar->_closeAllAction, SIGNAL(triggered()), this, SLOT(closeAllConnect()));
  disconnect(toolBar->_logUpAction, SIGNAL(triggered()), this, SLOT(changeLogViewerVisibility()));
  disconnect(toolBar->_closeOverview, SIGNAL(triggered()), this, SLOT(stopProcessing()));
  disconnect(toolBar->_exitAction, SIGNAL(triggered()), this, SLOT(closeEvent()));
  disconnect(toolBar, SIGNAL(warningShowed()), this, SLOT(mainWindowUp()));
  disconnect(toolBar->_domUpAction, SIGNAL(triggered(bool)), domainDock, SLOT(setVisible(bool)));
  disconnect(toolBar->_netUpAction, SIGNAL(triggered(bool)), networkDock, SLOT(setVisible(bool)));
  disconnect(toolBar->_storageUpAction, SIGNAL(triggered(bool)), storageVolDock, SLOT(setVisible(bool)));
  disconnect(toolBar->_storageUpAction, SIGNAL(triggered(bool)), storagePoolDock, SLOT(setVisible(bool)));
  disconnect(networkDockContent, SIGNAL(netMsg(QString&)), this, SLOT(writeToErrorLog(QString&)));
  disconnect(domainDockContent, SIGNAL(domMsg(QString&)), this, SLOT(writeToErrorLog(QString&)));
  disconnect(domainDockContent, SIGNAL(dislayRequest(virConnect*,QString,QString)),
             this, SLOT(invokeVMDisplay(virConnect*,QString,QString)));
  disconnect(storagePoolDockContent, SIGNAL(storagePoolMsg(QString&)), this, SLOT(writeToErrorLog(QString&)));
  disconnect(storageVolDockContent, SIGNAL(storageVolMsg(QString&)), this, SLOT(writeToErrorLog(QString&)));
  disconnect(storagePoolDockContent, SIGNAL(currPool(virConnect*,QString&,QString&)),
             this, SLOT(receivePoolName(virConnect*,QString&,QString&)));

  qDebug()<<"processing stopped";
  if ( wait_thread!=NULL ) {
      disconnect(wait_thread, SIGNAL(finished()), this, SLOT(closeEvent()));
      disconnect(wait_thread, SIGNAL(refreshProcessingState()), this, SLOT(stopProcessing()));
      delete wait_thread;
      wait_thread = 0;
  };
  foreach (QString name, VM_Displayed_Map.keys()) {
      VM_Viewer *wdg = VM_Displayed_Map.value(name);
      if ( wdg!=NULL ) {
          delete wdg;
          wdg = 0;
      }
  };
  VM_Displayed_Map.clear();

  delete logDockContent;
  logDockContent = 0;
  delete logDock;
  logDock = 0;

  delete domainDockContent;
  domainDockContent = 0;
  delete domainDock;
  domainDock = 0;

  delete networkDockContent;
  networkDockContent = 0;
  delete networkDock;
  networkDock = 0;

  delete storageVolDockContent;
  storageVolDockContent = 0;
  delete storageVolDock;
  storageVolDock = 0;

  delete storagePoolDockContent;
  storagePoolDockContent = 0;
  delete storagePoolDock;
  storagePoolDock = 0;

  delete connListWidget;
  connListWidget = 0;

  delete toolBar;
  toolBar = 0;

  delete trayIcon;
  trayIcon = 0;
  qDebug()<<"application stopped";
}
void MainWindow::closeEvent(QCloseEvent *ev)
{
  settings.setValue("Geometry", saveGeometry());
  settings.setValue("State", saveState());
  settings.setValue("ToolBarArea", toolBarArea(toolBar));
  settings.setValue("Visible", this->isVisible());
  settings.beginGroup("LogDock");
  settings.setValue("DockkArea", dockWidgetArea(logDock));
  settings.setValue("Visible", logDock->isVisible());
  settings.setValue("Floating", logDock->isFloating());
  settings.setValue("Geometry", logDock->saveGeometry());
  settings.endGroup();
  settings.beginGroup("DomainDock");
  settings.setValue("DockkArea", dockWidgetArea(domainDock));
  settings.setValue("Visible", domainDock->isVisible());
  settings.setValue("Floating", domainDock->isFloating());
  settings.setValue("Geometry", domainDock->saveGeometry());
  settings.endGroup();
  settings.beginGroup("NetworkDock");
  settings.setValue("DockArea", dockWidgetArea(networkDock));
  settings.setValue("Visible", networkDock->isVisible());
  settings.setValue("Floating", networkDock->isFloating());
  settings.setValue("Geometry", networkDock->saveGeometry());
  settings.endGroup();
  settings.beginGroup("StorageVolDock");
  settings.setValue("DockArea", dockWidgetArea(storageVolDock));
  settings.setValue("Visible", storageVolDock->isVisible());
  settings.setValue("Floating", storageVolDock->isFloating());
  settings.setValue("Geometry", storageVolDock->saveGeometry());
  settings.endGroup();
  settings.beginGroup("StoragePoolDock");
  settings.setValue("DockArea", dockWidgetArea(storagePoolDock));
  settings.setValue("Visible", storagePoolDock->isVisible());
  settings.setValue("Floating", storagePoolDock->isFloating());
  settings.setValue("Geometry", storagePoolDock->saveGeometry());
  settings.endGroup();
  settings.beginGroup("ConnectListColumns");
  settings.setValue("column0", connListWidget->columnWidth(0));
  settings.setValue("column1", connListWidget->columnWidth(1));
  settings.setValue("column2", connListWidget->columnWidth(2));
  settings.endGroup();
  settings.sync();
  if ( !this->isVisible() ) changeVisibility();
  if ( runningConnectsExist() && wait_thread==NULL ) {
      QString q;
      q.append("Running Connects are exist.\nKill it at exit?");
      int answer = QMessageBox::question(this, "Action", q, QMessageBox::Yes, QMessageBox::Cancel);
      if ( answer == QMessageBox::Cancel ) {
          ev->ignore();
          return;
      };
      connListWidget->setEnabled(false);
      toolBar->setEnabled(false);
      logDock->setEnabled(false);
      domainDock->setEnabled(false);
      networkDock->setEnabled(false);
      storageVolDock->setEnabled(false);
      storagePoolDock->setEnabled(false);
      wait_thread = new Wait(this, connListWidget, VM_Displayed_Map);
      connect(wait_thread, SIGNAL(finished()), this, SLOT(closeEvent()));
      connect(wait_thread, SIGNAL(refreshProcessingState()), this, SLOT(stopProcessing()));
      wait_thread->start();
      ev->ignore();
  } else if ( !runningConnectsExist() && (wait_thread==NULL || wait_thread->isFinished()) ) {
      ev->accept();
  } else {
      ev->ignore();
  };
}
void MainWindow::closeEvent()
{
    this->close();
}
void MainWindow::initTrayIcon()
{
    trayIcon = new TrayIcon(this);
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, \
                      SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    connect(trayIcon->hideAction, SIGNAL(triggered()), this, SLOT(changeVisibility()));
    connect(trayIcon->logUpAction, SIGNAL(triggered()), this, SLOT(changeLogViewerVisibility()));
    connect(trayIcon->closeAction, SIGNAL(triggered()), this, SLOT(closeEvent()));
}
void MainWindow::mainWindowUp()
{
    if ( !this->isVisible() ) {
        changeVisibility();
        QTimer::singleShot(333, this, SLOT(autoHide()));
    };
}
void MainWindow::changeVisibility()
{
    if (this->isVisible()) {
        this->hide();
        trayIcon->hideAction->setText (QString("Up"));
        trayIcon->hideAction->setIcon (QIcon::fromTheme("up"));
        if ( domainDock->isFloating() ) domainDock->hide();
        if ( networkDock->isFloating() ) networkDock->hide();
        if ( storageVolDock->isFloating() ) storageVolDock->hide();
        if ( storagePoolDock->isFloating() ) storagePoolDock->hide();
    } else {
        this->show();
        trayIcon->hideAction->setText (QString("Down"));
        trayIcon->hideAction->setIcon (QIcon::fromTheme("down"));
        if ( domainDock->isFloating() && toolBar->_domUpAction->isChecked() ) domainDock->show();
        if ( networkDock->isFloating() && toolBar->_netUpAction->isChecked() ) networkDock->show();
        if ( storageVolDock->isFloating() && toolBar->_storageUpAction->isChecked() ) storageVolDock->show();
        if ( storagePoolDock->isFloating() && toolBar->_storageUpAction->isChecked() ) storagePoolDock->show();
    };
}
void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason r)
{
  if (r==QSystemTrayIcon::Trigger) changeVisibility();
}
void MainWindow::initConnListWidget()
{
  connListWidget = new ConnectList(this);
  settings.beginGroup("ConnectListColumns");
  connListWidget->setColumnWidth(0, settings.value("column0", 132).toInt());
  connListWidget->setColumnWidth(1, settings.value("column1", 32).toInt());
  connListWidget->setColumnWidth(2, settings.value("column2", 32).toInt());
  settings.endGroup();
  settings.beginGroup("Connects");
  QStringList groups = settings.childGroups();
  settings.endGroup();
  QList<QString>::const_iterator i;
  for (i=groups.constBegin(); i!=groups.constEnd(); ++i) {
      QString s = (*i);
      connListWidget->addConnectItem(s);
  };
  setCentralWidget(connListWidget);
  connect(connListWidget, SIGNAL(removeConnect(QString&)), this, SLOT(removeConnectItem(QString&)));
  connect(connListWidget, SIGNAL(messageShowed()), this, SLOT(mainWindowUp()));
  connect(connListWidget, SIGNAL(warning(QString&)), this, SLOT(writeToErrorLog(QString&)));
  connect(connListWidget, SIGNAL(connPtr(virConnect*, QString&)), this, SLOT(receiveConnPtr(virConnect*, QString&)));
  connect(connListWidget, SIGNAL(connectClosed(virConnect*)), this, SLOT(stopConnProcessing(virConnect*)));
}
void MainWindow::initToolBar()
{
  toolBar = new ToolBar(this);
  toolBar->setObjectName("toolBar");
  connect(toolBar->_hideAction, SIGNAL(triggered()), this, SLOT(changeVisibility()));
  connect(toolBar->_createAction, SIGNAL(triggered()), this, SLOT(createNewConnect()));
  connect(toolBar->_editAction, SIGNAL(triggered()), this, SLOT(editCurrentConnect()));
  connect(toolBar->_deleteAction, SIGNAL(triggered()), this, SLOT(deleteCurrentConnect()));
  connect(toolBar->_openAction, SIGNAL(triggered()), this, SLOT(openCurrentConnect()));
  connect(toolBar->_showAction, SIGNAL(triggered()), this, SLOT(showCurrentConnect()));
  connect(toolBar->_closeAction, SIGNAL(triggered()), this, SLOT(closeCurrentConnect()));
  connect(toolBar->_closeAllAction, SIGNAL(triggered()), this, SLOT(closeAllConnect()));
  connect(toolBar->_logUpAction, SIGNAL(triggered()), this, SLOT(changeLogViewerVisibility()));
  connect(toolBar->_closeOverview, SIGNAL(triggered()), this, SLOT(stopProcessing()));
  connect(toolBar->_exitAction, SIGNAL(triggered()), this, SLOT(closeEvent()));
  connect(toolBar, SIGNAL(warningShowed()), this, SLOT(mainWindowUp()));
  int area_int = settings.value("ToolBarArea", 4).toInt();
  this->addToolBar(toolBar->get_ToolBarArea(area_int), toolBar);
}
void MainWindow::initDockWidgets()
{
    bool visible;
    Qt::DockWidgetArea area;
    logDock = new QDockWidget(this);
    logDock->setObjectName("logDock");
    logDock->setWindowTitle("Log");
    logDock->setFeatures(
        QDockWidget::DockWidgetMovable   |
        QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetVerticalTitleBar
    );
    logDockContent = new LogDock(this);
    logDock->setWidget( logDockContent );
    settings.beginGroup("LogDock");
    logDock->setFloating(settings.value("Floating", false).toBool());
    logDock->restoreGeometry(settings.value("Geometry").toByteArray());
    visible = !settings.value("Visible", false).toBool();
    logDock->setVisible(visible);
    changeLogViewerVisibility();
    area = getDockArea(settings.value("DockArea", Qt::BottomDockWidgetArea).toInt());
    settings.endGroup();
    addDockWidget(area, logDock);

    domainDock = new QDockWidget(this);
    domainDock->setObjectName("domainDock");
    domainDock->setWindowTitle("Domain");
    domainDock->setWindowIcon(QIcon::fromTheme("domain"));
    domainDock->setFeatures(
        QDockWidget::DockWidgetMovable   |
        QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetVerticalTitleBar
    );
    domainDockContent = new VirtDomainControl(this);
    domainDock->setWidget( domainDockContent );
    settings.beginGroup("DomainDock");
    domainDock->setFloating(settings.value("Floating", false).toBool());
    domainDock->restoreGeometry(settings.value("Geometry").toByteArray());
    visible = settings.value("Visible", false).toBool();
    domainDock->setVisible(visible);
    toolBar->_domUpAction->setChecked(visible);
    area = getDockArea(settings.value("DockArea", Qt::BottomDockWidgetArea).toInt());
    settings.endGroup();
    addDockWidget(area, domainDock);
    connect(toolBar->_domUpAction, SIGNAL(triggered(bool)), domainDock, SLOT(setVisible(bool)));
    connect(domainDockContent, SIGNAL(domMsg(QString&)), this, SLOT(writeToErrorLog(QString&)));
    connect(domainDockContent, SIGNAL(dislayRequest(virConnect*,QString,QString)),
            this, SLOT(invokeVMDisplay(virConnect*,QString,QString)));

    networkDock = new QDockWidget(this);
    networkDock->setObjectName("networkDock");
    networkDock->setWindowTitle("Network");
    networkDock->setWindowIcon(QIcon::fromTheme("network"));
    networkDock->setFeatures(
        QDockWidget::DockWidgetMovable   |
        QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetVerticalTitleBar
    );
    networkDockContent = new VirtNetControl(this);
    networkDock->setWidget( networkDockContent );
    settings.beginGroup("NetworkDock");
    networkDock->setFloating(settings.value("Floating", false).toBool());
    networkDock->restoreGeometry(settings.value("Geometry").toByteArray());
    visible = settings.value("Visible", false).toBool();
    networkDock->setVisible(visible);
    toolBar->_netUpAction->setChecked(visible);
    area = getDockArea(settings.value("DockArea", Qt::BottomDockWidgetArea).toInt());
    settings.endGroup();
    addDockWidget(area, networkDock);
    connect(toolBar->_netUpAction, SIGNAL(triggered(bool)), networkDock, SLOT(setVisible(bool)));
    connect(networkDockContent, SIGNAL(netMsg(QString&)), this, SLOT(writeToErrorLog(QString&)));

    storageVolDock = new QDockWidget(this);
    storageVolDock->setObjectName("storageVolDock");
    storageVolDock->setWindowTitle("StorageVol");
    storageVolDock->setFeatures(
        QDockWidget::DockWidgetMovable   |
        QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetVerticalTitleBar
    );
    storageVolDockContent = new VirtStorageVolControl(this);
    storageVolDock->setWidget( storageVolDockContent );
    settings.beginGroup("StorageVolDock");
    storageVolDock->setFloating(settings.value("Floating", false).toBool());
    storageVolDock->restoreGeometry(settings.value("Geometry").toByteArray());
    visible = settings.value("Visible", false).toBool();
    storageVolDock->setVisible(visible);
    toolBar->_storageUpAction->setChecked(visible);
    area = getDockArea(settings.value("DockArea", Qt::BottomDockWidgetArea).toInt());
    settings.endGroup();
    addDockWidget(area, storageVolDock);
    connect(toolBar->_storageUpAction, SIGNAL(triggered(bool)), storageVolDock, SLOT(setVisible(bool)));
    connect(storageVolDockContent, SIGNAL(storageVolMsg(QString&)), this, SLOT(writeToErrorLog(QString&)));

    storagePoolDock = new QDockWidget(this);
    storagePoolDock->setObjectName("storagePoolDock");
    storagePoolDock->setWindowTitle("StoragePool");
    storagePoolDock->setFeatures(
        QDockWidget::DockWidgetMovable   |
        QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetVerticalTitleBar
    );
    storagePoolDockContent = new VirtStoragePoolControl(this);
    storagePoolDock->setWidget( storagePoolDockContent );
    settings.beginGroup("StoragePoolDock");
    storagePoolDock->setFloating(settings.value("Floating", false).toBool());
    storagePoolDock->restoreGeometry(settings.value("Geometry").toByteArray());
    visible = settings.value("Visible", false).toBool();
    storagePoolDock->setVisible(visible);
    toolBar->_storageUpAction->setChecked(visible);
    area = getDockArea(settings.value("DockArea", Qt::BottomDockWidgetArea).toInt());
    settings.endGroup();
    addDockWidget(area, storagePoolDock);
    connect(toolBar->_storageUpAction, SIGNAL(triggered(bool)), storagePoolDock, SLOT(setVisible(bool)));
    connect(storagePoolDockContent, SIGNAL(storagePoolMsg(QString&)), this, SLOT(writeToErrorLog(QString&)));
    connect(storagePoolDockContent, SIGNAL(currPool(virConnect*,QString&,QString&)),
            this, SLOT(receivePoolName(virConnect*,QString&,QString&)));

    domainDockContent->setEnabled(false);
    networkDockContent->setEnabled(false);
    storageVolDockContent->setEnabled(false);
    storagePoolDockContent->setEnabled(false);
    /*
     * TODO: add docs for Interfaces, NetFilters, Node(Devices|Info),
     * Secrets, DomainInfo
     */
}
void MainWindow::editCurrentConnect()
{
  connListWidget->connectItemEditAction();
}
void MainWindow::createNewConnect()
{
  QString s = QString("<noname>");
  connListWidget->addConnectItem(s);
}
void MainWindow::deleteCurrentConnect()
{
  connListWidget->deleteCurrentConnect();
}
void MainWindow::removeConnectItem(QString &connect)
{
  settings.beginGroup("Connects");
  settings.remove(connect);
  settings.endGroup();
  //qDebug()<<connect<<"connect deleted";
}
void MainWindow::openCurrentConnect()
{
    QModelIndex _item = connListWidget->currentIndex();
    if (_item.isValid()) {
        connListWidget->openConnect(_item);
    };
}
void MainWindow::showCurrentConnect()
{
    QModelIndex _item = connListWidget->currentIndex();
    if (_item.isValid()) {
        connListWidget->showConnect(_item);
    };
}
void MainWindow::closeCurrentConnect()
{
    QModelIndex _item = connListWidget->currentIndex();
    if (_item.isValid()) {
        connListWidget->closeConnect(_item);
    };
}
void MainWindow::closeAllConnect()
{
  int count = connListWidget->connItemModel->rowCount();
  for (int i = 0; i< count; i++) closeConnect(i);
}
void MainWindow::closeConnect(int i)
{
  //qDebug()<<i<<" item to stop";
  QModelIndex _item = connListWidget->connItemModel->index(i, 0);
  connListWidget->closeConnect(_item);
}
bool MainWindow::runningConnectsExist()
{
    bool result = false;
    int count = connListWidget->connItemModel->rowCount();
    for (int i=0; i<count; i++) {
        ConnItemIndex *item = connListWidget->connItemModel->connItemDataList.at(i);
        //qDebug()<<connListWidget->item(i)->text()<< connListWidget->item(i)->data(Qt::UserRole).toMap().value("isRunning").toInt();
        if ( item->getData().value("isRunning").toInt()==RUNNING ||
            !item->getData().value("availability").toBool() ) {
            result = true;
            break;
        };
    };
    return result;
}
void MainWindow::autoHide()
{
    if (this->isVisible()) changeVisibility();
}
void MainWindow::writeToErrorLog(QString &msg)
{
    logDockContent->appendErrorMsg(msg);
}
void MainWindow::changeLogViewerVisibility()
{
    QString text;
    if ( logDock->isVisible() ) {
        logDock->hide();
        text = "Show Log Viewer";
    } else {
        logDock->show();
        text = "Hide Log Viewer";
    };
    trayIcon->setLogUpActionText(text);
    toolBar->_logUpAction->setChecked(logDock->isVisible());
}

Qt::DockWidgetArea MainWindow::getDockArea(int i) const
{
    Qt::DockWidgetArea result;
    switch (i) {
    case 1:
        result = Qt::LeftDockWidgetArea;
        break;
    case 2:
        result = Qt::RightDockWidgetArea;
        break;
    case 4:
        result = Qt::TopDockWidgetArea;
        break;
    case 8:
        result = Qt::BottomDockWidgetArea;
        break;
    default:
        result = Qt::AllDockWidgetAreas;
        break;
    };
    return result;
}
void MainWindow::receiveConnPtr(virConnect *conn, QString &name)
{
    // send connect ptr to all related virtual resources for operating
    if ( domainDockContent->setCurrentWorkConnect(conn) ) domainDockContent->setListHeader(name);
    if ( networkDockContent->setCurrentWorkConnect(conn) ) networkDockContent->setListHeader(name);
    if ( storagePoolDockContent->setCurrentWorkConnect(conn) ) storagePoolDockContent->setListHeader(name);
    storageVolDockContent->stopProcessing();
}
void MainWindow::receivePoolName(virConnect *conn, QString &connName, QString &poolName)
{
    // send Pool name to Volume Comtrol for operating
    storageVolDockContent->setCurrentStoragePool(conn, connName, poolName);
}
void MainWindow::stopConnProcessing(virConnect *conn)
{
    // clear Overview Docks if closed connect is overviewed
    if ( NULL!=conn && conn==domainDockContent->getConnect() ) stopProcessing();
}
void MainWindow::stopProcessing()
{
    bool result = true;
    // stop processing of all virtual resources
    domainDockContent->stopProcessing();
    networkDockContent->stopProcessing();
    storagePoolDockContent->stopProcessing();
    storageVolDockContent->stopProcessing();
    result = result && domainDockContent->getThreadState();
    result = result && networkDockContent->getThreadState() ;
    result = result && storagePoolDockContent->getThreadState();
    result = result && storageVolDockContent->getThreadState();
    if ( wait_thread!=NULL ) wait_thread->setProcessingState(result);
}
void MainWindow::invokeVMDisplay(virConnect *conn, QString connName, QString domName)
{
    QString key = QString("%1_%2").arg(connName).arg(domName);
    if ( !VM_Displayed_Map.contains(key) ) {
        VM_Viewer *value = new VM_Viewer(this, conn, domName);
        VM_Displayed_Map.insert(key, value);
    } else {
        setFocusProxy(VM_Displayed_Map.value(key, NULL));
    };
}
