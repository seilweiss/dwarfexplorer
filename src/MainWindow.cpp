#include "MainWindow.h"

#include "CppCodeModel.h"
#include "Output.h"

#include <qmenubar.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qdockwidget.h>
#include <qsettings.h>

MainWindow* MainWindow::s_mainWindow = nullptr;

AppSettings MainWindow::s_defaultSettings
{
    true, // openMostRecentFileOnStartup
    {}, // recentPaths
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_path()
    , m_elf()
    , m_dwarf()
    , m_tabWidget(new QTabWidget(this))
    , m_dwarfModel(new DwarfModel(this))
    , m_dwarfView(new DwarfView(this))
    , m_filesModel(new FilesModel(this))
    , m_filesView(new FilesView(this))
    , m_functionsModel(new FunctionsModel(this))
    , m_functionsView(new FunctionsView(this))
    , m_variablesModel(new VariablesModel(this))
    , m_variablesView(new VariablesView(this))
    , m_typesModel(new TypesModel(this))
    , m_typesView(new TypesView(this))
    , m_codeModel(new CppCodeModel(this))
    , m_codeView(new CodeView(this))
    , m_outputView(new OutputView(this))
    , m_fileMenu(nullptr)
    , m_recentPathsSeparator(nullptr)
    , m_recentPathActions()
    , m_settings()
{
    s_mainWindow = this;

    updateWindowTitle();

    loadSettings();
    saveSettings();

    resize(screen()->availableSize() * 0.5f);

    m_tabWidget->addTab(m_dwarfView, tr("DWARF Tree"));
    m_tabWidget->addTab(m_filesView, tr("Files"));
    m_tabWidget->addTab(m_functionsView, tr("Functions"));
    m_tabWidget->addTab(m_variablesView, tr("Variables"));
    m_tabWidget->addTab(m_typesView, tr("Types"));

    m_dwarfView->setModel(m_dwarfModel);
    m_filesView->setModel(m_filesModel);
    m_functionsView->setModel(m_functionsModel);
    m_variablesView->setModel(m_variablesModel);
    m_typesView->setModel(m_typesModel);
    m_codeView->setModel(m_codeModel);

    connect(m_dwarfView, &DwarfView::entrySelected, this, &MainWindow::dwarfEntrySelected);
    connect(m_dwarfView, &DwarfView::attributeSelected, this, &MainWindow::dwarfAttributeSelected);
    connect(m_filesView, &FilesView::fileSelected, this, &MainWindow::filesFileSelected);
    connect(m_filesView, &FilesView::noneSelected, this, &MainWindow::filesNoneSelected);
    connect(m_functionsView, &FunctionsView::functionSelected, this, &MainWindow::functionsFunctionSelected);
    connect(m_functionsView, &FunctionsView::noneSelected, this, &MainWindow::functionsNoneSelected);
    connect(m_variablesView, &VariablesView::variableSelected, this, &MainWindow::variablesVariableSelected);
    connect(m_variablesView, &VariablesView::noneSelected, this, &MainWindow::variablesNoneSelected);
    connect(m_typesView, &TypesView::typeDefinitionSelected, this, &MainWindow::typesTypeDefinitionSelected);

    m_fileMenu = menuBar()->addMenu(tr("File"));
    m_fileMenu->addAction(tr("Open..."), this, QOverload<>::of(&MainWindow::openFile));
    m_fileMenu->addAction(tr("Close"), this, &MainWindow::closeFile);
    m_fileMenu->addSeparator();

    m_recentPathsSeparator = m_fileMenu->addSeparator();

    m_fileMenu->addAction(tr("Exit"), this, &QMainWindow::close);

    updateFileMenu();

    QMenu* optionsMenu = menuBar()->addMenu(tr("Options"));
    QAction* action;

    action = optionsMenu->addAction(tr("Open most recent file on startup"));
    action->setCheckable(true);
    action->setChecked(m_settings.openMostRecentFileOnStartup);
    connect(action, &QAction::triggered, this, [=] {
        m_settings.openMostRecentFileOnStartup = action->isChecked();
        saveSettings();
        });

    setCentralWidget(m_tabWidget);

    QDockWidget* codeViewDock = new QDockWidget(tr("Code"));
    codeViewDock->setWidget(m_codeView);

    addDockWidget(Qt::RightDockWidgetArea, codeViewDock);

    QDockWidget* outputViewDock = new QDockWidget(tr("Output"));
    outputViewDock->setWidget(m_outputView);

    addDockWidget(Qt::BottomDockWidgetArea, outputViewDock);

    resizeDocks({ codeViewDock }, { width() / 2 }, Qt::Horizontal);

    Output::setWriteCallback(outputWriteCallback);

    if (m_settings.openMostRecentFileOnStartup && !m_settings.recentPaths.isEmpty())
    {
        QTimer::singleShot(0, [=] { openFile(m_settings.recentPaths.first()); });
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::openFile()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
        "ELF file (*.axf *.bin *.elf *.o *.out *.prx *.puff *.ko *.mod *.so);;"
        "All files (*.*)");

    if (path.isNull())
    {
        return;
    }

    openFile(path);
}

void MainWindow::openFile(const QString& path)
{
    closeFile();

    Output::write(tr("Opening file %1").arg(path));

    bool error = false;
    QString errorString;

    switch (m_elf.read(qPrintable(path)))
    {
    case Elf::ReadOpenFailed:
        error = true;
        errorString = tr("Could not open file %1").arg(path);
        break;
    case Elf::ReadFailed:
        error = true;
        errorString = tr("Could not read file %1").arg(path);
        break;
    case Elf::ReadInvalidHeader:
        error = true;
        errorString = tr("%1 is not a valid ELF file.").arg(path);
        break;
    }

    if (error)
    {
        Output::write(errorString);
        QMessageBox::warning(this, tr("Error"), errorString);
        return;
    }

    switch (m_dwarf.read(&m_elf))
    {
    case Dwarf::ReadSectionNotFound:
        error = true;
        errorString = tr("Could not find DWARF section in file %1").arg(path);
        break;
    }

    if (error)
    {
        Output::write(errorString);
        QMessageBox::warning(this, tr("Error"), errorString);
        return;
    }

    m_path = path;

    m_dwarfModel->setDwarf(&m_dwarf);
    m_filesModel->setDwarf(&m_dwarf);
    m_functionsModel->setDwarf(&m_dwarf);
    m_variablesModel->setDwarf(&m_dwarf);
    m_typesModel->setDwarf(&m_dwarf);
    m_codeModel->setDwarf(&m_dwarf);

    m_settings.recentPaths.removeAll(path); // for some reason path contains a different string after this call despite being passed in as a const ref... (Qt bug?)
    m_settings.recentPaths.prepend(m_path); // so we pass in m_path here

    if (m_settings.recentPaths.size() > 10)
    {
        m_settings.recentPaths.resize(10);
    }

    saveSettings();
    updateWindowTitle();
    updateFileMenu();
}

void MainWindow::closeFile()
{
    if (m_path.isEmpty())
    {
        return;
    }

    Output::write(tr("Closing file %1").arg(m_path));

    m_dwarfModel->setDwarf(nullptr);
    m_filesModel->setDwarf(nullptr);
    m_functionsModel->setDwarf(nullptr);
    m_variablesModel->setDwarf(nullptr);
    m_typesModel->setDwarf(nullptr);
    m_codeView->clear();
    m_codeModel->setDwarf(nullptr);

    m_dwarf.destroy();
    m_elf.destroy();

    m_path = QString();

    updateWindowTitle();
}

void MainWindow::dwarfEntrySelected(DwarfEntry* entry)
{
    m_codeView->viewDwarfEntry(entry->offset);
}

void MainWindow::dwarfAttributeSelected(DwarfAttribute* attribute)
{
}

void MainWindow::filesFileSelected(const QString& path)
{
    m_codeView->viewFile(path);
}

void MainWindow::filesDirectorySelected(const QString& path)
{
}

void MainWindow::filesNoneSelected()
{
    //m_codeView->clear();
}

void MainWindow::functionsFunctionSelected(Elf32_Off dwarfOffset)
{
    m_codeView->viewDwarfEntry(dwarfOffset);
}

void MainWindow::functionsNoneSelected()
{
}

void MainWindow::variablesVariableSelected(Elf32_Off dwarfOffset)
{
    m_codeView->viewDwarfEntry(dwarfOffset);
}

void MainWindow::variablesNoneSelected()
{
}

void MainWindow::typesTypeDefinitionSelected(Elf32_Off dwarfOffset)
{
    m_codeView->viewDwarfEntry(dwarfOffset);
}

void MainWindow::outputWriteCallback(const QString& text)
{
    printf("%s\n", qPrintable(text));
    s_mainWindow->m_outputView->appendPlainText(text);
}

void MainWindow::loadSettings()
{
    m_settings.recentPaths.clear();

    QSettings settings;

    m_settings.openMostRecentFileOnStartup = settings.value("settings/openMostRecentFileOnStartup", s_defaultSettings.openMostRecentFileOnStartup).toBool();

    int recentFilesCount = settings.beginReadArray("recentFiles");
    m_settings.recentPaths.reserve(recentFilesCount);

    for (int i = 0; i < recentFilesCount; i++)
    {
        settings.setArrayIndex(i);
        m_settings.recentPaths.append(settings.value("path").toString());
    }

    settings.endArray();
}

void MainWindow::saveSettings()
{
    QSettings settings;

    settings.setValue("settings/openMostRecentFileOnStartup", m_settings.openMostRecentFileOnStartup);

    int recentFilesCount = m_settings.recentPaths.size();
    settings.beginWriteArray("recentFiles", recentFilesCount);

    for (int i = 0; i < recentFilesCount; i++)
    {
        settings.setArrayIndex(i);
        settings.setValue("path", m_settings.recentPaths.at(i));
    }

    settings.endArray();
}

void MainWindow::updateWindowTitle()
{
    QString title = "DWARF Explorer";

    if (!m_path.isEmpty())
    {
        title.prepend(QString("%1 - ").arg(QFileInfo(m_path).fileName()));
    }

    setWindowTitle(title);
}

void MainWindow::updateFileMenu()
{
    qDeleteAll(m_recentPathActions);
    m_recentPathActions.clear();

    if (m_settings.recentPaths.isEmpty())
    {
        m_recentPathsSeparator->setVisible(false);
        return;
    }
    
    m_recentPathsSeparator->setVisible(true);
    
    for (const QString& path : m_settings.recentPaths)
    {
        QAction* action = new QAction(path);

        connect(action, &QAction::triggered, this, [=] { openFile(path); });

        m_fileMenu->insertAction(m_recentPathsSeparator, action);
        m_recentPathActions.append(action);
    }
}
