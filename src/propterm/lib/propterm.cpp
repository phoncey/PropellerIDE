#include "propterm.h"
#include "console.h"

#include <QDebug>

Q_LOGGING_CATEGORY(terminal, "ide.terminal")

PropTerm::PropTerm(PropellerManager * manager,
                   QWidget *parent)
: QWidget(parent)
{
    ui.setupUi(this);

    this->manager = manager;
    this->session = manager->session();

    timeout = 100;

    rxTimeout.setSingleShot(true);
    txTimeout.setSingleShot(true);

    toggleRxLight(false);
    toggleTxLight(false);

    connect(&rxTimeout, SIGNAL(timeout()), this, SLOT(turnOffRxLight()));
    connect(&txTimeout, SIGNAL(timeout()), this, SLOT(turnOffTxLight()));

    connect(manager, SIGNAL(portListChanged()), this, SLOT(updatePorts()));
    connect(session, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(session, SIGNAL(deviceFree()), this, SLOT(open()));
    connect(session, SIGNAL(deviceBusy()), this, SLOT(close()));
    connect(ui.console, SIGNAL(getData(QByteArray)), this, SLOT(writeData(QByteArray)));

    connect(ui.sendLineEdit, SIGNAL(returnPressed()), this, SLOT(sendDataLine()));
    connect(ui.sendButton, SIGNAL(pressed()), this, SLOT(sendDataLine()));

    connect(ui.port, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(portChanged()));
    connect(ui.baudRate, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(baudRateChanged(const QString &)));

    connect(ui.clear, SIGNAL(clicked()), ui.console, SLOT(clear()));
    connect(ui.activeButton, SIGNAL(toggled(bool)), this, SLOT(handleEnable(bool)));

    ui.echo->setChecked(true);

    title = tr("Propeller Terminal");
    setWindowTitle(title);

    updatePorts();
    ui.console->clear();
    open();
}

PropTerm::~PropTerm()
{
    close();
    manager->endSession(session);
}

void PropTerm::setFont(const QFont & font)
{
    ui.console->setFont(font);
    ui.sendLineEdit->setFont(font);
}

void PropTerm::updatePorts()
{
    ui.port->clear();
    foreach(QString s, manager->listPorts())
    {
        ui.port->addItem(s);
    }
}

void PropTerm::turnOffRxLight()
{
    toggleRxLight(false);
}

void PropTerm::turnOffTxLight()
{
    toggleTxLight(false);
}

void PropTerm::toggleRxLight(bool enabled)
{
    if (enabled)
    {
        rxTimeout.start(timeout);
        ui.rxLight->setPixmap(QPixmap(":/icons/propterm/led-red.png"));
    }
    else
    {
        rxTimeout.stop();
        ui.rxLight->setPixmap(QPixmap(":/icons/propterm/led-off.png"));
    }
}

void PropTerm::toggleTxLight(bool enabled)
{
    if (enabled)
    {
        txTimeout.start(timeout);
        ui.txLight->setPixmap(QPixmap(":/icons/propterm/led-blue.png"));
    }
    else
    {
        txTimeout.stop();
        ui.txLight->setPixmap(QPixmap(":/icons/propterm/led-off.png"));
    }
}

void PropTerm::message(QString text)
{
    text = "[PropellerTerminal]: "+text;
    fprintf(stderr, "%s\n", qPrintable(text));
    fflush(stderr);
}

void PropTerm::error(QString text)
{
    message("ERROR: "+text);
}

void PropTerm::open()
{
    ui.console->enable(true);
    ui.console->setEnabled(true);
    ui.sendButton->setEnabled(true);
    ui.sendLineEdit->setEnabled(true);
    ui.activeLight->setPixmap(QPixmap(":/icons/propterm/led-green.png"));

    portChanged();
    session->unpause();
    baudRateChanged(ui.baudRate->currentText());
}

void PropTerm::close()
{
    ui.console->enable(false);
    ui.console->setEnabled(false);
    ui.sendButton->setEnabled(false);
    ui.sendLineEdit->setEnabled(false);
    ui.activeLight->setPixmap(QPixmap(":/icons/propterm/led-off.png"));

    session->pause();
}

void PropTerm::portChanged()
{
    qCDebug(terminal) << "port" << ui.port->currentText();
    session->setPortName(ui.port->currentText());
    setWindowTitle(tr("%1 - %2").arg(session->portName()).arg(title));
}

void PropTerm::baudRateChanged(const QString & text)
{
    bool ok;
    int baud = text.toInt(&ok);
    if (!ok)
    {
        qCCritical(terminal) << "Baud rate invalid:" << baud;
        return;
    }

    session->setBaudRate(baud);
    qCDebug(terminal) << "new baud rate:" << baud;
}

void PropTerm::sendDataLine()
{
    QByteArray data = ui.sendLineEdit->text().toLocal8Bit();
    data.append(13);
    writeData(data);
    ui.sendLineEdit->clear();
}

void PropTerm::writeData(const QByteArray &data)
{
    toggleTxLight(true);

    session->write(data);
    if (ui.echo->isChecked())
        ui.console->putData(data);
}

void PropTerm::readData()
{
    toggleRxLight(true);

    QByteArray data = session->readAll();
    ui.console->putData(data);
}

void PropTerm::handleError()
{
    close();
}

void PropTerm::handleEnable(bool checked)
{
    if (checked)
        open();
    else
        close();
}
