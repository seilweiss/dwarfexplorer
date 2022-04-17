#include "CodeView.h"

#include "Icons.h"

#include <qboxlayout.h>
#include <qfontdatabase.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qscrollbar.h>

#include <Qsci/qscilexercpp.h>

CodeView::CodeView(QWidget* parent)
    : QWidget(parent)
    , m_viewInfo()
    , m_model(nullptr)
    , m_editor(new QsciScintilla(this))
    , m_code()
    , m_pathLineEdit(new QLineEdit(this))
    , m_saveButton(new QPushButton(Icons::saveIcon(), QString(), this))
    , m_settingsButton(new QPushButton(Icons::settingsIcon(), QString(), this))
    , m_settingsMenu(new QMenu(this))
{
    QsciLexer* lexer = new QsciLexerCPP(m_editor);

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(11);

    lexer->setDefaultFont(font);

    m_editor->setLexer(lexer);
    m_editor->setWrapMode(QsciScintilla::WrapWhitespace);
    m_editor->setWrapIndentMode(QsciScintilla::WrapIndentIndented);
    m_editor->setIndentationsUseTabs(false);
    m_editor->setTabWidth(4);
    m_editor->setAutoIndent(true);
    m_editor->setMargins(1);
    m_editor->setMarginType(0, QsciScintilla::NumberMargin);
    m_editor->setMarginWidth(0, 50);

    m_pathLineEdit->setReadOnly(true);

    m_settingsButton->setMenu(m_settingsMenu);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addWidget(m_saveButton);
    topLayout->addWidget(m_pathLineEdit, 1);
    topLayout->addWidget(m_settingsButton);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_editor, 1);

    setLayout(mainLayout);

    clear();

    connect(m_saveButton, &QPushButton::clicked, this, &CodeView::onSaveButtonClicked);
}

void CodeView::clear()
{
    m_viewInfo.mode = ViewInfo::None;
    refresh(false);
}

AbstractCodeModel* CodeView::model() const
{
    return m_model;
}

void CodeView::setModel(AbstractCodeModel* model)
{
    if (m_model)
    {
        m_model->disconnect(this);
    }

    m_model = model;

    m_settingsMenu->clear();

    if (m_model)
    {
        m_model->setupSettingsMenu(m_settingsMenu);
    }

    refresh(false);

    if (m_model)
    {
        connect(m_model, &AbstractCodeModel::rewriteRequested, this, &CodeView::onModelRewriteRequested);
    }
}

void CodeView::viewDwarfEntry(Elf32_Off offset)
{
    m_viewInfo.mode = ViewInfo::DwarfEntry;
    m_viewInfo.dwarfEntryOffset = offset;
    refresh(false);
}

void CodeView::viewFile(const QString& path)
{
    m_viewInfo.mode = ViewInfo::File;
    m_viewInfo.filePath = path;
    refresh(false);
}

void CodeView::refresh(bool retainScroll)
{
    m_code.clear();

    if (m_model)
    {
        switch (m_viewInfo.mode)
        {
        case ViewInfo::None:
            break;
        case ViewInfo::DwarfEntry:
            m_model->writeDwarfEntry(m_code, m_viewInfo.dwarfEntryOffset);
            m_pathLineEdit->setText(m_model->dwarfEntryName(m_viewInfo.dwarfEntryOffset));
            break;
        case ViewInfo::File:
            m_model->writeFile(m_code, m_viewInfo.filePath);
            m_pathLineEdit->setText(m_viewInfo.filePath);
            break;
        }
    }
    else
    {
        m_pathLineEdit->clear();
    }

    if (retainScroll)
    {
        // todo: this doesn't work right
        int firstVisibleLine = m_editor->firstVisibleLine();
        m_editor->setText(m_code);
        m_editor->setFirstVisibleLine(firstVisibleLine);
    }
    else
    {
        m_editor->setText(m_code);
    }
}

void CodeView::onSaveButtonClicked()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save file"), m_pathLineEdit->text());

    if (!path.isEmpty())
    {
        QFile file(path);

        if (!file.open(QFile::WriteOnly))
        {
            QMessageBox::warning(this, tr("Save error"), tr("Could not open file."));
            return;
        }

        file.write(m_editor->text().toLatin1());
    }
}

void CodeView::onModelRewriteRequested()
{
    refresh(true);
}
