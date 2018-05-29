#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "vm.h"

#include<QFile>
#include<QTextStream>
#include<QMessageBox>
#include<QString>
#include<QProcess>
#include<thread>
#include<chrono>
#include<windows.h>
#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>
#include<string.h>
#include<shellapi.h>
#include<iostream>
#include<sstream>

using namespace std;

MY_VM vm,vm1;
int sstop =0;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

static size_t getFileSize(FILE* file)
{
    long int original_cursor = ftell(file);
    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    fseek(file, original_cursor, SEEK_SET);
    return size;
}

void MainWindow::on_load_clicked()
{
    QFile file("C:/VM/in.txt");
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this,"title","file not open");
    }
    QTextStream in(&file);
    QString text = in.readAll();
    ui->textEdit->setPlainText(text);
    file.close();

    QFile file1("C:/VM/stop.txt");
    if(!file1.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this,"title","file not open");
    }
    QTextStream in1(&file1);
    QString text1 = in1.readAll();
    ui->textEdit_stop->setPlainText(text1);
    file1.close();
    statusBar()->showMessage("File has been loaded",2000);
}

void MainWindow::on_save_clicked()
{
    QFile file("C:/VM/in.txt");
    if(!file.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox::warning(this,"title","file not open");
    }
    QTextStream out(&file);
    QString text = ui->textEdit->toPlainText();
    out<<text;
    file.flush();
    file.close();

    QFile file1("C:/VM/stop.txt");
    if(!file1.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox::warning(this,"title","file not open");
    }
    QTextStream out1(&file1);
    QString text1 = ui->textEdit_stop->toPlainText();
    out1<<text1;
    file1.flush();
    file1.close();
	vm.cpu.program_counter = 0;
    statusBar()->showMessage("File has been saved",2000);
}

void MainWindow::on_help_clicked()
{
    helpdialog = new HelpDialog(this);
    helpdialog->show();
    statusBar()->showMessage("Help",2000);
}

void MainWindow::on_assemble_clicked()
{
    ofstream out3("C:/VM/out.txt");
    out3.close();

    Assemble();
    Assemble1();

    statusBar()->showMessage("Assemble has finished",2000);
    FILE* file = fopen("C:/VM/out.bin", "r");
    FILE* file2 = fopen("C:/VM/out_stop.bin","r");
    if (!file)
    {
        QMessageBox::warning(this,"title","Error: cannot read file");
        return;
    }

	memory();

    QFile file1("C:/VM/mem.txt");
	if (!file1.open(QFile::ReadOnly | QFile::Text))
	{
		QMessageBox::warning(this, "title", "file not open");
	}
	QTextStream in(&file1);
	QString text1 = in.readAll();
    ui->textEdit_memory->setPlainText(text1);
	file1.close();


    size_t file_size = getFileSize(file);

    InitializeVM(&vm, 2 * file_size, file_size);
    InitializeVM(&vm1, 2 * file_size, file_size);

    fread(vm.memory, 1, file_size, file);
    fclose(file);

    fread(vm1.memory, 1, file_size, file2);
    fclose(file2);
    ofstream out2("C:/VM/out_stop.txt");
    out2.close();

    ui->textEdit_2->setPlainText("");
    ui->PClineEdit->setText("");
    ui->SPlineEdit->setText("");
    ui->RAlineEdit->setText("");
    ui->RBlineEdit->setText("");
    ui->RClineEdit->setText("");
    ui->RDlineEdit->setText("");
}

QString To_QString(string s)
{
    QString qstr;
    s = qstr.toStdString();
    qstr = QString::fromStdString(s);
    return qstr;
}

string Num_To_String(uint64_t x)
{
    stringstream st;
    string s;
    st<<x;
    st>>s;
    return s;
}

void MainWindow::on_run_clicked()
{
    vm.step=0;

    ofstream out("C:/VM/out.txt",ios::app);

    if (vm.cpu.status.ILLEGAL_ACCESS ||
        vm.cpu.status.ILLEGAL_INSTRUCTION ||
        vm.cpu.status.INVALID_REGISTER_INDEX ||
        vm.cpu.status.STACK_OVERFLOW ||
        vm.cpu.status.STACK_UNDERFLOW)
        PrintStatus(&vm);

    if(!sstop)
    RunVM(&vm);
    else
    RunVM(&vm1);
    //cout<<Num_To_String(vm.cpu.program_counter);
	out.close();
    ui->PClineEdit->setText(QString::number(vm.cpu.program_counter));
    ui->SPlineEdit->setText(QString::number(vm.cpu.stack_pointer));
    ui->RAlineEdit->setText(QString::number(vm.cpu.registers[0]));
    ui->RBlineEdit->setText(QString::number(vm.cpu.registers[1]));
    ui->RClineEdit->setText(QString::number(vm.cpu.registers[2]));
    ui->RDlineEdit->setText(QString::number(vm.cpu.registers[3]));

    QFile file("C:/VM/out.txt");
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this,"title","file not open");
    }
    QTextStream in(&file);
    QString text = in.readAll();
    ui->textEdit_2->setPlainText(text);
    file.close();
    statusBar()->showMessage("Run has finishd and result get daze*",3000);
}

void MainWindow::on_step_clicked()
{
    vm.step=1;

    if (vm.cpu.status.ILLEGAL_ACCESS ||
        vm.cpu.status.ILLEGAL_INSTRUCTION ||
        vm.cpu.status.INVALID_REGISTER_INDEX ||
        vm.cpu.status.STACK_OVERFLOW ||
        vm.cpu.status.STACK_UNDERFLOW)
        PrintStatus(&vm);

    if(!sstop)
    RunVM(&vm);
    else
    RunVM(&vm1);

    if(!sstop)
    {
        ui->PClineEdit->setText(QString::number(vm.cpu.program_counter));
        ui->SPlineEdit->setText(QString::number(vm.cpu.stack_pointer));
        ui->RAlineEdit->setText(QString::number(vm.cpu.registers[0]));
        ui->RBlineEdit->setText(QString::number(vm.cpu.registers[1]));
        ui->RClineEdit->setText(QString::number(vm.cpu.registers[2]));
        ui->RDlineEdit->setText(QString::number(vm.cpu.registers[3]));
    }
    else
    {
        ui->PClineEdit->setText(QString::number(vm1.cpu.program_counter));
        ui->SPlineEdit->setText(QString::number(vm1.cpu.stack_pointer));
        ui->RAlineEdit->setText(QString::number(vm1.cpu.registers[0]));
        ui->RBlineEdit->setText(QString::number(vm1.cpu.registers[1]));
        ui->RClineEdit->setText(QString::number(vm1.cpu.registers[2]));
        ui->RDlineEdit->setText(QString::number(vm1.cpu.registers[3]));
    }
    QFile file("C:/VM/out.txt");
	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		QMessageBox::warning(this, "title", "file not open");
	}
	QTextStream in(&file);
	QString text = in.readAll();
	ui->textEdit_2->setPlainText(text);
	file.close();
    statusBar()->showMessage("This step has finishd",2000);
}

void MainWindow::on_sample1_clicked()
{
    QFile file("C:/VM/sample1.txt");
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this,"title","file not open");
    }
    QTextStream in(&file);
    QString text = in.readAll();
    ui->textEdit->setPlainText(text);
    file.close();
    statusBar()->showMessage("sample1 has been loaded",2000);
}

void MainWindow::on_sample2_clicked()
{
    QFile file("C:/VM/sample2.txt");
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this,"title","file not open");
    }
    QTextStream in(&file);
    QString text = in.readAll();
    ui->textEdit->setPlainText(text);
    file.close();
    statusBar()->showMessage("sample1 has been loaded",2000);
}

void MainWindow::on_stop_clicked()
{
    if(sstop==0)
        ui->lineEdit_flag->setText(QString::number(1));
    else
        ui->lineEdit_flag->setText(QString::number(0));
    sstop=~sstop;
}
