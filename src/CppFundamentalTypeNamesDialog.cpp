#include "CppFundamentalTypeNamesDialog.h"

#include <qboxlayout.h>
#include <qlabel.h>
#include <qdialogbuttonbox.h>

CppFundamentalTypeNamesDialog::CppFundamentalTypeNamesDialog(QWidget* parent)
    : QDialog(parent)
    , m_names()
    , m_defaultNames()
    , m_lineEdits()
{
    setWindowTitle(tr("Edit fundamental type names"));

    QVBoxLayout* mainLayout = new QVBoxLayout;
    QHBoxLayout* bodyLayout = new QHBoxLayout;
    QVBoxLayout* leftLayout = new QVBoxLayout;
    QVBoxLayout* rightLayout = new QVBoxLayout;

    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok
        | QDialogButtonBox::Cancel
        | QDialogButtonBox::RestoreDefaults);

    connect(buttonBox, &QDialogButtonBox::clicked, this,
        [=](QAbstractButton* button)
        {
            switch (buttonBox->standardButton(button))
            {
            case QDialogButtonBox::Ok:
                accept();
                break;
            case QDialogButtonBox::Cancel:
                reject();
                break;
            case QDialogButtonBox::RestoreDefaults:
                restoreDefaults();
                break;
            }
        });

    bodyLayout->addLayout(leftLayout, 1);
    bodyLayout->addLayout(rightLayout, 1);

    mainLayout->addLayout(bodyLayout, 1);
    mainLayout->addWidget(buttonBox);

    addLineEdit(Cpp::FundamentalType::Char, "char", leftLayout);
    addLineEdit(Cpp::FundamentalType::SignedChar, "signed char", leftLayout);
    addLineEdit(Cpp::FundamentalType::UnsignedChar, "unsigned char", leftLayout);
    addLineEdit(Cpp::FundamentalType::Short, "short", leftLayout);
    addLineEdit(Cpp::FundamentalType::SignedShort, "signed short", leftLayout);
    addLineEdit(Cpp::FundamentalType::UnsignedShort, "unsigned short", leftLayout);
    addLineEdit(Cpp::FundamentalType::Int, "int", leftLayout);
    addLineEdit(Cpp::FundamentalType::SignedInt, "signed int", leftLayout);
    addLineEdit(Cpp::FundamentalType::UnsignedInt, "unsigned int", leftLayout);

    addLineEdit(Cpp::FundamentalType::Long, "long", rightLayout);
    addLineEdit(Cpp::FundamentalType::SignedLong, "signed long", rightLayout);
    addLineEdit(Cpp::FundamentalType::UnsignedLong, "unsigned long", rightLayout);
    addLineEdit(Cpp::FundamentalType::VoidPointer, "void*", rightLayout);
    addLineEdit(Cpp::FundamentalType::Float, "float", rightLayout);
    addLineEdit(Cpp::FundamentalType::Double, "double", rightLayout);
    addLineEdit(Cpp::FundamentalType::Void, "void", rightLayout);
    addLineEdit(Cpp::FundamentalType::Bool, "bool", rightLayout);
    addLineEdit(Cpp::FundamentalType::LongLong, "long long", rightLayout);

    setLayout(mainLayout);
}

QHash<Cpp::FundamentalType, QString> CppFundamentalTypeNamesDialog::names() const
{
    return m_names;
}

void CppFundamentalTypeNamesDialog::setNames(QHash<Cpp::FundamentalType, QString>& names)
{
    m_names = names;
    m_lineEdits[Cpp::FundamentalType::Char]->setText(m_names[Cpp::FundamentalType::Char]);
    m_lineEdits[Cpp::FundamentalType::SignedChar]->setText(m_names[Cpp::FundamentalType::SignedChar]);
    m_lineEdits[Cpp::FundamentalType::UnsignedChar]->setText(m_names[Cpp::FundamentalType::UnsignedChar]);
    m_lineEdits[Cpp::FundamentalType::Short]->setText(m_names[Cpp::FundamentalType::Short]);
    m_lineEdits[Cpp::FundamentalType::SignedShort]->setText(m_names[Cpp::FundamentalType::SignedShort]);
    m_lineEdits[Cpp::FundamentalType::UnsignedShort]->setText(m_names[Cpp::FundamentalType::UnsignedShort]);
    m_lineEdits[Cpp::FundamentalType::Int]->setText(m_names[Cpp::FundamentalType::Int]);
    m_lineEdits[Cpp::FundamentalType::SignedInt]->setText(m_names[Cpp::FundamentalType::SignedInt]);
    m_lineEdits[Cpp::FundamentalType::UnsignedInt]->setText(m_names[Cpp::FundamentalType::UnsignedInt]);
    m_lineEdits[Cpp::FundamentalType::Long]->setText(m_names[Cpp::FundamentalType::Long]);
    m_lineEdits[Cpp::FundamentalType::SignedLong]->setText(m_names[Cpp::FundamentalType::SignedLong]);
    m_lineEdits[Cpp::FundamentalType::UnsignedLong]->setText(m_names[Cpp::FundamentalType::UnsignedLong]);
    m_lineEdits[Cpp::FundamentalType::VoidPointer]->setText(m_names[Cpp::FundamentalType::VoidPointer]);
    m_lineEdits[Cpp::FundamentalType::Float]->setText(m_names[Cpp::FundamentalType::Float]);
    m_lineEdits[Cpp::FundamentalType::Double]->setText(m_names[Cpp::FundamentalType::Double]);
    m_lineEdits[Cpp::FundamentalType::Void]->setText(m_names[Cpp::FundamentalType::Void]);
    m_lineEdits[Cpp::FundamentalType::Bool]->setText(m_names[Cpp::FundamentalType::Bool]);
    m_lineEdits[Cpp::FundamentalType::LongLong]->setText(m_names[Cpp::FundamentalType::LongLong]);
}

QHash<Cpp::FundamentalType, QString> CppFundamentalTypeNamesDialog::defaultNames() const
{
    return m_defaultNames;
}

void CppFundamentalTypeNamesDialog::setDefaultNames(QHash<Cpp::FundamentalType, QString>& names)
{
    m_defaultNames = names;
}

void CppFundamentalTypeNamesDialog::addLineEdit(Cpp::FundamentalType type, const QString& label, QVBoxLayout* layout)
{
    QHBoxLayout* hlayout = new QHBoxLayout;
    QLineEdit* lineEdit = new QLineEdit;

    hlayout->addWidget(new QLabel(label), 1);
    hlayout->addWidget(lineEdit, 1);

    layout->addLayout(hlayout);

    m_lineEdits[type] = lineEdit;

    connect(lineEdit, &QLineEdit::editingFinished, this, [=]
        {
            m_names[type] = lineEdit->text();
        });
}

void CppFundamentalTypeNamesDialog::restoreDefaults()
{
    setNames(m_defaultNames);
}
