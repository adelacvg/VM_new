#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include"helpdialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_load_clicked();

    void on_save_clicked();

    void on_help_clicked();

    void on_assemble_clicked();

    void on_run_clicked();

    void on_step_clicked();


    void on_sample1_clicked();

    void on_sample2_clicked();

    void on_stop_clicked();

private:
    Ui::MainWindow *ui;
    HelpDialog * helpdialog;
};

#endif // MAINWINDOW_H
