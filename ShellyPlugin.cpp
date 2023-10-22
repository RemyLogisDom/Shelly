#include <QtWidgets>

#include "ShellyPlugin.h"
#include "../common.h"


ShellyPlugin::ShellyPlugin(QWidget *parent) : QWidget(parent)
{
    ui = new QWidget();
    QGridLayout *layout = new QGridLayout(ui);
    QWidget *w = new QWidget();
    mui = new Ui::ShellyUI;
    mui->setupUi(w);
    layout->addWidget(w);
    mui->logTxt->hide();
    mui->checkBoxWr10->setEnabled(false);
    mui->treeWidget->hide();
    mui->gridLayout_3->removeWidget(mui->treeWidget);

    mui->deviceTable->setColumnCount(4);
    mui->deviceTable->setHorizontalHeaderItem(0, new QTableWidgetItem("IP Address"));
    mui->deviceTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Read Interval"));
    mui->deviceTable->setHorizontalHeaderItem(2, new QTableWidgetItem("Model"));
    mui->deviceTable->setHorizontalHeaderItem(3, new QTableWidgetItem("Last Read"));
    mui->pushButtonAddParameter->setEnabled(false);

    connect(mui->checkBoxLog, SIGNAL(stateChanged(int)), this, SLOT(logChanged()));
    connect(mui->checkBoxWr10, SIGNAL(stateChanged(int)), this, SLOT(logChanged()));
    connect(mui->pushButtonClearLog, SIGNAL(clicked()), this, SLOT(clearLog()));
    connect(mui->editName, SIGNAL(editingFinished()), this, SLOT(on_editName_editingFinished()));
    connect(mui->deviceTable, SIGNAL(cellPressed(int, int)),SLOT(displayDevice(int, int)));
    connect(mui->ReadButton, SIGNAL(clicked()), this, SLOT(on_ReadButton_clicked()));
    connect(mui->ReadAllButton, SIGNAL(clicked()), this, SLOT(on_ReadAllButton_clicked()));
    connect(mui->RemoveButton, SIGNAL(clicked()), this, SLOT(on_RemoveButton_clicked()));
    connect(mui->pushButtonLock, SIGNAL(clicked()), this, SLOT(setLockedState()));
    connect(mui->deviceTable, SIGNAL(itemSelectionChanged()), this, SLOT(deviceTableSelection()));

    connect(mui->AddDevice, SIGNAL(clicked()), this, SLOT(AddDeviceClick()));
    connect(mui->pushButtonAddParameter, SIGNAL(clicked()), this, SLOT(AddParameterClick()));
    connect(&readTimer, SIGNAL(timeout()),SLOT(readAllNow()));
    readTimer.start(1000);
}


ShellyPlugin::~ShellyPlugin()
{
}


QObject *ShellyPlugin::getObject()
{
    return this;
}


QWidget *ShellyPlugin::widgetUi()
{
    return ui;
}

QWidget *ShellyPlugin::getDevWidget(QString)
{
    return nullptr;
}

void ShellyPlugin::setConfigFileName(const QString fileName)
{
    configFileName = fileName;
    configFileName.chop(3);
    configFileName.append(".cfg");
    mui->labelInterfaceName->setToolTip(configFileName);
}


QString ShellyPlugin::getDeviceCommands(const QString &)
{
    return "on=" + tr("On") + "|off=" + tr("Off");
}


void ShellyPlugin::saveConfig()
{
    QFile file(configFileName);
    QSettings settings(configFileName, QSettings::IniFormat);
    settings.clear();
    settings.setValue("Interface_Name", mui->editName->text() );
    int devCount = 0;
    foreach (shellyMainDevice *Shellydev, shellyMainDevices) {
        int devIndex = 0;
        QString data;
        QString devIDstr = QString("/Device_%1").arg(devCount++, 3, 10, QChar('0'));
        data.append(Shellydev->address->text() + "/");    // IP Address
        data.append(Shellydev->readInterval->currentText());    // Read Interval
        settings.setValue(devIDstr, data );
        foreach (ShellyDevice *dev, Shellydev->ShellyDevices) {
            QString devID = QString("/Device_%1/Parameter_%2").arg(devCount, 3, 10, QChar('0')).arg(++devIndex, 3, 10, QChar('0'));
            QString s = dev->pathLocation;
            settings.setValue(devID + "/pathLocation",s.replace("/", "*"));
            settings.setValue(devID + "/maxValue", dev->maxValue_item.text());
            settings.setValue(devID + "/Command", dev->command_item.text());
            settings.setValue(devID + "/ReadBack", dev->readback_item.text());
        }
    }
}


void ShellyPlugin::readConfig()
{
    QSettings settings(configFileName, QSettings::IniFormat);
    QString Name = settings.value("Interface_Name").toString();
    if (!Name.isEmpty()) mui->editName->setText(Name);
    int devID = 0;
    QString devIDstr = QString("/Device_%1").arg(devID++, 3, 10, QChar('0'));
    QString devSettings = settings.value(QString(devIDstr)).toString();
    while (!devSettings.isEmpty()) {
        //qDebug() << devSettings;
        devSettings = settings.value(QString(devIDstr)).toString();
        QStringList parameters = devSettings.split("/");
        if (parameters.count() == 2) {
            shellyMainDevice *dev = addDevice();
            if (dev) {
                dev->address->setText(parameters.at(0));
                dev->readInterval->setCurrentText(parameters.at(1));
                dev->readDevice();
            }
            int devIndex = 0;
        // get parameters
            QString devParameter = QString("/Device_%1/Parameter_%2").arg(devID, 3, 10, QChar('0')).arg(++devIndex, 3, 10, QChar('0'));
            QString devParamterPathLocation = settings.value(QString(devParameter + "/pathLocation")).toString().replace("*", "/");
            while (!devParamterPathLocation.isEmpty()) {
                ShellyDevice *shellyDev = dev->addShellyDevice(devParamterPathLocation);
                QString devParamterMaxValue = settings.value(QString(devParameter + "/maxValue")).toString();
                shellyDev->maxValue_item.setText(devParamterMaxValue);
                QString devCommand = settings.value(QString(devParameter + "/Command")).toString();
                shellyDev->command_item.setText(devCommand);
                QString devReadBack = settings.value(QString(devParameter + "/ReadBack")).toString();
                shellyDev->readback_item.setText(devReadBack);
                devParameter = QString("/Device_%1/Parameter_%2").arg(devID, 3, 10, QChar('0')).arg(++devIndex, 3, 10, QChar('0'));
                devParamterPathLocation = settings.value(QString(devParameter + "/pathLocation")).toString().replace("*", "/");
            }
        }
        devIDstr = QString("/Device_%1").arg(devID++, 3, 10, QChar('0'));
        devSettings = settings.value(QString(devIDstr)).toString();
    }
    emit(updateInterfaceName(this, Name));
}



void ShellyPlugin::clearLog()
{
    mui->logTxt->clear();
}


void ShellyPlugin::logChanged()
{
    foreach (shellyMainDevice *dev, shellyMainDevices) {
        disconnect(dev, SIGNAL(logMe(const QString &)), this, SLOT(logThis(const QString &)));
        disconnect(dev, SIGNAL(logCommand(const QString &)), this, SLOT(logThis(const QString &)));
    }
    if (mui->checkBoxLog->isChecked()) {
        mui->logTxt->show();
        mui->checkBoxWr10->setEnabled(true);
        foreach (shellyMainDevice *dev, shellyMainDevices) connect(dev, SIGNAL(logCommand(const QString &)), this, SLOT(logThis(const QString &)));
        if (!mui->checkBoxWr10->isChecked()) {
            foreach (shellyMainDevice *dev, shellyMainDevices) connect(dev, SIGNAL(logMe(const QString &)), this, SLOT(logThis(const QString &)));
        }
    }
    else {
        mui->logTxt->hide();
        mui->checkBoxWr10->setEnabled(false);
    }
}



void ShellyPlugin::deviceTableSelection()
{
    QTableWidgetItem *item = mui->deviceTable->currentItem();
    if (item == nullptr) mui->RemoveButton->setEnabled(false);
    else {
        if (item->isSelected()) mui->RemoveButton->setEnabled(!lockedState);
        else  mui->RemoveButton->setEnabled(false);
    }
}



void ShellyPlugin::setLockedState()
{
    lockedState = !lockedState;
    setLockedState(lockedState);
}


void ShellyPlugin::setLockedState(bool state)
{
    mui->AddDevice->setEnabled(!state);
    QTableWidgetItem *item = mui->deviceTable->currentItem();
    if (item == nullptr) mui->RemoveButton->setEnabled(false);
    else {
        if (item->isSelected()) mui->RemoveButton->setEnabled(!state);
        else  mui->RemoveButton->setEnabled(false);
    }
    mui->editName->setEnabled(!state);
    for (int n=0; n<mui->deviceTable->rowCount(); n++) {
        QTableWidgetItem *item = mui->deviceTable->item(n, 0);
        if (item) {
        if (state) item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        else item->setFlags(item->flags() | Qt::ItemIsEditable); }
        foreach (shellyMainDevice *dev, shellyMainDevices) {
        dev->readInterval->setEnabled(!state);
        }
    }
    //mui->pushButtonAddParameter->setEnabled(!lockedState);
    lockedState = state;
    if (state) {
        mui->pushButtonLock->setIcon(QIcon(QPixmap(QString::fromUtf8(":/images/lock.png"))));
    }
    else
    {
        mui->pushButtonLock->setIcon(QIcon(QPixmap(QString::fromUtf8(":/images/unlock.png"))));
    }
    QModelIndex index = mui->deviceTable->currentIndex();
    if (index.isValid()) displayDevice(index.row(), 0);
}


QString ShellyPlugin::getDeviceConfig(QString)
{
    return "";
}


void ShellyPlugin::setDeviceConfig(const QString &, const QString &)
{
}


QString ShellyPlugin::getName()
{
    return mui->editName->text();
}



double ShellyPlugin::getMaxValue(const QString)
{
    return 1;
}


bool ShellyPlugin::isManual(const QString)
{
    return false;
}


bool ShellyPlugin::isDimmable(const QString)
{
    return false;
}


bool ShellyPlugin::acceptCommand(const QString RomID)
{
    foreach (shellyMainDevice *devMain, shellyMainDevices)
    {
        foreach (ShellyDevice *dev, devMain->ShellyDevices) {
        if (dev->RomID == RomID) {
                if (dev->command_item.text().isEmpty()) return false; else return true; }
        }
    }
    return false;
}


void ShellyPlugin::setStatus(const QString status)
{
    log("setStatus : " + status);
    QStringList split = status.split("=");
    QString RomID = split.first();
    QString command = split.last();
    if (split.count() != 2) return;
    foreach (shellyMainDevice *devMain, shellyMainDevices)
    {
        foreach (ShellyDevice *dev, devMain->ShellyDevices) {
        if (dev->RomID == RomID) {
                devMain->setCommand(dev, command);
                return; }
        }
    }
}



void ShellyPlugin::log(const QString str)
{
    if (mui->checkBoxLog->isChecked())
    {
        if (logStr.length() > 100000) logStr = logStr.mid(90000);
        mui->logTxt->append(str);
        mui->logTxt->moveCursor(QTextCursor::End);
    }
}




void ShellyPlugin::AddDeviceClick()
{
    addDevice();
}

QString ShellyPlugin::selectedPath()
{
    int index = mui->deviceTable->currentRow();
    if (index != -1) {
        QString path;
        if (shellyMainDevices.at(index)->deviceCommand) return "";
        QModelIndex md = shellyMainDevices.at(index)->treeView.currentIndex();
        if (md.column() != 0) md = md.sibling(md.row(), 0);
        if (md.isValid()) {
        path = md.data(Qt::DisplayRole).toString();
        while(md.parent().isValid()) {
                path = md.parent().data().toString() + "/" + path;
                md = md.parent();
        }
        // check if device exists :
        bool found = false;
        foreach (ShellyDevice *dev, shellyMainDevices.at(index)->ShellyDevices)
        {
                if (dev->pathLocation == path) found = true;
        }
        if (!found) return path;
        } }
        return "";
}


void ShellyPlugin::pathSelected()
{
    QString path = selectedPath();
    if (!path.isEmpty())
    {
        mui->pushButtonAddParameter->setEnabled(true);
        mui->pushButtonAddParameter->setToolTip(path);
    }
    else
    {
        mui->pushButtonAddParameter->setEnabled(false);
        mui->pushButtonAddParameter->setToolTip("");
    }
}


void ShellyPlugin::AddParameterClick()
{
        QString path = selectedPath();
        int index = mui->deviceTable->currentRow();
        if (index != -1)
        {
            shellyMainDevices.at(index)->addShellyDevice(path);
            mui->pushButtonAddParameter->setEnabled(false);
            mui->pushButtonAddParameter->setToolTip("");
        }
}


shellyMainDevice *ShellyPlugin::addDevice()
{
    // create new device
    shellyMainDevice *newDev = new shellyMainDevice();
    if (newDev) {
        int count = shellyMainDevices.count();
        shellyMainDevices.append(newDev);
        mui->deviceTable->insertRow(count);
        mui->deviceTable->setItem(count, 0, newDev->address);
        mui->deviceTable->setCellWidget(count, 1, newDev->readInterval);
        mui->deviceTable->setItem(count, 2, newDev->model);
        mui->deviceTable->setItem(count, 3, newDev->lastRead);
        connect(newDev, SIGNAL(newDevice(const ShellyDevice*)), this, SLOT(newShellyDevice(const ShellyDevice*)));
        connect(newDev, SIGNAL(newDeviceValue(const ShellyDevice*)), this, SLOT(newShellyDeviceValue(const ShellyDevice*)));
        connect(newDev, SIGNAL(showDevice(const ShellyDevice*)), this, SLOT(showDevice(const ShellyDevice*)));
        connect(&newDev->treeView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(pathSelected()));
        return newDev;
    }
    return nullptr;
}



void ShellyPlugin::on_editName_editingFinished()
{
    emit(updateInterfaceName(this, mui->editName->text()));
}


void ShellyPlugin::on_ReadButton_clicked()
{
    int index = mui->deviceTable->currentRow();
    if (index != -1) {
        QList<shellyMainDevice*> shellyDev;
        shellyDev.append(shellyMainDevices.at(index));
        readDevices(shellyDev);
    }
}


void ShellyPlugin::on_ReadAllButton_clicked()
{
    readDevices(shellyMainDevices);
}


void ShellyPlugin::on_RemoveButton_clicked()
{
    int index = mui->deviceTable->currentRow();
    if (index != -1) {
        //shellyMainDevices.at(index)->qnam.disconnect();
        mui->deviceTable->removeRow(index);
        shellyMainDevices.removeAt(index);
    }
}


void ShellyPlugin::newShellyDevice(const ShellyDevice* dev)
{
    bool ok;
    QString value = dev->value;
    value.toDouble(&ok);
    if (!ok) {
        if (value.contains("false", Qt::CaseInsensitive)) value = "0";
        else if (value.contains("true", Qt::CaseInsensitive)) value = "1";
    }
    emit(newDevice(this, dev->RomID));
    emit(newDeviceValue(dev->RomID, value));
}


void ShellyPlugin::newShellyDeviceValue(const ShellyDevice* dev)
{
    bool ok;
    QString value = dev->value;
    value.toDouble(&ok);
    if (!ok) {
        if (value.contains("false", Qt::CaseInsensitive)) value = "0";
        else if (value.contains("true", Qt::CaseInsensitive)) value = "1";
    }
    emit(newDeviceValue(dev->RomID, value));
}



void ShellyPlugin::showDevice(const ShellyDevice* dev)
{
    emit(deviceSelected(dev->RomID));
}


void ShellyPlugin::logThis(const QString &log)
{
    QString txt = mui->logTxt->toPlainText();
    txt.append(log);
    int l = txt.length();
    if (l > 10000) txt = txt.mid(l-10000, 10000);
    if (mui->checkBoxLog->isChecked()) {
        mui->logTxt->setText(txt);
        mui->logTxt->moveCursor(QTextCursor::End);
    }
}


void ShellyPlugin::readDevice(const QString &RomID)
{
/*    foreach (shellyMainDevice *devMain, shellyMainDevices)
    {
        foreach (ShellyDevice *dev, devMain->ShellyDevices) {
        if (dev->RomID == RomID) {
                devMain->readDevice(); return;
        } } }*/
}


void ShellyPlugin::readDevices(QList<shellyMainDevice*>& devList)
{
    foreach (shellyMainDevice *dev, devList) {
        dev->readDevice();
    }
}



void ShellyPlugin::readAllNow()
{
    if (lastMinute < 0) {
        readDevices(shellyMainDevices);
        lastMinute = QDateTime::currentDateTime().time().minute();
    }
    QList<shellyMainDevice*> devList;
    int m = QDateTime::currentDateTime().time().minute();
    if (m != lastMinute)
    {
        foreach (shellyMainDevice *dev, shellyMainDevices) {
            switch (dev->readInterval->currentIndex())
            {
                case read1mn : devList.append(dev); break;
                case read2mn : if (m % 2 == 0) devList.append(dev); break;
                case read5mn : if (m % 5 == 0) devList.append(dev); break;
                case read10mn : if (m % 10 == 0) devList.append(dev); break;
                case read30mn : if (m % 30 == 0) devList.append(dev); break;
                case read1hour : if (m == 0) devList.append(dev); break;
            }
        }
        lastMinute = m;
    }
    readDevices(devList);
}


void ShellyPlugin::displayDevice(int row, int)
{
    foreach (shellyMainDevice *dev, shellyMainDevices) {
        mui->gridLayout_3->removeWidget(&dev->treeView);
        dev->treeView.hide();
        dev->deviceView.hide();
    }
    if (lockedState) {
        mui->gridLayout_3->addWidget(&shellyMainDevices.at(row)->deviceView, 3, 0, 1, 3);
        shellyMainDevices.at(row)->deviceView.show();
    }
    else {
        mui->gridLayout_3->addWidget(&shellyMainDevices.at(row)->treeView, 3, 0, 1, 3);
        shellyMainDevices.at(row)->treeView.show();
    }

}
