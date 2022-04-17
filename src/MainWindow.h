#pragma once

#include <qmainwindow.h>

#include "Elf.h"
#include "Dwarf.h"
#include "DwarfModel.h"
#include "DwarfView.h"
#include "FilesModel.h"
#include "FilesView.h"
#include "FunctionsModel.h"
#include "FunctionsView.h"
#include "VariablesModel.h"
#include "VariablesView.h"
#include "TypesModel.h"
#include "TypesView.h"
#include "AbstractCodeModel.h"
#include "CodeView.h"
#include "OutputView.h"

#include <qtabwidget.h>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void openFile();
    void closeFile();

private:
    static MainWindow* s_mainWindow;

    QString m_path;
    Elf m_elf;
    Dwarf m_dwarf;
    QTabWidget* m_tabWidget;
    DwarfModel* m_dwarfModel;
    DwarfView* m_dwarfView;
    FilesModel* m_filesModel;
    FilesView* m_filesView;
    FunctionsModel* m_functionsModel;
    FunctionsView* m_functionsView;
    VariablesModel* m_variablesModel;
    VariablesView* m_variablesView;
    TypesModel* m_typesModel;
    TypesView* m_typesView;
    AbstractCodeModel* m_codeModel;
    CodeView* m_codeView;
    OutputView* m_outputView;

    static void outputWriteCallback(const QString& text);

private slots:
    void dwarfEntrySelected(DwarfEntry* entry);
    void dwarfAttributeSelected(DwarfAttribute* attribute);
    void filesFileSelected(const QString& path);
    void filesDirectorySelected(const QString& path);
    void filesNoneSelected();
    void functionsFunctionSelected(Elf32_Off dwarfOffset);
    void functionsNoneSelected();
    void variablesVariableSelected(Elf32_Off dwarfOffset);
    void variablesNoneSelected();
    void typesTypeDefinitionSelected(Elf32_Off dwarfOffset);
};
