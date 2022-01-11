#pragma once

#include <QDialog>

#include <memory>

namespace Ui { class UserDefinedConstantsDialog; }
struct ProjectContainer;
class ConstantsModel;

class UserDefinedConstantsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserDefinedConstantsDialog(ProjectContainer &project, QWidget *parent = nullptr);
    ~UserDefinedConstantsDialog();

private slots:
    void insert();
    void add();
    void delete_();
    void clear();
    void up();
    void down();
    void sort();
    void load();
    void save();

private:
    const std::unique_ptr<Ui::UserDefinedConstantsDialog> m_ui;

    ProjectContainer &m_project;

    const std::unique_ptr<ConstantsModel> m_model;
};
