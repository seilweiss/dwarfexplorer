#pragma once

#include "CppCodeModel.h"

#include <qdialog.h>
#include <qlineedit.h>
#include <qboxlayout.h>
#include <qabstractbutton.h>

class CppFundamentalTypeNamesDialog : public QDialog
{
    Q_OBJECT

public:
    CppFundamentalTypeNamesDialog(QWidget* parent = nullptr);

    QHash<Cpp::FundamentalType, QString> names() const;
    void setNames(QHash<Cpp::FundamentalType, QString>& names);

    QHash<Cpp::FundamentalType, QString> defaultNames() const;
    void setDefaultNames(QHash<Cpp::FundamentalType, QString>& names);

private:
    QHash<Cpp::FundamentalType, QString> m_names;
    QHash<Cpp::FundamentalType, QString> m_defaultNames;
    QHash<Cpp::FundamentalType, QLineEdit*> m_lineEdits;

    void addLineEdit(Cpp::FundamentalType type, const QString& label, QVBoxLayout* layout);
    void restoreDefaults();
};
