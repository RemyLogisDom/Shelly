#ifndef SHELLYPLUGIN_H
#define SHELLYPLUGIN_H

#include <QObject>
#include <QtPlugin>
#include <QTabWidget>
#include <QTcpSocket>
#include <QThread>
#include <QTreeWidget>
#include <QComboBox>
#include <QTimer>
#include <QNetworkReply>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMutex>
#include <QtMqtt/QMqttClient>
#include "ui_Shelly.h"
#include "qjsonmodel.h"
#include "../interface.h"



enum readHttpInterval { readHttp1mn, readHttp2mn, readHttp5mn, readHttp10mn, readHttp30mn, readHttp1hour };



class comboInterval : public QComboBox
{
    Q_OBJECT
public:
    comboInterval() {
        addItem("1mn");
        addItem("2mn");
        addItem("5mn");
        addItem("10mn");
        addItem("30mn");
        addItem("1 hour");
    }
};


class shellyMainDevice : public QWidget
{
    Q_OBJECT
public:
    QList<shellyHttpDevice*> httpShellyDevices;
    shellyHttpDevice* deviceCommand = nullptr;
    QTableWidgetItem *address;
    QTableWidgetItem *name;
    QTableWidgetItem *model;
    comboInterval *readInterval;
    QTableWidgetItem *lastRead;
    QString httpReq;
    QNetworkAccessManager qnamHttp, qnamGet, qnapCommand;
    QJsonModel *jsonmodel;
    QTreeView treeView;
    QTableWidget deviceView;
    QMutex mutex;
    /// used to request Shelly model when application starts.
    /// Once model has been identified got_model = true
    bool got_model = false;
    QString log, url_str;
signals:
    void newDevice(const shellyHttpDevice*);
    void newDeviceValue(const shellyHttpDevice*);
    void showDevice(const shellyHttpDevice*);
    void logCommand(const QString&);
    void logMe(const QString&);
public slots:
    void displayDevice(int row, int)
    {
        emit(showDevice(httpShellyDevices.at(row)));
    }

public:
    shellyMainDevice() {
        address = new QTableWidgetItem("192.168.1.xxx");
        name = new QTableWidgetItem("");
        model = new QTableWidgetItem("...");
        model->setFlags(model->flags() ^ Qt::ItemIsEditable);
        readInterval = new comboInterval;
        lastRead = new QTableWidgetItem();
        lastRead->setFlags(lastRead->flags() ^ Qt::ItemIsEditable);
        readInterval->setCurrentIndex(0);
        jsonmodel = new QJsonModel;
        treeView.setModel(jsonmodel);
        deviceView.setColumnCount(6);
        deviceView.setHorizontalHeaderItem(0, new QTableWidgetItem(tr("RomID")));
        deviceView.setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Path")));
        deviceView.setHorizontalHeaderItem(2, new QTableWidgetItem(tr("Value")));
        deviceView.setHorizontalHeaderItem(3, new QTableWidgetItem(tr("Max Value")));
        deviceView.setHorizontalHeaderItem(4, new QTableWidgetItem(tr("Command")));
        deviceView.setHorizontalHeaderItem(5, new QTableWidgetItem(tr("Read back")));
        connect(&deviceView, SIGNAL(cellPressed(int, int)),SLOT(displayDevice(int, int)));
        connect(&qnapCommand, SIGNAL(finished(QNetworkReply*)), this, SLOT(httpCommandFinished(QNetworkReply*)));
        connect(&qnamGet, SIGNAL(finished(QNetworkReply*)), this, SLOT(httpGetModel(QNetworkReply*)));
        connect(&qnamHttp, SIGNAL(finished(QNetworkReply*)), this, SLOT(httpFinished(QNetworkReply*)));
    }

    void readDevice() {
        log.clear();
        if (!got_model) {
            log.append("Check device model\n");
            httpReq = "http://" + address->text() + "/shelly";
            QUrl url(httpReq);
            url_str = httpReq;
            log.append(QDateTime::currentDateTime().toString("HH:mm:ss:zzz") + "  " + httpReq + "\n");
            qnamGet.get(QNetworkRequest(url));
            //connect(&qnam, SIGNAL(finished(QNetworkReply*)), this, SLOT(httpGetModel(QNetworkReply*)));
            return;
        }
        if (model->text().startsWith("SHTRV")) {
            httpReq = "http://" + address->text() + "/status";
        }
        else if (model->text().startsWith("SNSW")) {
            httpReq = "http://" + address->text() + "/rpc/Shelly.GetStatus";
        }
        else {
            httpReq = "http://" + address->text() + "/status";
        }
        log.append(QDateTime::currentDateTime().toString("HH:mm:ss:zzz") + "  " + httpReq + "\n");
        QUrl url(httpReq);
        url_str = httpReq;
        qnamHttp.get(QNetworkRequest(url));
        //connect(&qnam, SIGNAL(finished(QNetworkReply*)), this, SLOT(httpFinished(QNetworkReply*)));
    }

    // thermostats/0/target_t/value
    // http://192.168.1.61/thermostat/0/?target_t=14
    // https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Shelly
    // http://192.168.1.71/rpc/Shelly.GetStatus
    // http://192.168.1.71/rpc/Switch.Set?id=0&on=true
    // http://192.168.1.71/rpc/Switch.Set?id=0&on=false
    // http://192.168.1.71/rpc/Switch.Toggle?id=0
    // https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Shelly
    // MQTT
    //https://shelly-api-docs.shelly.cloud/gen1/#shelly-trv
    // topic shellies/shellytrv-8CF681E1363A/thermostat/0/command
    // payload target_t=21
    // shellyplusht-08b61fcb738c

void setCommand(shellyHttpDevice *dev, QString command) {
        QString command_str = command;
        bool ok;
        if (model->text().startsWith("SHTRV")) {
        }
        else if (model->text().startsWith("SNSW")) {
            int v = command.toDouble(&ok);
            if (!ok) {
                ok = false;
                if (command == "on") { v = 1; ok = true; }
                if (command == "off") { v = 0; ok = true; }
                if (!ok) {
                    QString log;
                    log.append(QDateTime::currentDateTime().toString("HH:mm:ss:zzz") + " Command error :  " + command + "\n");
                    emit(logCommand(log));
                    return; } }
            if (v >= 1) command_str = "true"; else command_str = "false";
        }
        else {
        }
        log.clear();
        httpReq = "http://" + address->text() + dev->command_item.text() + command_str;
        log.append(QDateTime::currentDateTime().toString("HH:mm:ss:zzz") + "  " + httpReq + "\n");
        QUrl url(httpReq);
        url_str = httpReq;
        qnapCommand.get(QNetworkRequest(url));
        deviceCommand = dev;
        //connect(&qnam, SIGNAL(finished(QNetworkReply*)), this, SLOT(httpCommandFinished(QNetworkReply*)));
    }

    static QString ip2Hex(const QString &ip)
    {
        bool ok;
        int p1 = ip.indexOf(".");		// get first point position
        if (p1 == -1) return "";
        int p2 = ip.indexOf(".", p1 + 1);	// get second point position
        if (p2 == -1) return "";
        int p3 = ip.indexOf(".", p2 + 1);	// get third point position
        if (p3 == -1) return "";
        int l = ip.length();
        QString N1 = ip.mid(0, p1);
        if (N1 == "") return "";
        QString N2 = ip.mid(p1 + 1, p2 - p1 - 1);
        if (N2 == "") return "";
        QString N3 = ip.mid(p2 + 1, p3 - p2 - 1);
        if (N3 == "") return "";
        QString N4 = ip.mid(p3 + 1, l - p3 - 1);
        if (N4 == "") return "";
        int n1 = N1.toInt(&ok);
        if (!ok) return "";
        int n2 = N2.toInt(&ok);
        if (!ok) return "";
        int n3 = N3.toInt(&ok);
        if (!ok) return "";
        int n4 = N4.toInt(&ok);
        if (!ok) return "";
        return QString("%1%2%3%4").arg(n1, 2, 16, QLatin1Char('0')).arg(n2, 2, 16, QLatin1Char('0')).arg(n3, 2, 16, QLatin1Char('0')).arg(n4, 2, 16, QLatin1Char('0')).toUpper();
    }

    QString buildRomID(int n)
    {
        QString ipHex = ip2Hex(address->text());
        QString id = QString("%1").arg(n, 3, 10, QLatin1Char('0')).toUpper();
        QString RomID = ipHex + id + "SD";
        return RomID;
    }

    shellyHttpDevice *addHttpShellyDevice(QString parameterPath)
    {
        shellyHttpDevice *newDev = new shellyHttpDevice;
        if (newDev) {
            int index = httpShellyDevices.count();
            httpShellyDevices.append(newDev);
            newDev->RomID = buildRomID(httpShellyDevices.count());
            newDev->pathLocation = parameterPath;
            newDev->value = getValue(parameterPath);
            deviceView.insertRow(index);
            deviceView.setItem(index, 0, &newDev->RomID_item);
            newDev->RomID_item.setText(newDev->RomID);
            newDev->RomID_item.setFlags(newDev->RomID_item.flags() ^ Qt::ItemIsEditable);
            deviceView.setItem(index, 1, &newDev->path_item);
            newDev->path_item.setText(newDev->pathLocation);
            newDev->path_item.setFlags(newDev->path_item.flags() & (~Qt::ItemIsEditable) | (Qt::ItemIsSelectable));
            deviceView.setItem(index, 2, &newDev->value_item);
            newDev->value_item.setText(newDev->value);
            newDev->value_item.setFlags(newDev->value_item.flags() ^ Qt::ItemIsEditable);
            deviceView.setItem(index, 3, &newDev->maxValue_item);
            deviceView.setItem(index, 4, &newDev->command_item);
            deviceView.setItem(index, 5, &newDev->readback_item);
            emit(newDevice(newDev));
            return newDev;
        }
        return nullptr;
    }

    QString getValue(QString path) {
        QStringList listParameters = path.split("/");
        int parametersCount = listParameters.count();
        if (!listParameters.isEmpty()) {
            if (parametersCount == 1) {
                for (int n = 0; n<jsonmodel->rowCount(); n++) {
                    QModelIndex m = jsonmodel->index(n, 0);
                    QModelIndex d = jsonmodel->index(n, 1);
                    if (m.data(Qt::DisplayRole).toString() == path) {
                        return d.data(Qt::DisplayRole).toString(); } } }
            if (parametersCount == 2) {
                for (int n = 0; n<jsonmodel->rowCount(); n++) {
                    QModelIndex mdA = jsonmodel->index(n, 0);
                    if (mdA.data(Qt::DisplayRole).toString() == listParameters.at(0)) {
                        int i = 0;
                        QModelIndex mdB = jsonmodel->index(i, 0, mdA);
                        if (!mdB.isValid()) qDebug() << "mdB not valid";
                        while (mdB.isValid()) {
                            if (mdB.data(Qt::DisplayRole).toString() == listParameters.at(1)) {
                                return jsonmodel->index(i, 1, mdA).data(Qt::DisplayRole).toString(); }
                            mdB = jsonmodel->index(++i, 0, mdA); }
                        return ""; } } }
            if (parametersCount == 3) {
                for (int n = 0; n<jsonmodel->rowCount(); n++) {
                    QModelIndex mdA = jsonmodel->index(n, 0);
                    if (mdA.data(Qt::DisplayRole).toString() == listParameters.at(0)) {
                        int i = 0;
                        QModelIndex mdB = jsonmodel->index(i, 0, mdA);
                        while (mdB.isValid()) {
                            if (mdB.data(Qt::DisplayRole).toString() == listParameters.at(1)) {
                                i = 0;
                                QModelIndex mdC = jsonmodel->index(i, 0, mdB);
                                while (mdC.isValid()) {
                                    if (mdC.data(Qt::DisplayRole).toString() == listParameters.at(2)) {
                                        return jsonmodel->index(i, 1, mdB).data(Qt::DisplayRole).toString(); }
                                    mdC = jsonmodel->index(++i, 0, mdB); } }
                            mdB = jsonmodel->index(++i, 0, mdA); }
                        return ""; } } }
            if (parametersCount == 4) {
                for (int n = 0; n<jsonmodel->rowCount(); n++) {
                    QModelIndex mdA = jsonmodel->index(n, 0);
                    if (mdA.data(Qt::DisplayRole).toString() == listParameters.at(0)) {
                        int i = 0;
                        QModelIndex mdB = jsonmodel->index(i, 0, mdA);
                        while (mdB.isValid()) {
                            if (mdB.data(Qt::DisplayRole).toString() == listParameters.at(1)) {
                                i = 0;
                                QModelIndex mdC = jsonmodel->index(i, 0, mdB);
                                while (mdC.isValid()) {
                                    if (mdC.data(Qt::DisplayRole).toString() == listParameters.at(2)) {
                                        i = 0;
                                        QModelIndex mdD = jsonmodel->index(i, 0, mdC);
                                        while (mdD.isValid()) {
                                            if (mdD.data(Qt::DisplayRole).toString() == listParameters.at(3)) {
                                            return jsonmodel->index(i, 1, mdC).data(Qt::DisplayRole).toString(); }
                                        mdD = jsonmodel->index(++i, 0, mdC); }
                                        return ""; }
                                    mdC = jsonmodel->index(++i, 0, mdB); }
                                return ""; }
                            mdB = jsonmodel->index(++i, 0, mdA); }
                        return ""; } } } }
        return ""; }
private slots:
    void httpGetModel(QNetworkReply *reply) {
        QByteArray data = reply->readAll();
        log.append(QDateTime::currentDateTime().toString("HH:mm:ss:zzz") + "\n" + data + "\n");
        if (data.isEmpty()) qDebug() << "html data is empty " + url_str;
        jsonmodel->load(data);
        QString md = getValue("model");
        if (!md.isEmpty()) {
            got_model = true;
            model->setText(md);
            log.append("Model is " + md + "\n");
        }
        QString tp = getValue("type");
        if (!tp.isEmpty()) {
            got_model = true;
            model->setText(tp);
            log.append("Model is " + tp + "\n");
        }
        emit(logMe(log));
    }
    void httpFinished(QNetworkReply *reply) {
        QString error = reply->errorString();
        if (reply->error() == QNetworkReply::NoError)
        {
            deviceCommand = nullptr;
            QByteArray data = reply->readAll();
            log.append(QDateTime::currentDateTime().toString("HH:mm:ss:zzz") + "\n" + data + "\n");
            if (data.isEmpty()) qDebug() << "html data is empty " + url_str;
            jsonmodel->load(data);
            model->setToolTip(data);
            if (!got_model) {
                QString md = getValue("fw_info/device");
                if (!md.isEmpty()) {
                    got_model = true;
                    model->setText(md);
                    if (md.startsWith("SHTRV-01"))
                    {
                        log.append("Device is SHTRV-01\n");
                        if (httpShellyDevices.isEmpty()) {
                            if (QMessageBox::question(this, "Temperature device", "Do you want to create Temperature device ?", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) addHttpShellyDevice("thermostats/0/tmp/value");
                            shellyHttpDevice *dev = nullptr;
                            if (QMessageBox::question(this, "Temperature de consigne", "Do you want to create Temperature de Consigne ?", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) dev = addHttpShellyDevice("thermostats/0/target_t/value");
                            if (dev) { dev->command_item.setText("/thermostat/0/?target_t=");
                                dev->readback_item.setText("target_t/value");
                            }
                        } } } }
            lastRead->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
            foreach (shellyHttpDevice *dev, httpShellyDevices) {
                dev->value = getValue(dev->pathLocation);
                dev->value_item.setText(dev->value);
                log.append("New device value " + dev->pathLocation + " = " + dev->value + "\n");
                //qDebug() << "Shelly set value : " + dev->pathLocation + " = " + dev->value;
                emit(newDeviceValue(dev));
            }
        }
        else
        {
            log.append(QDateTime::currentDateTime().toString("HH:mm:ss:zzz") + "  " + error + "\n");
            model->setToolTip(error);
        }
        emit(logMe(log));
        reply->deleteLater();
    }
    void httpCommandFinished(QNetworkReply *replyCommand) {
        QString error = replyCommand->errorString();
        if (replyCommand->error() == QNetworkReply::NoError)
        {
            QByteArray data = replyCommand->readAll();
            log.append(QDateTime::currentDateTime().toString("HH:mm:ss:zzz") + "\n" + data + "\n");
            if (data.isEmpty()) qDebug() << "html data is empty " + url_str;
            jsonmodel->load(data);
            model->setToolTip(data);
            if (deviceCommand) {
                QString value = getValue(deviceCommand->readback_item.text());
                if (!value.isEmpty()) {
                    lastRead->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
                    deviceCommand->value = value;
                    deviceCommand->value_item.setText(deviceCommand->value);
                    //emit(newDeviceValue(deviceCommand));
                }
            }
        }
        else
        {
            log.append(QDateTime::currentDateTime().toString("HH:mm:ss:zzz") + "  " + error + "\n");
            model->setToolTip(error);
        }
        emit(logCommand(log));
        readDevice();
        replyCommand->deleteLater();
    }
};




class ShellyPlugin : public QWidget, LogisDomInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "logisdom.network.LogisDomInterface/1.0" FILE "ShellyPlugin.json")
    Q_INTERFACES(LogisDomInterface)
public:
    ShellyPlugin( QWidget *parent = nullptr );
    ~ShellyPlugin() override;
    QObject* getObject() override;
    QWidget *widgetUi() override;
    QWidget *getDevWidget(QString RomID) override;
    void setConfigFileName(const QString) override;
    void readDevice(const QString &device) override;
    QString getDeviceCommands(const QString &device) override;
    void saveConfig() override;
    void readConfig() override;
    void setLockedState(bool) override;
    bool lockedState = true;
    QString getDeviceConfig(QString) override;
    void setDeviceConfig(const QString &, const QString &) override;
    QString getName() override;
    void setStatus(const QString) override;
    bool acceptCommand(const QString) override;
    bool isDimmable(const QString) override;
    bool isManual(const QString) override;
    double getMaxValue(const QString) override;
    QTimer readTimer;
    QMqttClient *m_client;
    int lastMinute = -1;
    int idle = 0;
    bool loadConfig = false;
signals:
    void newDeviceValue(QString, QString) override;
    void newDevice(LogisDomInterface*, QString) override;
    void deviceSelected(QString) override;
    void updateInterfaceName(LogisDomInterface*, QString) override;
    void connectionStatus(QAbstractSocket::SocketState) override;
private:
    QWidget *ui;
    QString configFileName;
    QList<shellyMainDevice*> shellyMainDevices;
    QList<shellyMqttDevice*> shellyDevicesMqtt, deviceSetTable;
    QString ipaddress;
    quint16 port = 80;
    Ui::ShellyUI *mui;
    QString lastStatus;
    void log(const QString);
    QString logStr;
    shellyMainDevice *addDevice();
    shellyMqttDevice *addMqttShellyDevice(const QString &, QString Name = "");
    QModelIndex getValue(QString);
    QJsonTreeItem *getMqttTreeItem(QString);
    QString buildRomID(int n);
    void readHttpDevices(QList<shellyMainDevice*>& devList);
    void readMqttDevices(QList<shellyMqttDevice*>& devList);
    QString selectedHttpPath();
    QString selectedMqttPath();
    QJsonModel *jsonmodel;
    QTreeView *mqttTree;
private slots:
    void AddDeviceClick();
    void AddHttpParameterClick();
    void AddMqttParameterClick();
    void readAllNow();
    void startMqtt();
    void displayHttpDevice(int, int);
    void displayMqttDevice(int, int);
    void ProvideContexMenu(const QPoint&);
    void on_editName_editingFinished();
    void on_ReadButton_clicked();
    void on_ReadAllButton_clicked();
    void on_RemoveButton_clicked();
    void on_buttonConnect_clicked();
    void on_buttonPublish_clicked();
    void on_buttonSubscribe_clicked();
    void handleMessage(const QMqttMessage &qmsg);
    void setClientPort(int p);
    void brokerConnected();
    void brokerDisconnected();
    void updateLogStateChange();
    void newHttpShellyDevice(const shellyHttpDevice*);
    void newMqttShellyDevice(const shellyMqttDevice*);
    void newHttpShellyDevice(const shellyHttpDevice*, QString);
    void newMqttShellyDevice(const shellyMqttDevice*, QString);
    void newHttpShellyDeviceValue(const shellyHttpDevice*);
    void newMqttShellyDeviceValue(const shellyMqttDevice*);
    void showHttpDevice(const shellyHttpDevice*);
    void showMqttDevice(const shellyMqttDevice*);
    void httpLogThis(const QString &);
    void httpLogCommand(const QString &);
    void mqttLogThis(const QString &);
    void mqttLogCommand(const QString &);
    void httpPathSelected();
    void mqttPathSelected();
    void deviceTableSelection();
    void httpLogChanged();
    void mqttLogChanged();
    void clearHttpLog();
    void clearMqttLog();
    void setLockedState();
    void newValue(QString, QString);
};


#endif

