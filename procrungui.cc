/**
 * @file procrungui.cc
 * @brief Definitions for ProcRunGui class.
 * @author Nicu Tofan <nicu.tofan@gmail.com>
 * @copyright Copyright 2014 piles contributors. All rights reserved.
 * This file is released under the
 * [MIT License](http://opensource.org/licenses/mit-license.html)
 */

#include "procrungui.h"
#include "ui_procrungui.h"

#include "procrungui-private.h"

#include <procrun/procrunmodel.h>

#include <QProcess>
#include <QThread>
#include <QDateTime>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QMenu>
#include <QAction>
#include <QStyle>
#include <QApplication>
#include <QFileDialog>

#include <assert.h>

//! A process managed by the ProcRunGui class.
class PrgProcess : public QProcess {
    Q_OBJECT

public:

    //! Constructor.
    PrgProcess (ProcRunGui * prg, ProcRunGui::Kb kb, void * user_data) :
        QProcess (static_cast<QObject*>(prg)),
        prg_ (prg),
        start_time_(),
        end_time_(),
        all_output_(),
        b_started_(false),
        errors_(),
        states_(),
        kb_(kb),
        user_data_(user_data),
        widget_(NULL),
        close_on_exit_(false)
    {

        connect (this,
                 SIGNAL(error(QProcess::ProcessError)),
                 SLOT(errorSlot(QProcess::ProcessError)));
        connect (this,
                 SIGNAL(finished(int,QProcess::ExitStatus)),
                 SLOT(finishedSlot(int,QProcess::ExitStatus)));
        connect (this,
                 SIGNAL(readyReadStandardError()),
                 SLOT(readyReadStandardErrorSlot()));
        connect (this,
                 SIGNAL(readyReadStandardOutput()),
                 SLOT(readyReadStandardOutputSlot()));
        connect (this,
                 SIGNAL(started()),
                 SLOT(startedSlot()));
        connect (this,
                 SIGNAL(stateChanged (QProcess::ProcessState)),
                 SLOT(stateChangedSlot (QProcess::ProcessState)));
    }

    ~PrgProcess() {
        if (widget_ != NULL) {
            delete widget_;
        }
    }

    //! Get the duration in seconds.
    qint64
    runDuration() const {
        return end_time_.secsTo (start_time_);
    }

    //! Get the duration in seconds.
    int durationInSeconds () {
        return static_cast<int>(end_time_.secsTo (start_time_));
    }

    //! Get the duration in milliseconds.
    int durationInMiliSeconds () {
        return static_cast<int>(end_time_.msecsTo (start_time_));
    }

    //! Start the program.
    void
    perform (const QStringList & input) {
        PROCRUNGUI_TRACE_ENTRY;

        // start the program
        this->start (QIODevice::ReadWrite);
        if (!this->waitForStarted()) {
            return;
        }

        // provide the input
        foreach (const QString & s, input) {
            this->write (s.toLatin1 ().constData ());
            // delay?
        }
        this->closeWriteChannel();

        PROCRUNGUI_TRACE_EXIT;
    }

    //! Tell if this process is running or not.
    bool
    isRunning () const {
        return state () != NotRunning;
    }

public slots:

    //! Some output coming out of error channel.
    void
    readyReadStandardErrorSlot () {
        PROCRUNGUI_TRACE_ENTRY;
        all_output_.append (
                    prg_->processGeneratedText (
                        this, readAllStandardError (), true));
        PROCRUNGUI_TRACE_EXIT;
    }

    //! Some output coming out of output channel.
    void
    readyReadStandardOutputSlot () {
        PROCRUNGUI_TRACE_ENTRY;
        all_output_.append (
                    prg_->processGeneratedText (
                        this, readAllStandardError (), false));
        PROCRUNGUI_TRACE_EXIT;
    }

    //! Connected to keep the started/not started state.
    void
    startedSlot () {
        PROCRUNGUI_TRACE_ENTRY;
        b_started_ = true;
        start_time_ = QDateTime::currentDateTime ();
        PROCRUNGUI_TRACE_EXIT;
    }

    //! Connected to keep the started/not started state.
    void
    finishedSlot (
            int /*exitCode*/,
            QProcess::ExitStatus /*exitStatus*/) {
        PROCRUNGUI_TRACE_ENTRY;
        // no need to cache them as they are available from QProcess
        end_time_ = QDateTime::currentDateTime ();
        if (kb_ != NULL) {
            kb_(prg_, this, user_data_);
        }
        prg_->finishProcess (this);

        PROCRUNGUI_TRACE_EXIT;
    }

    //! Track this slot to accumulate the list of states.
    void
    stateChangedSlot (
            QProcess::ProcessState newState) {
        PROCRUNGUI_TRACE_ENTRY;
        states_.append (newState);
        PROCRUNGUI_TRACE_EXIT;
    }

    //! Accumulate errors here.
    void
    errorSlot (QProcess::ProcessError error) {
        PROCRUNGUI_TRACE_ENTRY;
        errors_.append (error);
        PROCRUNGUI_TRACE_EXIT;
    }

public:
    ProcRunGui * prg_;
    QDateTime start_time_; /**< the time when the process was started */
    QDateTime end_time_; /**< the time when the process ended */
    QString all_output_; /**< the output through output and error channel */
    bool b_started_; /**< is the process already running? */
    QList<QProcess::ProcessError> errors_; /**< list of errors */
    QList<QProcess::ProcessState> states_; /**< list of states*/
    ProcRunGui::Kb kb_; /**< function to be called when the process ends */
    void * user_data_; /**< opaque user data */
    QLabel * widget_; /**< associated widget */
    bool close_on_exit_; /**< should this process close its tab on exit? */
};

#include "procrungui.moc"


/**
 * @class ProcRunGui
 *
 * The class represents a widget capable of presenting the output of
 * multiple processes, with the user being able to switch between then by
 * using a tab-based approach.
 *
 * The name of each executable is used by default for the name of the tab
 * and the icon associated with that process is also used if available.
 *
 * The child process may be closed mid-way.
 *
 * The output coming out of the error channel is colored differently
 * than the one coming out of standard output channel.
 */

/* ------------------------------------------------------------------------- */
/**
 */
ProcRunGui::ProcRunGui(QWidget * parent) :
    QWidget(parent),
    ui(new Ui::ProcRunGui ()),
    processes_(),
    close_on_last_(true),
    autoclose_finished_(false),
    running_mov_(),
    cmdmodl_(NULL),
    item_in_form_(NULL),
    b_list_lock_(false)
{
    PROCRUNGUI_TRACE_ENTRY;
    ui->setupUi (this);

    QListWidgetItem * it;
    it = new QListWidgetItem (
        qApp->style()->standardIcon (
                    QStyle::SP_ArrowRight),
                tr ("Add new argument"));
    it->setData (Qt::UserRole, it->text());
    it->setFlags (it->flags() |
                  Qt::ItemIsEditable |
                  Qt::ItemIsEnabled |
                  Qt::ItemIsSelectable);
    ui->argumentsListWidget->addItem (it);

    it = new QListWidgetItem (
        qApp->style()->standardIcon (
                    QStyle::SP_ArrowRight),
                tr ("Add new input"));
    it->setData (Qt::UserRole, it->text());
    it->setFlags (it->flags() |
                  Qt::ItemIsEditable |
                  Qt::ItemIsEnabled |
                  Qt::ItemIsSelectable);
    ui->inputListWidget->addItem (it);

    startTimer (100);
    loadCommands ();
    PROCRUNGUI_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 */
ProcRunGui::~ProcRunGui()
{
    PROCRUNGUI_TRACE_ENTRY;
    delete ui;
    PROCRUNGUI_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
PrgProcess *ProcRunGui::runProgram (
        const QString & s_program, const QStringList &sl_args,
        const QString s_crt_path, const QStringList &sl_input,
        ProcRunGui::Kb kb, void *user_data)
{
    PROCRUNGUI_TRACE_ENTRY;
    PrgProcess * result = new PrgProcess (this, kb, user_data);
    processes_.append (result);
    result->setProgram (s_program);
    result->setArguments (sl_args);
    result->setWorkingDirectory (s_crt_path);

    result->widget_ = new QLabel ();
    result->widget_->setText (tr ("%1> %2")
                             .arg (s_program)
                             .arg (sl_args.join (QChar (' '))));

    QFileInfo fl (s_program);
    ui->tabWidget->addTab (result->widget_, QIcon(), fl.baseName ());
    result->perform (sl_input);

    PROCRUNGUI_TRACE_EXIT;
    return result;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
int ProcRunGui::programIndex (PrgProcess *prg)
{
    return processes_.indexOf (prg);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
PrgProcess * ProcRunGui::program (int idx)
{
    return processes_.at (idx);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
static QString defaultDataFile ()
{
    QString s_input = QStandardPaths::writableLocation (
                QStandardPaths::AppDataLocation);
    QDir dr (s_input);
    dr.mkpath (QLatin1String ("."));
    if (!dr.exists()) {
        PROCRUNGUI_DEBUGM("Failed to create application data directory\n");
        return QString ();
    }

    s_input = dr.absoluteFilePath (QLatin1String ("proc_run_gui_commands.ini"));

    return s_input;
}
/* ========================================================================= */

#define STG_TOP_CONTAINER "ProcRunGui"
#define STG_CONTAINER "Container"
#define STG_VAL_TYPE "EntryType"

/* ------------------------------------------------------------------------- */
bool ProcRunGui::loadCommands(const QString &s_file)
{
    PROCRUNGUI_TRACE_ENTRY;
    bool b_ret = false;
    for (;;) {
        // Default to a file in application data directory.
        QString s_input = s_file;
        if (s_file.isEmpty()) {
            s_input = defaultDataFile ();
            if (s_file.isEmpty()) {
                break;
            }
        }

        // Make sure the file exists.
        if (!QFile (s_input).exists ()) {
            PROCRUNGUI_DEBUGM("Input file %s does not exists\n", TMP_A(s_input));
            break;
        }
        PROCRUNGUI_DEBUGM("ProcRunGui loads commands from file %s\n",
                          TMP_A(s_input));

        // Load the file using settings
        QSettings stg (s_input, QSettings::IniFormat);

        b_ret = loadCommands (stg);
        break;
    }

    PROCRUNGUI_TRACE_EXIT;
    return b_ret;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
bool ProcRunGui::loadCommands (QSettings & stg)
{
    PROCRUNGUI_TRACE_ENTRY;
    bool b_ret = false;
    for (;;) {

        ProcRunModel * mdl = new ProcRunModel (this);
        b_ret = mdl->load (stg);
        setCmdModel (mdl);

        break;
    }
    PROCRUNGUI_TRACE_EXIT;
    return b_ret;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::setCmdModel (ProcRunModel * mdl)
{
    if (cmdmodl_ != NULL) {
        delete cmdmodl_;
    }
    cmdmodl_ = mdl;
    ui->treeView->setModel (cmdmodl_);
    connect (ui->treeView->selectionModel(), &QItemSelectionModel::currentRowChanged,
             this, &ProcRunGui::treeviewSelectionChanged);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
ProcRunItemBase *ProcRunGui::selectedCmdEntry ()
{
    ProcRunItemBase * result = NULL;
    int index = -1;
    QItemSelectionModel * slc = ui->treeView->selectionModel ();
    if (slc != NULL) {
        QModelIndex mi = slc->currentIndex();
        if (mi.isValid()) {
            result = cmdmodl_->itemFromIndex (mi);
        }
    }
    return result;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
bool ProcRunGui::saveCommands(const QString &s_file)
{
    PROCRUNGUI_TRACE_ENTRY;
    bool b_ret = false;
    for (;;) {
        // Default to a file in application data directory.
        QString s_input = s_file;
        if (s_file.isEmpty()) {
            s_input = defaultDataFile ();
            if (s_file.isEmpty()) {
                break;
            }
        }

        PROCRUNGUI_DEBUGM("ProcRunGui saves commands to file %s\n",
                          TMP_A(s_input));

        // Load the file using settings
        QSettings stg (s_input, QSettings::IniFormat);

        b_ret = saveCommands (stg);

        break;
    }
    PROCRUNGUI_TRACE_EXIT;
    return b_ret;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
bool ProcRunGui::saveCommands(QSettings &s_data)
{
    PROCRUNGUI_TRACE_ENTRY;
    bool b_ret = false;
    if (cmdmodl_ != NULL) {
        b_ret = cmdmodl_->save (s_data);
    }
    PROCRUNGUI_TRACE_EXIT;
    return b_ret;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::closeEvent (QCloseEvent * ev)
{
    saveCommands ();
    int cnt = ui->tabWidget->count() - 1;
    for (int i = cnt; i >= 0; --i) {
        if (!on_tabWidget_tabCloseRequested (i)) {
            ev->ignore ();
            return;
        }
    }

    emit aboutToClose ();

    if (ui->tabWidget->count() > 0) {
        QThread::msleep (500);
    }

    qDeleteAll (processes_);

    ev->accept ();
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::timerEvent (QTimerEvent *)
{
    int cnt = ui->tabWidget->count() - 1;
    QIcon ic (running_mov_.currentPixmap());
    for (int i = cnt; i >= 0; --i) {
        PrgProcess * proc = processes_.at (i);
        assert(ui->tabWidget->widget(i) == proc->widget_);
        if (proc->isRunning ()) {
            ui->tabWidget->setTabIcon (i, ic);
        }
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
QString ProcRunGui::processGeneratedText (
        PrgProcess * proc, const QString &s_input, bool is_error)
{
    QString s_prefix;
    QString s_suffix;
    if (is_error) {
        s_prefix = QLatin1String ("<red>");
        s_suffix = QLatin1String ("</red>");
    } else {
        // ..
    }

    QString s_result = s_prefix;
    QLatin1String nl ("<br />\n");
    foreach(const QString s_line, s_input.toHtmlEscaped ().split (QChar('\n'))) {
        s_result.append (s_line);
        s_result.append (nl);
    }
    s_result.append (s_suffix);

    if (proc->widget_ == ui->tabWidget->currentWidget()) {
        ui->plainTextEdit->appendHtml (s_result);
    }

    return s_result;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::finishProcess (PrgProcess *proc)
{
    if (autoclose_finished_ || proc->close_on_exit_) {
        processDone (proc);
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::processDone (PrgProcess *proc, int idx)
{
    if (idx == -1) {
        idx = programIndex (proc);
        if (idx == -1)
            return;
    } else if (proc == NULL) {
        proc = processes_.at (idx);
    }

    assert(ui->tabWidget->widget(idx) == proc->widget_);
    ui->tabWidget->removeTab (idx);
    delete proc;
    processes_.removeAt (idx);

    if (processes_.isEmpty() && close_on_last_) {
        close ();
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::on_tabWidget_currentChanged (int index)
{
    if (index == -1) {
        ui->plainTextEdit->clear ();
    } else {
        PrgProcess * prc = program (index);
        assert(ui->tabWidget->widget (index) == prc->widget_);
        ui->plainTextEdit->clear ();
        ui->plainTextEdit->appendHtml (prc->all_output_);
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
bool ProcRunGui::on_tabWidget_tabCloseRequested (int index)
{
    PrgProcess * prc = program (index);
    assert(ui->tabWidget->widget (index) == prc->widget_);

    if (prc->isRunning ()) {
        int res = QMessageBox::question (
                    this,
                    tr("Terminate process?"),
                    tr("Are you sure you want to kill the process?\n"
                       "The process is given no chance to exit gracefully "
                       "and unsaved data may be lost."),
                    QMessageBox::Yes, QMessageBox::Cancel);
        if (res == QMessageBox::Yes) {
            prc->close_on_exit_ = true;
            prc->kill ();
        } else {
            return false;
        }
    } else {
        processDone (prc, index);
    }

    return true;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::on_buttonBox_clicked (QAbstractButton *button)
{
    if (ui->buttonBox->button (QDialogButtonBox::Reset) == button) {
        item_in_form_ = NULL;
        clearProgForm ();
    } else if (ui->buttonBox->button (QDialogButtonBox::Save) == button) {
        saveProgramForm ();
    } else if (ui->buttonBox->button (QDialogButtonBox::Ok) == button) {
        ui->stackedWidget->setCurrentIndex (0);
    } else if (ui->buttonBox->button (QDialogButtonBox::Cancel) == button) {
        ui->stackedWidget->setCurrentIndex (0);
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::on_runButton_clicked()
{
    ui->stackedWidget->setCurrentIndex (1);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::clearProgForm ()
{
    ui->programLineEdit->clear ();
    while (ui->argumentsListWidget->count() > 1) {
        if (ui->argumentsListWidget->item (0)->icon().isNull()) {
            delete ui->argumentsListWidget->takeItem (0);
        }
    }
    while (ui->inputListWidget->count() > 1) {
        if (ui->inputListWidget->item (0)->icon().isNull()) {
            delete ui->inputListWidget->takeItem (0);
        }
    }
    ui->wrkDirLineEdit->clear ();
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::saveProgramForm ()
{
    if (cmdmodl_ == NULL) {
        setCmdModel (new ProcRunModel (this));
    }

    if (item_in_form_ == NULL) {
        item_in_form_ = new ProcRunItem ();
        ProcRunGroup * gr = NULL;
        int index = -1;
        QItemSelectionModel * slc = ui->treeView->selectionModel ();
        if (slc != NULL) {
            QModelIndex mi = slc->currentIndex();
            if (mi.isValid()) {
                ProcRunItemBase * crtit = cmdmodl_->itemFromIndex (mi);
                if (crtit->type () == ProcRunItemBase::CommandType) {
                    gr = crtit->parent ();
                    index = gr->indexOf (crtit) + 1;
                } else if (crtit->type () == ProcRunItemBase::GroupType) {
                    gr = static_cast<ProcRunGroup*> (crtit);
                }
            }
        }
        cmdmodl_->insertItem (item_in_form_, index, gr);
    }

    item_in_form_->s_program_ = ui->programLineEdit->text ();

    item_in_form_->sl_arguments_.clear ();
    int i_max = ui->argumentsListWidget->count ();
    for (int i = 0; i < i_max; ++i) {
        QListWidgetItem * li = ui->argumentsListWidget->item (i);
        item_in_form_->sl_arguments_.append (li->text ());
    }

    item_in_form_->sl_input_.clear ();
    i_max = ui->inputListWidget->count ();
    for (int i = 0; i < i_max; ++i) {
        QListWidgetItem * li = ui->inputListWidget->item (i);
        item_in_form_->sl_input_.append (li->text ());
    }

    item_in_form_->s_wrk_dir_= ui->wrkDirLineEdit->text ();

    cmdmodl_->itemChanged (item_in_form_);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::loadProgramForm (ProcRunItem * item)
{
    if (item == NULL)
        return;
    if (item_in_form_ == item)
        return;
    clearProgForm ();

    ui->programLineEdit->setText (item->s_program_);
    ui->argumentsListWidget->addItems (item->sl_arguments_);
    ui->inputListWidget->addItems (item->sl_input_);
    ui->wrkDirLineEdit->setText (item->s_wrk_dir_);

    item_in_form_ = item;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::on_terminateButton_clicked()
{
    int index = ui->tabWidget->currentIndex();
    PrgProcess * prc = program (index);
    DBG_ASSERT(ui->tabWidget->widget (index) == prc->widget_);

    if (prc->isRunning ()) {
        prc->close_on_exit_ = true;
        prc->terminate ();
    } else {
        processDone (prc, index);
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::on_killButton_clicked()
{
    on_tabWidget_tabCloseRequested (ui->tabWidget->currentIndex());
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::addNewGroup ()
{
    if (cmdmodl_ == NULL) {
        setCmdModel (new ProcRunModel (this));
    }

    ProcRunGroup * item = new ProcRunGroup (
                tr ("New group"));
    ProcRunGroup * gr = NULL;
    int index = -1;
    QItemSelectionModel * slc = ui->treeView->selectionModel ();
    if (slc != NULL) {
        QModelIndex mi = slc->currentIndex();
        if (mi.isValid()) {
            ProcRunItemBase * crtit = cmdmodl_->itemFromIndex (mi);
            if (crtit->type () == ProcRunItemBase::CommandType) {
                gr = crtit->parent ();
                index = gr->indexOf (crtit) + 1;
            } else if (crtit->type () == ProcRunItemBase::GroupType) {
                gr = static_cast<ProcRunGroup*> (crtit);
            }
        }
    }
    cmdmodl_->insertItem (item, index, gr);

    ui->treeView->edit (cmdmodl_->indexFromItem (item));
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::on_treeView_customContextMenuRequested (const QPoint &pos)
{
    QMenu mnu;
    QAction act_new_folder (
                qApp->style()->standardIcon (QStyle::SP_DirIcon),
                tr("Add new group"), this);
    mnu.addAction (&act_new_folder);
    QAction act_remove (
                qApp->style()->standardIcon (QStyle::SP_BrowserStop),
                tr("Delete"), this);
    mnu.addAction (&act_remove);


    QAction * result = mnu.exec (ui->treeView->viewport()->mapToGlobal (pos));
    if (result == &act_new_folder) {
        addNewGroup ();
    } else if (result == &act_remove) {
        removeItem (selectedCmdEntry ());
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::removeItem (ProcRunItemBase * item)
{
    if (item == NULL)
        return;
    if (cmdmodl_ == NULL) {
        return;
    }

    int res = QMessageBox::question (
                this,
                tr("Delete saved command?"),
                tr("Are you sure you want to delete this entry?\n"
                   "Once deleted you will have no way to undo\n"
                   "this operation."),
                QMessageBox::Yes, QMessageBox::Cancel);
    if (res == QMessageBox::Yes) {
        cmdmodl_->removeItem (item);
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::on_argumentsListWidget_itemChanged (QListWidgetItem *item)
{
    if (b_list_lock_)
        return;
    b_list_lock_ = true;
    if (!item->icon().isNull()) {
        QListWidgetItem * li = new QListWidgetItem (item->text ());
        li->setFlags (li->flags() |
                      Qt::ItemIsEditable |
                      Qt::ItemIsEnabled |
                      Qt::ItemIsSelectable);
        ui->argumentsListWidget->insertItem (
                    ui->argumentsListWidget->count() - 2, li);
        item->setText (item->data (Qt::UserRole).toString ());
    }
    b_list_lock_ = false;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::on_inputListWidget_itemChanged (QListWidgetItem *item)
{
    if (b_list_lock_)
        return;
    b_list_lock_ = true;
    if (!item->icon().isNull()) {
        QListWidgetItem * li = new QListWidgetItem (item->text ());
        li->setFlags (li->flags() |
                      Qt::ItemIsEditable |
                      Qt::ItemIsEnabled |
                      Qt::ItemIsSelectable);
        ui->inputListWidget->insertItem (
                    ui->inputListWidget->count() - 2, li);
        item->setText (item->data (Qt::UserRole).toString ());
    }
    b_list_lock_ = false;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::treeviewSelectionChanged (
        const QModelIndex &current, const QModelIndex &)
{
    if (!current.isValid()) {
        return;
    }
    if (cmdmodl_ == NULL) {
        setCmdModel (new ProcRunModel (this));
    }
    ProcRunItemBase * it = cmdmodl_->itemFromIndex (current);
    if (it == NULL) {
        return;
    }

    if (it->type () == ProcRunItemBase::CommandType) {
        loadProgramForm (static_cast<ProcRunItem*>(it));
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::on_programButton_clicked()
{
    QFileDialog dialog (this);
    dialog.setFileMode (QFileDialog::ExistingFile);
    dialog.setViewMode (QFileDialog::Detail);

    // filters
    QStringList filters;
    filters.append (QLatin1String ("Programs (*.exe)"));
    filters.append (trUtf8 ("All Files (*)"));
    dialog.setNameFilters (filters);

    dialog.selectFile (
                QDir::fromNativeSeparators (
                    ui->programLineEdit->text ()));

    if (dialog.exec()) {
        if (dialog.selectedFiles().count() == 1) {
            ui->programLineEdit->setText (QDir::toNativeSeparators (
                        dialog.selectedFiles ().at (0)));
        }
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcRunGui::on_wrkDirButton_clicked()
{
    QFileDialog dialog (this);
    dialog.setFileMode (QFileDialog::Directory);
    dialog.setViewMode (QFileDialog::Detail);
    dialog.setOption (QFileDialog::ShowDirsOnly, true);
    QString s_path = ui->wrkDirLineEdit->text ();
    dialog.selectFile (QDir::fromNativeSeparators (s_path));

    if (dialog.exec()) {
        if (dialog.selectedFiles().count() == 1) {
            ui->wrkDirLineEdit->setText (
                        QDir::toNativeSeparators (
                            dialog.selectedFiles ().at (0)));
        }
    }
}
/* ========================================================================= */
