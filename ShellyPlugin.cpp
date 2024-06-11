#include <QtWidgets>

#include "ShellyPlugin.h"
#include "simplecrypt.h"
#include "../common.h"


ShellyPlugin::ShellyPlugin(QWidget *parent) : QWidget(parent)
{
    ui = new QWidget();
    QGridLayout *layout = new QGridLayout(ui);
    QWidget *w = new QWidget();
    mui = new Ui::ShellyUI;
    mui->setupUi(w);
    layout->addWidget(w);
    mui->logTxtHttp->hide();
    mui->logTxtMqtt->hide();
    mui->checkBoxWrite->setEnabled(false);
    mui->httpTreeWidget->hide();
    mui->gridLayout_3->removeWidget(mui->httpTreeWidget);
    jsonmodel = new QJsonModel;
    mqttTree = new QTreeView(mui->frame);
    mui->gridLayout_5->removeWidget(mui->tableWidgetMqtt);
    mui->tableWidgetMqtt->hide();
    mui->gridLayout_5->addWidget(mqttTree, 0, 0, 1, 1);
    mqttTree->setModel(jsonmodel);
    m_client = new QMqttClient(this);
    m_client->setHostname(mui->lineEditHost->text());
    m_client->setPort(static_cast<quint16>(mui->spinBoxPort->value()));

    mui->buttonSubscribe->setEnabled(false);
    mui->buttonPublish->setEnabled(false);

    connect(m_client, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
        const QString content = QDateTime::currentDateTime().toString() + " Received Topic : " + topic.name() + " Payload : " + message + u'\n';
        mqttLogThis(content);
    });

    mui->httpDeviceTable->setColumnCount(5);
    mui->httpDeviceTable->setHorizontalHeaderItem(0, new QTableWidgetItem("IP Address"));
    mui->httpDeviceTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Name"));
    mui->httpDeviceTable->setHorizontalHeaderItem(2, new QTableWidgetItem("Read Interval"));
    mui->httpDeviceTable->setHorizontalHeaderItem(3, new QTableWidgetItem("Model"));
    mui->httpDeviceTable->setHorizontalHeaderItem(4, new QTableWidgetItem("Last Read"));
    mui->pushButtonAddHttpParameter->setEnabled(false);
    mui->pushButtonAddMqttParameter->setEnabled(false);

    connect(mui->checkBoxHttpLog, SIGNAL(stateChanged(int)), this, SLOT(httpLogChanged()));
    connect(mui->checkBoxMqttLog, SIGNAL(stateChanged(int)), this, SLOT(mqttLogChanged()));
    connect(mui->checkBoxWrite, SIGNAL(stateChanged(int)), this, SLOT(httpLogChanged()));
    connect(mui->pushButtonClearHttpLog, SIGNAL(clicked()), this, SLOT(clearHttpLog()));
    connect(mui->pushButtonClearMqttLog, SIGNAL(clicked()), this, SLOT(clearMqttLog()));
    connect(mui->editName, SIGNAL(editingFinished()), this, SLOT(on_editName_editingFinished()));
    connect(mui->httpDeviceTable, SIGNAL(cellPressed(int, int)),this, SLOT(displayHttpDevice(int, int)));
    connect(mui->ReadButton, SIGNAL(clicked()), this, SLOT(on_ReadButton_clicked()));
    connect(mui->ReadAllButton, SIGNAL(clicked()), this, SLOT(on_ReadAllButton_clicked()));
    connect(mui->RemoveButton, SIGNAL(clicked()), this, SLOT(on_RemoveButton_clicked()));
    connect(mui->pushButtonLock, SIGNAL(clicked()), this, SLOT(setLockedState()));
    connect(mui->pushButtonLockMqtt, SIGNAL(clicked()), this, SLOT(setLockedState()));
    connect(mui->httpDeviceTable, SIGNAL(itemSelectionChanged()), this, SLOT(deviceTableSelection()));
    connect(mui->buttonConnect, SIGNAL(clicked()), this, SLOT(on_buttonConnect_clicked()));
    connect(mui->buttonPublish, SIGNAL(clicked()), this, SLOT(on_buttonPublish_clicked()));
    connect(mui->buttonSubscribe, SIGNAL(clicked()), this, SLOT(on_buttonSubscribe_clicked()));
    connect(mui->lineEditHost, &QLineEdit::textChanged, m_client, &QMqttClient::setHostname);
    connect(mui->spinBoxPort, QOverload<int>::of(&QSpinBox::valueChanged), this, &ShellyPlugin::setClientPort);
    connect(m_client, &QMqttClient::connected, this, &ShellyPlugin::brokerConnected);
    connect(m_client, &QMqttClient::disconnected, this, &ShellyPlugin::brokerDisconnected);
    connect(m_client, &QMqttClient::stateChanged, this, &ShellyPlugin::updateLogStateChange);
    connect(mqttTree, SIGNAL(clicked(const QModelIndex &)), this, SLOT(mqttPathSelected()));

    mui->tableWidgetMqtt->setColumnCount(9);
    mui->tableWidgetMqtt->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("RomID")));
    mui->tableWidgetMqtt->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Name")));
    mui->tableWidgetMqtt->setHorizontalHeaderItem(2, new QTableWidgetItem(tr("Path")));
    mui->tableWidgetMqtt->setHorizontalHeaderItem(3, new QTableWidgetItem(tr("Value")));
    mui->tableWidgetMqtt->setHorizontalHeaderItem(4, new QTableWidgetItem(tr("Last Read")));
    mui->tableWidgetMqtt->setHorizontalHeaderItem(5, new QTableWidgetItem(tr("Command")));
    mui->tableWidgetMqtt->setHorizontalHeaderItem(6, new QTableWidgetItem(tr("Payload")));
    mui->tableWidgetMqtt->setHorizontalHeaderItem(7, new QTableWidgetItem(tr("Read interval")));
    mui->tableWidgetMqtt->setHorizontalHeaderItem(8, new QTableWidgetItem(tr("Max Value")));
    connect(mui->tableWidgetMqtt, SIGNAL(cellPressed(int, int)),SLOT(displayMqttDevice(int, int)));
    mui->tableWidgetMqtt->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mui->tableWidgetMqtt, SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(ProvideContexMenu(const QPoint&)));
    /*connect(mui->tableWidgetMqtt, &QWidget::customContextMenuRequested,this,[this] ()
    {
        QMenu *context= new QMenu(this);
        context->addAction("Add");
        context->addAction("Delete");
        context->exec();*/
        /*QList<QTableWidgetItem *> items=mui->tableWidgetMqtt->selectedItems();
        if(items.count()==1) {
            qDebug()<<items[0]->text();
            qApp->clipboard()->setText(items[0]->text());
        }*/
    //});
    //connect(m_client, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic)
    //connect(sub, &QMqttSubscription::messageReceived, this, &QmlMqttSubscription::handleMessage);

    mqttTree->header()->resizeSection(0, 350);
    connect(mui->AddDevice, SIGNAL(clicked()), this, SLOT(AddDeviceClick()));
    connect(mui->pushButtonAddHttpParameter, SIGNAL(clicked()), this, SLOT(AddHttpParameterClick()));
    connect(mui->pushButtonAddMqttParameter, SIGNAL(clicked()), this, SLOT(AddMqttParameterClick()));
    connect(&readTimer, SIGNAL(timeout()),SLOT(startMqtt()));
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
    mui->labelInterfaceName->setToolTip(configFileName);
}


QString ShellyPlugin::getDeviceCommands(const QString &)
{
    return "on=" + tr("On") + "|off=" + tr("Off");
}



void ShellyPlugin::on_buttonConnect_clicked()
{
    if (m_client->state() == QMqttClient::Disconnected) {
        m_client->setUsername(mui->lineEditUser->text());
        m_client->setPassword(mui->lineEditPassword->text());
        m_client->connectToHost();
    } else {
        m_client->disconnectFromHost();
    }
}


void ShellyPlugin::setClientPort(int p)
{
    m_client->setPort(static_cast<quint16>(p));
}


void ShellyPlugin::brokerConnected()
{
    mui->lineEditHost->setEnabled(false);
    mui->spinBoxPort->setEnabled(false);
    mui->lineEditUser->setEnabled(false);
    mui->lineEditPassword->setEnabled(false);
    mui->buttonSubscribe->setEnabled(true);
    mui->buttonPublish->setEnabled(true);
    mui->buttonConnect->setText(tr("Disconnect"));
    on_buttonSubscribe_clicked();
    autoConnectionDone = true;
}



void ShellyPlugin::brokerDisconnected()
{
    mui->lineEditHost->setEnabled(true);
    mui->spinBoxPort->setEnabled(true);
    mui->buttonConnect->setText(tr("Connect"));
    mui->buttonSubscribe->setEnabled(false);
    mui->buttonPublish->setEnabled(false);
    mui->lineEditUser->setEnabled(true);
    mui->lineEditPassword->setEnabled(true);
    mui->lineEditSubTopic->setEnabled(true);
}



void ShellyPlugin::updateLogStateChange()
{
    const QString content = QDateTime::currentDateTime().toString() + ": State Change" + QString::number(m_client->state()) + u'\n';
    mqttLogThis(content);
}


void ShellyPlugin::on_buttonPublish_clicked()
{
    if (mui->checkBoxMqttLog->isChecked()) mqttLogCommand("Publish topic : " + mui->lineEditCommandTopic->text() + " Payload : " + mui->lineEditPayLoad->text());
    if (m_client->publish(mui->lineEditCommandTopic->text(), mui->lineEditPayLoad->text().toUtf8()) == -1)
    {
        if (mui->checkBoxMqttLog->isChecked()) mqttLogCommand(" Failed\n");
        QMessageBox::critical(this, "Error", "Could not publish message");
    }
    else
    {
        if (mui->checkBoxMqttLog->isChecked()) mqttLogCommand(" Succesfull\n");
    }
}


void ShellyPlugin::on_buttonSubscribe_clicked()
{
    if (mui->lineEditSubTopic->isEnabled())
    {
        QMqttTopicFilter topic;
        topic.setFilter(mui->lineEditSubTopic->text());
        auto subscription = m_client->subscribe(topic);
        if (!subscription) {
            QMessageBox::critical(this, "Error", "Could not subscribe. Is there a valid connection?");
            return;
        }
        mui->lineEditSubTopic->setEnabled(false);
        mui->buttonSubscribe->setText("Unsubscribe");
        connect(subscription, &QMqttSubscription::messageReceived, this, &ShellyPlugin::handleMessage);
    }
    else
    {
        m_client->unsubscribe(mui->lineEditSubTopic->text());
        mui->lineEditSubTopic->setEnabled(true);
        mui->buttonSubscribe->setText("Subscribe");
    }
}



void ShellyPlugin::handleMessage(const QMqttMessage &qmsg)
{
    jsonmodel->appendJson(qmsg.topic().name(), qmsg.payload());
    foreach (shellyMqttDevice *dev, deviceSetTable) {
        if (dev->pathLocation.contains(qmsg.topic().name()))
        {
            QJsonTreeItem *item = getMqttTreeItem(dev->pathLocation);
            if (item) {
                item->tableValue = &dev->valueMqtt;
                item->tableValue->RomID = dev->RomID;
                dev->value = item->value().toString();
                dev->valueMqtt.tableWidgetItem.setText(item->value().toString());
                dev->valueMqtt.lastRead_item.setText(QDateTime::currentDateTime().toString("HH:mm:ss"));
                deviceSetTable.removeAll(dev);
                connect(item->tableValue, SIGNAL(heho(QString, QString)), this, SLOT(newValue(QString, QString)));
                bool ok;
                QString value = dev->value;
                value.toDouble(&ok);
                if (!ok) value = translateValue(value);
                emit(newDeviceValue(dev->RomID, value));
            }
        }
    }
}



void ShellyPlugin::newValue(QString RomID, QString value)
{
    bool ok;
    value.toDouble(&ok);
    if (!ok) value = translateValue(value);
    emit(newDeviceValue(RomID, value));
}


void ShellyPlugin::saveConfig()
{
    QFile file(configFileName);
    QSettings settings(configFileName, QSettings::IniFormat);
    settings.clear();
    settings.setValue("Interface_Name", mui->editName->text());
    SimpleCrypt crypto(Q_UINT64_C(0x15cad52ccb57d84c)); //some random number
    QString passwordEncrypted = crypto.encryptToString(mui->lineEditPassword->text());
    settings.setValue("Mqtt_Host", mui->lineEditHost->text());
    QString port = QString("%1").arg(mui->spinBoxPort->value());
    settings.setValue("Mqtt_Port", port);
    settings.setValue("Mqtt_User", mui->lineEditUser->text());
    settings.setValue("Mqtt_PassWord", passwordEncrypted);

    int devCount = 0;
    foreach (shellyMainDevice *Shellydev, shellyMainDevices) {
        int devIndex = 0;
        QString data;
        QString devIDstr = QString("/Device_%1").arg(devCount++, 3, 10, QChar('0'));
        data.append(Shellydev->address->text() + "/");    // IP Address
        data.append(Shellydev->name->text() + "/");    // Name
        data.append(Shellydev->readInterval->currentText());    // Read Interval
        settings.setValue(devIDstr, data );
        foreach (shellyHttpDevice *dev, Shellydev->httpShellyDevices) {
            QString devID = QString("/Device_%1/Parameter_%2").arg(devCount, 3, 10, QChar('0')).arg(++devIndex, 3, 10, QChar('0'));
            QString s = dev->pathLocation;
            settings.setValue(devID + "/pathLocation",s.replace("/", "*"));
            settings.setValue(devID + "/maxValue", dev->maxValue_item.text());
            settings.setValue(devID + "/Command", dev->command_item.text());
            settings.setValue(devID + "/ReadBack", dev->readback_item.text());
        }
    }

    devCount = 0;
    foreach (shellyMainDevice *Shellydev, shellyMainDevices) {
        int devIndex = 0;
        QString data;
        QString devIDstr = QString("/httpDevice_%1").arg(devCount++, 3, 10, QChar('0'));
        data.append(Shellydev->address->text() + "/");    // IP Address
        data.append(Shellydev->name->text() + "/");    // Name
        data.append(Shellydev->readInterval->currentText());    // Read Interval
        settings.setValue(devIDstr, data );
        foreach (shellyHttpDevice *dev, Shellydev->httpShellyDevices) {
            QString devID = QString("/httpDevice_%1/Parameter_%2").arg(devCount, 3, 10, QChar('0')).arg(++devIndex, 3, 10, QChar('0'));
            QString s = dev->pathLocation;
            settings.setValue(devID + "/pathLocation",s.replace("/", "*"));
            settings.setValue(devID + "/maxValue", dev->maxValue_item.text());
            settings.setValue(devID + "/Command", dev->command_item.text());
            settings.setValue(devID + "/ReadBack", dev->readback_item.text());
        }
    }

    int devIndex = 0;
    foreach (shellyMqttDevice *dev, shellyDevicesMqtt) {
        QString devID = QString("/mqttDevice_%1").arg(++devIndex, 3, 10, QChar('0'));
        QString s = dev->pathLocation;
        settings.setValue(devID + "/pathLocation",s.replace("/", "*"));
        settings.setValue(devID + "/maxValue", dev->maxValue_item.text());
        settings.setValue(devID + "/Command", dev->command_item.text());
        settings.setValue(devID + "/Payload", dev->payload_item.text());
        settings.setValue(devID + "/ReadInterval", dev->readInterval.currentText());
    }
}



void ShellyPlugin::readConfig()
{
    loadConfig = true;
    QSettings settings(configFileName, QSettings::IniFormat);
    QString Name = settings.value("Interface_Name").toString();
    if (!Name.isEmpty()) mui->editName->setText(Name);
    QString Mqtt_Host = settings.value("Mqtt_Host").toString();
    if (!Mqtt_Host.isEmpty()) mui->lineEditHost->setText(Mqtt_Host);
    QString Mqtt_Port = settings.value("Mqtt_Port").toString();
    bool ok;
    int port = Mqtt_Port.toInt(&ok);
    if (ok) mui->spinBoxPort->setValue(port);
    QString Mqtt_User = settings.value("Mqtt_User").toString();
    mui->lineEditUser->setText(Mqtt_User);
    QString Mqtt_Pasword_Encrypted = settings.value("Mqtt_PassWord").toString();
    QString Mqtt_Pasword;
    SimpleCrypt crypto(Q_UINT64_C(0x15cad52ccb57d84c)); //some random number
    if (!Mqtt_Pasword_Encrypted.isEmpty())
    {
        Mqtt_Pasword = crypto.decryptToString(Mqtt_Pasword_Encrypted);
        mui->lineEditPassword->setText(Mqtt_Pasword);
    }
    int devID = 0;
    QString devIDstr = QString("/httpDevice_%1").arg(devID++, 3, 10, QChar('0'));
    QString devSettings = settings.value(QString(devIDstr)).toString();
    while (!devSettings.isEmpty()) {
        devSettings = settings.value(QString(devIDstr)).toString();
        QStringList parameters = devSettings.split("/");
        if (parameters.count() > 1) {
            shellyMainDevice *dev = addDevice();
            if (dev) {
                dev->address->setText(parameters.at(0));
                if (parameters.count() == 2) {
                    dev->readInterval->setCurrentText(parameters.at(1));
                }
                if (parameters.count() == 3) {
                    dev->name->setText(parameters.at(1));
                    dev->readInterval->setCurrentText(parameters.at(2)); }
                dev->readDevice();
            }
            int devIndex = 0;
        // get parameters
            QString devParameter = QString("/httpDevice_%1/Parameter_%2").arg(devID, 3, 10, QChar('0')).arg(++devIndex, 3, 10, QChar('0'));
            QString devParamterPathLocation = settings.value(QString(devParameter + "/pathLocation")).toString().replace("*", "/");
            while (!devParamterPathLocation.isEmpty()) {
                shellyHttpDevice *shellyDev = dev->addHttpShellyDevice(devParamterPathLocation);
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
        devIDstr = QString("/httpDevice_%1").arg(devID++, 3, 10, QChar('0'));
        devSettings = settings.value(QString(devIDstr)).toString();
    }

    if (shellyMainDevices.isEmpty())
    {
        int devID = 0;
        QString devIDstr = QString("/Device_%1").arg(devID++, 3, 10, QChar('0'));
        QString devSettings = settings.value(QString(devIDstr)).toString();
        while (!devSettings.isEmpty()) {
            devSettings = settings.value(QString(devIDstr)).toString();
            QStringList parameters = devSettings.split("/");
            if (parameters.count() > 1) {
                shellyMainDevice *dev = addDevice();
                if (dev) {
                    dev->address->setText(parameters.at(0));
                    if (parameters.count() == 2) {
                        dev->readInterval->setCurrentText(parameters.at(1));
                    }
                    if (parameters.count() == 3) {
                        dev->name->setText(parameters.at(1));
                        dev->readInterval->setCurrentText(parameters.at(2)); }
                    dev->readDevice();
                }
                int devIndex = 0;
                // get parameters
                QString devParameter = QString("/Device_%1/Parameter_%2").arg(devID, 3, 10, QChar('0')).arg(++devIndex, 3, 10, QChar('0'));
                QString devParamterPathLocation = settings.value(QString(devParameter + "/pathLocation")).toString().replace("*", "/");
                while (!devParamterPathLocation.isEmpty()) {
                    shellyHttpDevice *shellyDev = dev->addHttpShellyDevice(devParamterPathLocation);
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
    }

    int devIndex = 1;
    QString devParameter = QString("/mqttDevice_%1").arg(devIndex, 3, 10, QChar('0'));
    QString devParamterPathLocation = settings.value(QString(devParameter + "/pathLocation")).toString().replace("*", "/");
    while (!devParamterPathLocation.isEmpty()) {
        shellyMqttDevice *shellyDev = addMqttShellyDevice(devParamterPathLocation);
        QString devParamterMaxValue = settings.value(QString(devParameter + "/maxValue")).toString();
        shellyDev->maxValue_item.setText(devParamterMaxValue);
        QString devCommand = settings.value(QString(devParameter + "/Command")).toString();
        shellyDev->command_item.setText(devCommand);
        QString devPayload = settings.value(QString(devParameter + "/Payload")).toString();
        shellyDev->payload_item.setText(devPayload);
        QString devReadInterval = settings.value(QString(devParameter + "/ReadInterval")).toString();
        shellyDev->readInterval.setCurrentText(devReadInterval);
        devParameter = QString("/mqttDevice_%1").arg(++devIndex, 3, 10, QChar('0'));
        devParamterPathLocation = settings.value(QString(devParameter + "/pathLocation")).toString().replace("*", "/");
        QString RomID = shellyDev->RomID;
        emit(newDevice(this, RomID));
    }
    // set interval enable/disable
    foreach (shellyMqttDevice *dev, shellyDevicesMqtt) {
        if (dev->readInterval.currentIndex() > 0) {
            foreach (shellyMqttDevice *device, shellyDevicesMqtt) {
                if ((dev->devID == device->devID) && (dev != device)) {
                    device->readInterval.setCurrentIndex(0);
                    device->readInterval.setEnabled(false);
            }
        }
    }
}
    loadConfig = false;
    emit(updateInterfaceName(this, Name));
}



void ShellyPlugin::clearHttpLog()
{
    mui->logTxtHttp->clear();
}


void ShellyPlugin::clearMqttLog()
{
    mui->logTxtMqtt->clear();
}



void ShellyPlugin::httpLogChanged()
{
    foreach (shellyMainDevice *dev, shellyMainDevices) {
        disconnect(dev, SIGNAL(logMe(const QString &)), this, SLOT(httpLogThis(const QString &)));
        disconnect(dev, SIGNAL(logCommand(const QString &)), this, SLOT(httpLogCommand(const QString &)));
    }
    if (mui->checkBoxHttpLog->isChecked()) {
        mui->logTxtHttp->show();
        mui->checkBoxWrite->setEnabled(true);
        foreach (shellyMainDevice *dev, shellyMainDevices) connect(dev, SIGNAL(logCommand(const QString &)), this, SLOT(httpLogThis(const QString &)));
        if (!mui->checkBoxWrite->isChecked()) {
            foreach (shellyMainDevice *dev, shellyMainDevices) connect(dev, SIGNAL(logMe(const QString &)), this, SLOT(httpLogCommand(const QString &)));
        }
    }
    else {
        mui->logTxtHttp->hide();
        mui->checkBoxWrite->setEnabled(false);
    }
}



void ShellyPlugin::mqttLogChanged()
{
    if (mui->checkBoxMqttLog->isChecked()) {
        mui->logTxtMqtt->show();
        mui->checkBoxWriteMqtt->setEnabled(true);
    }
    else {
        mui->logTxtMqtt->hide();
        mui->checkBoxWriteMqtt->setEnabled(false);
    }
}


void ShellyPlugin::deviceTableSelection()
{
    QTableWidgetItem *item = mui->httpDeviceTable->currentItem();
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
    QTableWidgetItem *item = mui->httpDeviceTable->currentItem();
    if (item == nullptr) mui->RemoveButton->setEnabled(false);
    else {
        if (item->isSelected()) mui->RemoveButton->setEnabled(!state);
        else  mui->RemoveButton->setEnabled(false);
    }
    mui->editName->setEnabled(!state);
    for (int n=0; n<mui->httpDeviceTable->rowCount(); n++) {
        QTableWidgetItem *item = mui->httpDeviceTable->item(n, 0);
        if (item) {
        if (state) item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        else item->setFlags(item->flags() | Qt::ItemIsEditable); }
        foreach (shellyMainDevice *dev, shellyMainDevices) {
        dev->readInterval->setEnabled(!state);
        }
    }
    lockedState = state;
    if (state) {
        mui->pushButtonLock->setIcon(QIcon(QPixmap(QString::fromUtf8(":/images/lock.png"))));
        mui->pushButtonLockMqtt->setIcon(QIcon(QPixmap(QString::fromUtf8(":/images/lock.png"))));
        mui->pushButtonAddHttpParameter->setEnabled(false);
        mui->gridLayout_5->removeWidget(mqttTree);
        mqttTree->hide();
        mui->gridLayout_5->addWidget(mui->tableWidgetMqtt, 0, 0, 1, 1);
        mui->tableWidgetMqtt->show();
    }
    else
    {
        mui->pushButtonLock->setIcon(QIcon(QPixmap(QString::fromUtf8(":/images/unlock.png"))));
        mui->pushButtonLockMqtt->setIcon(QIcon(QPixmap(QString::fromUtf8(":/images/unlock.png"))));
        mui->gridLayout_5->removeWidget(mui->tableWidgetMqtt);
        mui->tableWidgetMqtt->hide();
        mui->gridLayout_5->addWidget(mqttTree, 0, 0, 1, 1);
        mqttTree->show();
        httpPathSelected();
    }
    QModelIndex index = mui->httpDeviceTable->currentIndex();
    if (index.isValid()) displayHttpDevice(index.row(), 0);
}


QString ShellyPlugin::getDeviceConfig(QString)
{
    return "";
}


void ShellyPlugin::setDeviceConfig(const QString &config, const QString &data)
{
    // if (plugin_interface) plugin_interface->setDeviceConfig("setDeviceName",  romid + "//" + name);
    if (config == "setDeviceName") {
        QStringList d = data.split("//");
        if (d.count() == 2) {
            QString  RomID = d.first();
            QString  Name = d.last();
            foreach (shellyMqttDevice *dev, shellyDevicesMqtt)
                if (dev->RomID == RomID) dev->name_item.setText(Name);
        }
    }
}


QString ShellyPlugin::getName()
{
    return mui->editName->text();
}



double ShellyPlugin::getMaxValue(const QString RomID)
{
    foreach (shellyMainDevice *devMain, shellyMainDevices)
    {
        foreach (shellyHttpDevice *dev, devMain->httpShellyDevices) {
        if (dev->RomID == RomID) {
            bool ok;
            int v = dev->maxValue_item.text().toInt(&ok);
            if (ok) return v;
        } } }
    foreach (shellyMqttDevice *dev, shellyDevicesMqtt) {
        if (dev->RomID == RomID) {
            bool ok;
            int v = dev->maxValue_item.text().toInt(&ok);
            if (ok) return v;
        } }
        return 1;
}


bool ShellyPlugin::isManual(const QString)
{
    return false;
}


bool ShellyPlugin::isDimmable(const QString RomID)
{
    foreach (shellyMainDevice *devMain, shellyMainDevices)
    {
        foreach (shellyHttpDevice *dev, devMain->httpShellyDevices) {
            if (dev->RomID == RomID) {
            bool ok;
            int v = dev->maxValue_item.text().toInt(&ok);
            if (ok && (v > 1)) return true;
            } } }
    foreach (shellyMqttDevice *dev, shellyDevicesMqtt) {
        if (dev->RomID == RomID) {
            bool ok;
            int v = dev->maxValue_item.text().toInt(&ok);
            if (ok && (v > 1)) return true;
        } }
    return false;
}


bool ShellyPlugin::acceptCommand(const QString)
{
    return true;
/*    foreach (shellyMainDevice *devMain, shellyMainDevices)
    {
        foreach (shellyHttpDevice *dev, devMain->httpShellyDevices) {
        if (dev->RomID == RomID) {
                if (dev->command_item.text().isEmpty()) return false; else return true; }
        }
        foreach (shellyMqttDevice *dev, shellyDevicesMqtt) {
        if (dev->RomID == RomID) {
                if (dev->command_item.text().isEmpty()) return false; else return true; }
        }
    }
    return false;*/
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
        foreach (shellyHttpDevice *dev, devMain->httpShellyDevices) {
        if (dev->RomID == RomID) {
                devMain->setCommand(dev, command);
                return; }
        }
    }
    foreach (shellyMqttDevice *dev, shellyDevicesMqtt) {
    if (dev->RomID == RomID) {
        QString payload = dev->payload_item.text();
        if (payload.contains("XXX")) payload.replace("XXX", command); else payload.append(command);
        if (mui->checkBoxMqttLog->isChecked()) mqttLogCommand("Publish topic : " + dev->command_item.text() + " Payload : " + payload);
        QByteArray P = (payload).toUtf8();
        if (m_client->publish(dev->command_item.text(), P) == -1)
        {
            if (mui->checkBoxMqttLog->isChecked()) mqttLogCommand(" Failed\n");
        }
        else
        {
            if (mui->checkBoxMqttLog->isChecked()) mqttLogCommand(" Succesfull\n");
        }
        return; }
    }
}



void ShellyPlugin::log(const QString str)
{
    if (mui->checkBoxHttpLog->isChecked())
    {
        if (logStr.length() > 100000) logStr = logStr.mid(90000);
        mui->logTxtHttp->append(str);
        mui->logTxtHttp->moveCursor(QTextCursor::End);
    }
}



void ShellyPlugin::AddDeviceClick()
{
    addDevice();
}


QString ShellyPlugin::selectedHttpPath()
{
    int index = mui->httpDeviceTable->currentRow();
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
        foreach (shellyHttpDevice *dev, shellyMainDevices.at(index)->httpShellyDevices)
        {
                if (dev->pathLocation == path) found = true;
        }
        if (!found) return path;
        } }
        return "";
}



QString ShellyPlugin::selectedMqttPath()
{
        QModelIndex md = mqttTree->currentIndex();
        if (md.column() != 0) md = md.sibling(md.row(), 0);
        QString path;
        if (md.isValid()) {
        path = md.data(Qt::DisplayRole).toString();
        while(md.parent().isValid()) {
        path = md.parent().data().toString() + "/" + path;
        md = md.parent(); }
        }
        // check if device exists :
        bool found = false;
        foreach (shellyMqttDevice *dev, shellyDevicesMqtt) {
                if (dev->pathLocation == path) found = true; }
        if (!found) return path;
        return "";
}



void ShellyPlugin::httpPathSelected()
{
    QString path = selectedHttpPath();
    if (path.isEmpty())
    {
        mui->pushButtonAddHttpParameter->setEnabled(false);
        mui->pushButtonAddHttpParameter->setToolTip("");
    }
    else
    {
        mui->pushButtonAddHttpParameter->setEnabled(true);
        mui->pushButtonAddHttpParameter->setToolTip(path);
    }
}



void ShellyPlugin::mqttPathSelected()
{
    QString path = selectedMqttPath();
    if (path.isEmpty())
    {
        mui->pushButtonAddMqttParameter->setEnabled(false);
        mui->pushButtonAddMqttParameter->setToolTip("");
    }
    else
    {
        mui->pushButtonAddMqttParameter->setEnabled(true);
        mui->pushButtonAddMqttParameter->setToolTip(path);
    }
}


void ShellyPlugin::AddHttpParameterClick()
{
        QString path = selectedHttpPath();
        int index = mui->httpDeviceTable->currentRow();
        if (index != -1)
        {
            shellyMainDevices.at(index)->addHttpShellyDevice(path);
            mui->pushButtonAddHttpParameter->setEnabled(false);
            mui->pushButtonAddHttpParameter->setToolTip("");
        }
}


void ShellyPlugin::AddMqttParameterClick()
{
        QString path = selectedMqttPath();
        QStringList pList = path.split("/");
        QString Name = pList.first() + "_" + pList.last();
        addMqttShellyDevice(path, Name);
        mui->pushButtonAddMqttParameter->setEnabled(false);
        mui->pushButtonAddMqttParameter->setToolTip("");
}




shellyMqttDevice *ShellyPlugin::addMqttShellyDevice(const QString &parameterPath, QString Name)
{
        shellyMqttDevice *newDev = new shellyMqttDevice;
        if (newDev) {
            int index = shellyDevicesMqtt.count();
            shellyDevicesMqtt.append(newDev);
            connect(&newDev->readInterval, &QComboBox::currentTextChanged, [=] { intervalChanged( &newDev->readInterval );  } );
            newDev->RomID = buildRomID(shellyDevicesMqtt.count());
            newDev->pathLocation = parameterPath;
            QJsonTreeItem *item = getMqttTreeItem(parameterPath);
            if (item) {
                        item->tableValue = &newDev->valueMqtt;
                        item->tableValue->RomID = newDev->RomID;
                        newDev->value = item->value().toString();
                        newDev->valueMqtt.tableWidgetItem.setText(item->value().toString());
                        connect(item->tableValue, SIGNAL(heho(QString, QString)), this, SLOT(newValue(QString, QString)));
                        newDev->valueMqtt.lastRead_item.setText(QDateTime::currentDateTime().toString("HH:mm:ss"));
                        bool ok;
                        QString value = newDev->value;
                        value.toDouble(&ok);
                        if (!ok) value = translateValue(value);
                        emit(newDeviceValue(newDev->RomID, value));
            }
            else deviceSetTable.append(newDev);
            mui->tableWidgetMqtt->insertRow(index);
            mui->tableWidgetMqtt->setItem(index, 0, &newDev->RomID_item);
            newDev->RomID_item.setText(newDev->RomID);
            newDev->name_item.setFlags(newDev->name_item.flags() ^ Qt::ItemIsEditable);
            mui->tableWidgetMqtt->setItem(index, 1, &newDev->name_item);
            newDev->RomID_item.setFlags(newDev->RomID_item.flags() ^ Qt::ItemIsEditable);
            mui->tableWidgetMqtt->setItem(index, 2, &newDev->path_item);
            newDev->path_item.setText(parameterPath);
            newDev->path_item.setFlags(newDev->path_item.flags() ^ Qt::ItemIsEditable);
            mui->tableWidgetMqtt->setItem(index, 3, &newDev->valueMqtt.tableWidgetItem);
            newDev->valueMqtt.tableWidgetItem.setText(newDev->value);
            newDev->valueMqtt.tableWidgetItem.setFlags(newDev->valueMqtt.tableWidgetItem.flags() ^ Qt::ItemIsEditable);
            mui->tableWidgetMqtt->setItem(index, 4, &newDev->valueMqtt.lastRead_item);
            mui->tableWidgetMqtt->setItem(index, 5, &newDev->command_item);
            mui->tableWidgetMqtt->setItem(index, 6, &newDev->payload_item);
            mui->tableWidgetMqtt->setCellWidget(index, 7, &newDev->readInterval);
            mui->tableWidgetMqtt->setItem(index, 8, &newDev->maxValue_item);
            QStringList L = newDev->path_item.text().split("/");
            if (L.count() > 1) { if (L.at(0) == "shellies") {
                newDev->devID = L.at(1);
                newDev->devCommandStr = L.at(0) + "/" + L.at(1) + "/command"; }
                else newDev->devID = L.at(0); }
            bool ok;
            QString value = newDev->value;
            value.toDouble(&ok);
            if (!ok) value = translateValue(value);
            QString RomID = newDev->RomID;
            if (!loadConfig) RomID.append("!");
            if (!Name.isEmpty()) RomID.append( ":" + Name);
            if (!loadConfig) {
                        emit(newDevice(this, RomID));
                        emit(newDeviceValue(newDev->RomID, value));
                        setLockedState();
            }
            return newDev;
        }
        return nullptr;
}


QString ShellyPlugin::translateValue(QString value)
{
    if (value.contains("true", Qt::CaseInsensitive)) return "1";
    if (value.contains("on", Qt::CaseInsensitive)) return "1";
    if (value.contains("false", Qt::CaseInsensitive)) return "0";
    if (value.contains("off", Qt::CaseInsensitive)) return "0";
    return "NA";
}




QJsonTreeItem *ShellyPlugin::getMqttTreeItem(QString path)
{
    QStringList listParameters = path.split("/");
    int parametersCount = listParameters.count();
    if (!listParameters.isEmpty()) {

    if (parametersCount == 1) {
        for (int n = 0; n<jsonmodel->rowCount(); n++) {
                QModelIndex m = jsonmodel->index(n, 0);
                if (m.data().toString() == path) {
                    QJsonTreeItem *item = static_cast<QJsonTreeItem*>(m.internalPointer());
                    return item; } } }

        if (parametersCount > 1) {
            for (int n = 0; n<jsonmodel->rowCount(); n++) {
                    QModelIndex mRoot = jsonmodel->index(n, 0);
                    if (mRoot.data().toString() == listParameters.first()) {
                    int i = 0;
                    int pIndex = 1;
                    QModelIndex m = jsonmodel->index(i, 0, mRoot);
                oneMoreStep:
                    while (m.isValid()) {
                        if (m.data().toString() == listParameters.at(pIndex)) {
                                if (++pIndex == listParameters.count()) {
                                    QJsonTreeItem *item = static_cast<QJsonTreeItem*>(jsonmodel->index(i, 1, mRoot).internalPointer());
                                    return item;
                            }
                            mRoot = m;
                            i = 0;
                            m = jsonmodel->index(i, 0, mRoot);
                            goto oneMoreStep;
                        }
                        m = jsonmodel->index(++i, 0, mRoot);
                    }
                }
            }
        }
        return nullptr;
    }
    return nullptr;
}



QModelIndex ShellyPlugin::getValue(QString path) {
    QModelIndex z;
    QStringList listParameters = path.split("/");
    int parametersCount = listParameters.count();
    if (!listParameters.isEmpty()) {
        if (parametersCount == 1) {
        for (int n = 0; n<jsonmodel->rowCount(); n++) {
                QModelIndex m = jsonmodel->index(n, 0);
                QModelIndex d = jsonmodel->index(n, 1);
                if (m.data().toString() == path) {
                    return d; } } }


        if (parametersCount > 1) {
        for (int n = 0; n<jsonmodel->rowCount(); n++) {
                QModelIndex mRoot = jsonmodel->index(n, 0);
                if (mRoot.data().toString() == listParameters.first()) {
                    int i = 0;
                    int pIndex = 1;
                    QModelIndex m = jsonmodel->index(i, 0, mRoot);
                oneMoreStep:
                    while (m.isValid()) {
                        if (m.data().toString() == listParameters.at(pIndex)) {
                            if (++pIndex == listParameters.count()) {
                                    jsonmodel->index(i, 1, mRoot);
                            }
                            mRoot = m;
                            i = 0;
                            m = jsonmodel->index(i, 0, mRoot);
                            goto oneMoreStep;
                        }
                        m = jsonmodel->index(++i, 0, mRoot);
                    }
                }
        }
        }

    }

   /*     if (parametersCount == 2) {
        for (int n = 0; n<jsonmodel->rowCount(); n++) {
                QModelIndex mdA = jsonmodel->index(n, 0);
                if (mdA.data().toString() == listParameters.at(0)) {
                    int i = 0;
                    QModelIndex mdB = jsonmodel->index(i, 0, mdA);
                    if (!mdB.isValid()) qDebug() << "mdB not valid";
                    while (mdB.isValid()) {
                        if (mdB.data().toString() == listParameters.at(1)) {
                            return jsonmodel->index(i, 1, mdA); }
                        mdB = jsonmodel->index(++i, 0, mdA); }
                    return z; } } }
        if (parametersCount == 3) {
        for (int n = 0; n<jsonmodel->rowCount(); n++) {
                QModelIndex mdA = jsonmodel->index(n, 0);
                if (mdA.data().toString() == listParameters.at(0)) {
                    int i = 0;
                    QModelIndex mdB = jsonmodel->index(i, 0, mdA);
                    while (mdB.isValid()) {
                        if (mdB.data().toString() == listParameters.at(1)) {
                            i = 0;
                            QModelIndex mdC = jsonmodel->index(i, 0, mdB);
                            while (mdC.isValid()) {
                                if (mdC.data().toString() == listParameters.at(2)) {
                                    return jsonmodel->index(i, 1, mdB); }
                                mdC = jsonmodel->index(++i, 0, mdB); }
                            return z; }
                        mdB = jsonmodel->index(++i, 0, mdA); }
                    return z; } } }
        if (parametersCount == 4) {
        for (int n = 0; n<jsonmodel->rowCount(); n++) {
                QModelIndex mdA = jsonmodel->index(n, 0);
                if (mdA.data().toString() == listParameters.at(0)) {
                    int i = 0;
                    QModelIndex mdB = jsonmodel->index(i, 0, mdA);
                    while (mdB.isValid()) {
                        if (mdB.data().toString() == listParameters.at(1)) {
                            i = 0;
                            QModelIndex mdC = jsonmodel->index(i, 0, mdB);
                            while (mdC.isValid()) {
                                if (mdC.data().toString() == listParameters.at(2)) {
                                    i = 0;
                                    QModelIndex mdD = jsonmodel->index(i, 0, mdC);
                                    while (mdD.isValid()) {
                                        if (mdD.data().toString() == listParameters.at(3)) {
                                            return jsonmodel->index(i, 1, mdC); }
                                        mdD = jsonmodel->index(++i, 0, mdC); }
                                    return z; }
                                mdC = jsonmodel->index(++i, 0, mdB); }
                            return z; }
                        mdB = jsonmodel->index(++i, 0, mdA); }
                    return z; } } }
        if (parametersCount == 5) {
        for (int n = 0; n<jsonmodel->rowCount(); n++) {
                QModelIndex mdA = jsonmodel->index(n, 0);
                if (mdA.data().toString() == listParameters.at(0)) {
                    int i = 0;
                    QModelIndex mdB = jsonmodel->index(i, 0, mdA);
                    while (mdB.isValid()) {
                        if (mdB.data().toString() == listParameters.at(1)) {
                            i = 0;
                            QModelIndex mdC = jsonmodel->index(i, 0, mdB);
                            while (mdC.isValid()) {
                                if (mdC.data().toString() == listParameters.at(2)) {
                                    i = 0;
                                    QModelIndex mdD = jsonmodel->index(i, 0, mdC);
                                    while (mdD.isValid()) {
                                        if (mdD.data().toString() == listParameters.at(3)) {
                                            i = 0;
                                            QModelIndex mdE = jsonmodel->index(i, 0, mdD);
                                            while (mdE.isValid()) {
                                                if (mdE.data().toString() == listParameters.at(4)) {
                                                    return jsonmodel->index(i, 1, mdD); }
                                                mdE = jsonmodel->index(++i, 0, mdD); }
                                            return z; }
                                        mdD = jsonmodel->index(++i, 0, mdC); }
                                    return z; }
                                mdC = jsonmodel->index(++i, 0, mdB); }
                            return z; }
                        mdB = jsonmodel->index(++i, 0, mdA); }
                    return z; } } }
    }*/
    return z;
}



QString ShellyPlugin::buildRomID(int n)
{
        QString ipHex = shellyMainDevice::ip2Hex(mui->lineEditHost->text());
        QString id = QString("%1").arg(n, 3, 10, QLatin1Char('0')).toUpper();
        QString RomID = ipHex + id + "SD";
        return RomID;
}


shellyMainDevice *ShellyPlugin::addDevice()
{
    // create new device
    shellyMainDevice *newDev = new shellyMainDevice();
    if (newDev) {
        int count = shellyMainDevices.count();
        shellyMainDevices.append(newDev);
        mui->httpDeviceTable->insertRow(count);
        mui->httpDeviceTable->setItem(count, 0, newDev->address);
        mui->httpDeviceTable->setItem(count, 1, newDev->name);
        mui->httpDeviceTable->setCellWidget(count, 2, newDev->readInterval);
        mui->httpDeviceTable->setItem(count, 3, newDev->model);
        mui->httpDeviceTable->setItem(count, 4, newDev->lastRead);
        connect(newDev, SIGNAL(newDevice(const shellyHttpDevice*)), this, SLOT(newHttpShellyDevice(const shellyHttpDevice*)));
        connect(newDev, SIGNAL(newDeviceValue(const shellyHttpDevice*)), this, SLOT(newHttpShellyDeviceValue(const shellyHttpDevice*)));
        connect(newDev, SIGNAL(showDevice(const shellyHttpDevice*)), this, SLOT(showHttpDevice(const shellyHttpDevice*)));
        connect(&newDev->treeView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(httpPathSelected()));
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
    int index = mui->httpDeviceTable->currentRow();
    if (index != -1) {
        QList<shellyMainDevice*> shellyDev;
        shellyDev.append(shellyMainDevices.at(index));
        readHttpDevices(shellyDev);
    }
}


void ShellyPlugin::on_ReadAllButton_clicked()
{
    readHttpDevices(shellyMainDevices);
}


void ShellyPlugin::on_RemoveButton_clicked()
{
    int index = mui->httpDeviceTable->currentRow();
    if (index != -1) {
        mui->httpDeviceTable->removeRow(index);
        shellyMainDevices.removeAt(index);
    }
}





void ShellyPlugin::newHttpShellyDevice(const shellyHttpDevice* dev, QString Name)
{
    bool ok;
    QString value = dev->value;
    value.toDouble(&ok);
    if (!ok) value = translateValue(value);
    QString RomID = dev->RomID;
    if (!loadConfig) RomID.append("!");
    if (!Name.isEmpty()) RomID.append( ":" + Name);
    emit(newDevice(this, RomID));
    if (!loadConfig) {
        emit(newDeviceValue(dev->RomID, value));
        setLockedState(); }
}


void ShellyPlugin::newMqttShellyDevice(const shellyMqttDevice* dev, QString Name)
{
    bool ok;
    QString value = dev->value;
    value.toDouble(&ok);
    if (!ok) value = translateValue(value);
    QString RomID = dev->RomID;
    if (!loadConfig) RomID.append("!");
    if (!Name.isEmpty()) RomID.append( ":" + Name);
    emit(newDevice(this, RomID));
    if (!loadConfig) {
        emit(newDeviceValue(dev->RomID, value));
        setLockedState();
    }
}


void ShellyPlugin::newHttpShellyDevice(const shellyHttpDevice* dev)
{
    bool ok;
    QString value = dev->value;
    value.toDouble(&ok);
    if (!ok) value = translateValue(value);
    QString RomID = dev->RomID;
    if (!loadConfig) RomID.append("!");
    emit(newDevice(this, RomID));
    if (!loadConfig) {
        emit(newDeviceValue(dev->RomID, value));
        setLockedState(); }
}



void ShellyPlugin::newMqttShellyDevice(const shellyMqttDevice* dev)
{
    bool ok;
    QString value = dev->value;
    value.toDouble(&ok);
    if (!ok) value = translateValue(value);
    QString RomID = dev->RomID;
    if (!loadConfig) RomID.append("!");
    emit(newDevice(this, RomID));
    if (!loadConfig) { emit(newDeviceValue(dev->RomID, value));
        setLockedState(); }
}


void ShellyPlugin::newHttpShellyDeviceValue(const shellyHttpDevice* dev)
{
    bool ok;
    QString value = dev->value;
    value.toDouble(&ok);
    if (!ok) value = translateValue(value);
    emit(newDeviceValue(dev->RomID, value));
}




void ShellyPlugin::newMqttShellyDeviceValue(const shellyMqttDevice* dev)
{
    bool ok;
    QString value = dev->value;
    value.toDouble(&ok);
    if (!ok) value = translateValue(value);
    emit(newDeviceValue(dev->RomID, value));
}


void ShellyPlugin::showHttpDevice(const shellyHttpDevice* dev)
{
    emit(deviceSelected(dev->RomID));
}


void ShellyPlugin::showMqttDevice(const shellyMqttDevice* dev)
{
    emit(deviceSelected(dev->RomID));
}


void ShellyPlugin::intervalChanged(const mqttReadInterval *interval)
{
    shellyMqttDevice *device = nullptr;
    if (interval) {
        foreach (shellyMqttDevice *dev, shellyDevicesMqtt) {
        if (&dev->readInterval == interval) { device = dev; break; }
        }
        if (!device) return;
        QStringList paths = device->pathLocation.split("/");
        QString strCommand;
        if (paths.count() > 2) strCommand = paths.at(0) + "/" + paths.at(1) + "/command";
        if (paths.at(0) == "shellies") { device->devCommandStr = strCommand;
        device->command_item.setText(strCommand); }
        if (device->readInterval.currentIndex() == 0) {
        foreach (shellyMqttDevice *dev, shellyDevicesMqtt) {
                if ((dev->devID == device->devID) && (dev != device)) {
                    dev->readInterval.setEnabled(true);
                } }
        }
        else {
        foreach (shellyMqttDevice *dev, shellyDevicesMqtt) {
                if ((dev->devID == device->devID) && (dev != device)) {
                    dev->readInterval.setCurrentIndex(0);
                    dev->readInterval.setEnabled(false);
                } }
        }
    }
}


void ShellyPlugin::httpLogThis(const QString &log)
{
    if (!mui->checkBoxHttpLog->isChecked()) return;
    if (mui->checkBoxHttpLog->isChecked() && mui->checkBoxWrite->isChecked()) return;
    QString txt = mui->logTxtHttp->toPlainText();
    txt.append(QDateTime::currentDateTime().toString("HH:mm:ss ") + log + "\n");
    int l = txt.length();
    if (l > 10000) txt = txt.mid(l-10000, 10000);
    if (mui->checkBoxHttpLog->isChecked()) {
        mui->logTxtHttp->setText(txt);
        mui->logTxtHttp->moveCursor(QTextCursor::End);
    }
}



void ShellyPlugin::httpLogCommand(const QString &log)
{
    if (mui->checkBoxHttpLog->isChecked() && mui->checkBoxWrite->isChecked())
    {
        QString txt = mui->logTxtHttp->toPlainText();
        txt.append(QDateTime::currentDateTime().toString("HH:mm:ss ") + log + "\n");
        int l = txt.length();
        if (l > 10000) txt = txt.mid(l-10000, 10000);
        if (mui->checkBoxHttpLog->isChecked()) {
            mui->logTxtHttp->setText(txt);
            mui->logTxtHttp->moveCursor(QTextCursor::End);
        }
    }
}


void ShellyPlugin::mqttLogThis(const QString &log)
{
    if (!mui->checkBoxMqttLog->isChecked()) return;
    if (mui->checkBoxMqttLog->isChecked() && mui->checkBoxWriteMqtt->isChecked()) return;
    QString txt = mui->logTxtMqtt->toPlainText();
    txt.append(QDateTime::currentDateTime().toString("HH:mm:ss ") + log + "\n");
    int l = txt.length();
    if (l > 10000) txt = txt.mid(l-10000, 10000);
    if (mui->checkBoxMqttLog->isChecked()) {
        mui->logTxtMqtt->setText(txt);
        mui->logTxtMqtt->moveCursor(QTextCursor::End);
    }
}




void ShellyPlugin::mqttLogCommand(const QString &log)
{
    if (mui->checkBoxMqttLog->isChecked() && mui->checkBoxWriteMqtt->isChecked())
    {
        QString txt = mui->logTxtMqtt->toPlainText();
        txt.append(QDateTime::currentDateTime().toString("HH:mm:ss ") + log  + "\n");
        int l = txt.length();
        if (l > 10000) txt = txt.mid(l-10000, 10000);
        if (mui->checkBoxMqttLog->isChecked()) {
            mui->logTxtMqtt->setText(txt);
            mui->logTxtMqtt->moveCursor(QTextCursor::End);
        }
    }
}


void ShellyPlugin::readDevice(const QString &)
{
}


void ShellyPlugin::readHttpDevices(QList<shellyMainDevice*>& devList)
{
    foreach (shellyMainDevice *dev, devList) {
        dev->readDevice();
    }
}



void ShellyPlugin::readMqttDevices(QList<shellyMqttDevice*>& devList)
{
    foreach (shellyMqttDevice *dev, devList) {
        //qDebug() << "Read " + dev->pathLocation;
        QString topic = dev->devCommandStr;
        QString payload = "announce";
        if (mui->checkBoxMqttLog->isChecked()) mqttLogCommand("Publish topic : " + topic + " Payload : " + payload);
        if (m_client->publish(topic, payload.toUtf8()) == -1)
        {
            if (mui->checkBoxMqttLog->isChecked()) mqttLogCommand(" Failed\n");
        }
        else
        {
            if (mui->checkBoxMqttLog->isChecked()) mqttLogCommand(" Succesfull\n");
        }
    }
}



void ShellyPlugin::startMqtt()
{
    disconnect(&readTimer, 0, 0, 0);
    on_buttonConnect_clicked();
    connect(&readTimer, SIGNAL(timeout()),SLOT(readAllNow()));
}




void ShellyPlugin::readAllNow()
{
    QList<shellyMainDevice*> httpDevList;
    QList<shellyMqttDevice*> mqttDevList;
    if (lastMinute < 0) {
        readHttpDevices(shellyMainDevices);
        foreach (shellyMqttDevice *dev, shellyDevicesMqtt) {
            if (dev->readInterval.currentIndex() > 0) mqttDevList.append(dev);
        }
        readMqttDevices(mqttDevList);
        lastMinute = QDateTime::currentDateTime().time().minute();
    }
    int m = QDateTime::currentDateTime().time().minute();
    if (m != lastMinute)
    {
        if (m_client->state() != QMqttClient::Connected) { autoConnectionDone = false; }
        if (!autoConnectionDone) on_buttonConnect_clicked();
        foreach (shellyMainDevice *dev, shellyMainDevices) {
            switch (dev->readInterval->currentIndex())
            {
                case readHttp1mn : httpDevList.append(dev); break;
                case readHttp2mn : if (m % 2 == 0) httpDevList.append(dev); break;
                case readHttp5mn : if (m % 5 == 0) httpDevList.append(dev); break;
                case readHttp10mn : if (m % 10 == 0) httpDevList.append(dev); break;
                case readHttp30mn : if (m % 30 == 0) httpDevList.append(dev); break;
                case readHttp1hour : if (m == 0) httpDevList.append(dev); break;
            }
        }
        foreach (shellyMqttDevice *dev, shellyDevicesMqtt) {
            switch (dev->readInterval.currentIndex())
            {
            case readMqtt1mn : mqttDevList.append(dev); break;
            case readMqtt2mn : if (m % 2 == 0) mqttDevList.append(dev); break;
            case readMqtt5mn : if (m % 5 == 0) mqttDevList.append(dev); break;
            case readMqtt10mn : if (m % 10 == 0) mqttDevList.append(dev); break;
            case readMqtt30mn : if (m % 30 == 0) mqttDevList.append(dev); break;
            case readMqtt1hour : if (m == 0) mqttDevList.append(dev); break;
            }
        }
        lastMinute = m;
    }
    readHttpDevices(httpDevList);
    readMqttDevices(mqttDevList);
}




void ShellyPlugin::displayHttpDevice(int row, int)
{
    foreach (shellyMainDevice *dev, shellyMainDevices) {
        mui->gridLayout_3->removeWidget(&dev->treeView);
        dev->treeView.hide();
        dev->deviceView.hide();
    }
    if (lockedState) {
        mui->gridLayout_3->addWidget(&shellyMainDevices.at(row)->deviceView, 3, 0, 1, 4);
        shellyMainDevices.at(row)->deviceView.show();
    }
    else {
        mui->gridLayout_3->addWidget(&shellyMainDevices.at(row)->treeView, 3, 0, 1, 4);
        shellyMainDevices.at(row)->treeView.show();
    }
}




void ShellyPlugin::displayMqttDevice(int row, int)
{
    emit(showMqttDevice(shellyDevicesMqtt.at(row)));
}



void ShellyPlugin::ProvideContexMenu(const QPoint&p)
{
    QPoint c = mui->tableWidgetMqtt->mapToGlobal(p);
    QMenu submenu;
    QAction copyAction(tr("Copy"));
    QAction lockAction(tr("Lock"));
    QAction unlockAction(tr("Unlock"));
    QAction pasteAction("");
    QAction pasteCommandAction("");
    QString strPaste, strPasteCommand;
    QList<QTableWidgetItem *> items = mui->tableWidgetMqtt->selectedItems();
    if (items.count() == 1) {
        if (items[0]->column() == 1) { submenu.addAction(&copyAction); }
        if (items[0]->column() == 2) {
            submenu.addAction(&copyAction);
            if (items[0]->flags() & Qt::ItemIsEditable) submenu.addAction(&lockAction); else submenu.addAction(&unlockAction); }
        if (items[0]->column() == 5) {
            int r = items[0]->row();
            QString path = mui->tableWidgetMqtt->item(r, 2)->text();
            QStringList paths = path.split("/");
            if (paths.count() > 2) strPaste = paths.at(0) + "/" + paths.at(1);
            if (paths.count() > 2) strPasteCommand = paths.at(0) + "/" + paths.at(1) + "/command";
            if (paths.at(0) == "shellies") { pasteAction.setText(strPaste);
            pasteCommandAction.setText(strPasteCommand);
            submenu.addAction(&pasteAction);
            submenu.addAction(&pasteCommandAction); } }
        QAction* rightClickItem = submenu.exec(c);
        if (rightClickItem == &copyAction) {
            qApp->clipboard()->setText(items[0]->text()); }
        if (rightClickItem == &lockAction) items[0]->setFlags(items[0]->flags() ^ Qt::ItemIsEditable);
        if (rightClickItem == &unlockAction) items[0]->setFlags(items[0]->flags() | Qt::ItemIsEditable);
        if (rightClickItem == &pasteAction) items[0]->setText(strPaste);
        if (rightClickItem == &pasteCommandAction) items[0]->setText(strPasteCommand);
        }
}

