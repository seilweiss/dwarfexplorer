#include "CodeView.h"

#include "Icons.h"

#include <qboxlayout.h>
#include <qfontdatabase.h>
#include <qfiledialog.h>
#include <qmessagebox.h>

#include <Qsci/qscilexercpp.h>

CodeView::CodeView(QWidget* parent)
    : QWidget(parent)
    , m_model(nullptr)
    , m_editor(new QsciScintilla(this))
    , m_code()
    , m_pathLineEdit(new QLineEdit(this))
    , m_saveButton(new QPushButton(Icons::saveIcon(), QString(), this))
    , m_settingsButton(new QPushButton(Icons::settingsIcon(), QString(), this))
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

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addWidget(m_saveButton);
    topLayout->addWidget(m_pathLineEdit, 1);
    topLayout->addWidget(m_settingsButton);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_editor, 1);

    setLayout(mainLayout);

    refresh();

    connect(m_saveButton, &QPushButton::clicked, this, &CodeView::onSaveButtonClicked);
}

void CodeView::clear()
{
    m_editor->clear();
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
    m_code.clear();

    refresh();

    connect(model, &AbstractCodeModel::modelReset, this, &CodeView::refresh);
}

void CodeView::viewDwarfEntry(Elf32_Off offset)
{
    m_code.clear();

    if (m_model)
    {
        m_model->writeDwarfEntry(m_code, offset);
        m_pathLineEdit->setText(m_model->dwarfEntryName(offset));
    }
    else
    {
        m_pathLineEdit->clear();
    }

    refresh();
}

void CodeView::viewFile(const QString& path)
{
    m_code.clear();

    if (m_model)
    {
        m_model->writeFile(m_code, path);
    }

    m_pathLineEdit->setText(path);

    refresh();
}

void CodeView::refresh()
{
    m_editor->setText(m_code);
    m_editor->setCursorPosition(0, 0);
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
