/*
 * EJERCICIO 2: Chat con Interfaz Gráfica Qt - Versión Menu & Navigation & Alerts
 * 
 * Comando de compilación:
 * g++ -fPIC prueba.cc -o prueba $(pkg-config --cflags --libs Qt5Widgets Qt5Network Qt5Sql)
 */

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QUdpSocket>
#include <QHostAddress>
#include <QString>
#include <QByteArray>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QMap>
#include <QTimer>
#include <QtSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDialog>
#include <QFormLayout>
#include <QListWidget>

// --- Database Helpers ---

void initDb() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("BaseDeDatos.db");
    if (!db.open()) qDebug() << "Error DB";
    
    QSqlQuery q;
    q.exec("CREATE TABLE IF NOT EXISTS messages (id TEXT PRIMARY KEY, content TEXT, timestamp TEXT, is_mine INTEGER, status INTEGER, remote_port INTEGER)");
    q.exec("CREATE TABLE IF NOT EXISTS alerts (id TEXT PRIMARY KEY, content TEXT, priority INTEGER, timestamp TEXT, is_sent INTEGER, remote_port INTEGER)");
    q.exec("CREATE TABLE IF NOT EXISTS incidents (id TEXT PRIMARY KEY, name TEXT, description TEXT, timestamp TEXT)");
    q.exec("CREATE TABLE IF NOT EXISTS alerts (id TEXT PRIMARY KEY, content TEXT, priority INTEGER, timestamp TEXT, is_sent INTEGER, remote_port INTEGER)");
    q.exec("CREATE TABLE IF NOT EXISTS incidents (id TEXT PRIMARY KEY, name TEXT, description TEXT, timestamp TEXT)");
    q.exec("CREATE TABLE IF NOT EXISTS minutes (id TEXT PRIMARY KEY, title TEXT, content TEXT, timestamp TEXT)");
    q.exec("CREATE TABLE IF NOT EXISTS chat_messages (id TEXT PRIMARY KEY, sender TEXT, receiver TEXT, content TEXT, timestamp TEXT)");
}

void saveMessage(const QString& id, const QString& content, const QString& timestamp, bool isMine, int status, int remotePort) {
    QSqlQuery query;
    query.prepare("INSERT INTO messages VALUES (:id, :content, :timestamp, :is_mine, :status, :remote_port)");
    query.bindValue(":id", id); query.bindValue(":content", content); query.bindValue(":timestamp", timestamp);
    query.bindValue(":is_mine", isMine ? 1 : 0); query.bindValue(":status", status); query.bindValue(":remote_port", remotePort);
    query.exec();
}

void saveAlert(const QString& id, const QString& content, int priority, const QString& timestamp, bool isSent, int remotePort) {
    QSqlQuery query;
    query.prepare("INSERT INTO alerts VALUES (:id, :content, :priority, :timestamp, :is_sent, :remote_port)");
    query.bindValue(":id", id); query.bindValue(":content", content); query.bindValue(":priority", priority);
    query.bindValue(":timestamp", timestamp); query.bindValue(":is_sent", isSent ? 1 : 0); query.bindValue(":remote_port", remotePort);
    query.exec();
}

void saveIncident(const QString& id, const QString& name, const QString& description, const QString& timestamp) {
    QSqlQuery query;
    query.prepare("INSERT INTO incidents VALUES (:id, :name, :description, :timestamp)");
    query.bindValue(":id", id); query.bindValue(":name", name); query.bindValue(":description", description);
    query.bindValue(":timestamp", timestamp);
    query.exec();
}

void saveMinute(const QString& id, const QString& title, const QString& content, const QString& timestamp) {
    QSqlQuery query;
    query.prepare("INSERT INTO minutes VALUES (:id, :title, :content, :timestamp)");
    query.bindValue(":id", id); query.bindValue(":title", title); query.bindValue(":content", content);
    query.bindValue(":timestamp", timestamp);
    query.exec();
}

void saveChatMessage(const QString& id, const QString& sender, const QString& receiver, const QString& content, const QString& timestamp) {
    QSqlQuery query;
    query.prepare("INSERT INTO chat_messages VALUES (:id, :sender, :receiver, :content, :timestamp)");
    query.bindValue(":id", id); query.bindValue(":sender", sender); query.bindValue(":receiver", receiver);
    query.bindValue(":content", content); query.bindValue(":timestamp", timestamp);
    query.exec();
}

void markMessageAsRead(const QString& id) {
    QSqlQuery query;
    query.prepare("UPDATE messages SET status = 1 WHERE id = :id");
    query.bindValue(":id", id);
    query.exec();
}

// --- UI Helpers ---

QWidget* createMessageWidget(const QString& text, const QString& time, bool isMine, int status, QLabel** statusLabelOut = nullptr) {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(10, 5, 10, 5);

    QWidget* bubble = new QWidget();
    bubble->setStyleSheet(isMine 
        ? "background-color: #005C4B; border-radius: 15px; padding: 10px;" 
        : "background-color: #202C33; border-radius: 15px; padding: 10px;" 
    );
    
    QVBoxLayout* bubbleLayout = new QVBoxLayout(bubble);
    bubbleLayout->setContentsMargins(12, 12, 12, 12);
    bubbleLayout->setSpacing(5);

    QLabel* msgLabel = new QLabel(text);
    msgLabel->setWordWrap(true);
    msgLabel->setStyleSheet("color: #E9EDEF; font-size: 15px; background: transparent;");
    msgLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubbleLayout->addWidget(msgLabel);

    QHBoxLayout* metaLayout = new QHBoxLayout();
    metaLayout->addStretch();
    QLabel* timeLabel = new QLabel(time);
    timeLabel->setStyleSheet("color: #8696A0; font-size: 11px; background: transparent;");
    metaLayout->addWidget(timeLabel);

    if (isMine) {
        QLabel* statusLabel = new QLabel(status == 1 ? "✓✓" : "✓");
        statusLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold; background: transparent; margin-left: 5px;").arg(status == 1 ? "#53BDEB" : "#8696A0"));
        if (statusLabelOut) *statusLabelOut = statusLabel;
        metaLayout->addWidget(statusLabel);
    }
    bubbleLayout->addLayout(metaLayout);

    if (isMine) { layout->addStretch(); layout->addWidget(bubble); } 
    else { layout->addWidget(bubble); layout->addStretch(); }
    bubble->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    return widget;
}

// --- Login Dialog ---
class LoginDialog : public QDialog {
public:
    QString role;
    QString userEmail;
    QString userDNI;
    QString targetIP;
    int myPort;
    int targetPort;
    QLineEdit *emailEdit;
    QLineEdit *ipEdit;
    QLineEdit *myPortEdit;
    QLineEdit *targetPortEdit;

    LoginDialog() {
        setWindowTitle("Inicio de Sesión & Red");
        setFixedSize(350, 450);
        setStyleSheet("background-color: #111B21; color: #E9EDEF;");
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setSpacing(15); layout->setContentsMargins(30, 30, 30, 30);

        QLabel *title = new QLabel("Bienvenido");
        title->setStyleSheet("font-size: 24px; font-weight: bold; color: #00A884; margin-bottom: 10px;");
        title->setAlignment(Qt::AlignCenter);
        layout->addWidget(title);

        emailEdit = new QLineEdit(this);
        emailEdit->setPlaceholderText("Correo (@tutor.es / @alumno.es)");
        layout->addWidget(emailEdit);
        
        QLineEdit *passEdit = new QLineEdit(this);
        passEdit->setPlaceholderText("Contraseña");
        passEdit->setEchoMode(QLineEdit::Password);
        layout->addWidget(passEdit);

        // Network Config
        QLabel *netLabel = new QLabel("Configuración de Red:");
        netLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
        layout->addWidget(netLabel);

        ipEdit = new QLineEdit(this);
        ipEdit->setPlaceholderText("IP Destino (ej: 192.168.1.5)");
        ipEdit->setText("127.0.0.1");
        layout->addWidget(ipEdit);

        QHBoxLayout *portsLayout = new QHBoxLayout();
        myPortEdit = new QLineEdit(this); myPortEdit->setPlaceholderText("Mi Puerto"); myPortEdit->setText("5000");
        targetPortEdit = new QLineEdit(this); targetPortEdit->setPlaceholderText("Puerto Destino"); targetPortEdit->setText("5001");
        portsLayout->addWidget(myPortEdit); portsLayout->addWidget(targetPortEdit);
        layout->addLayout(portsLayout);
        
        QPushButton *loginBtn = new QPushButton("Entrar", this);
        layout->addWidget(loginBtn);
        
        connect(loginBtn, &QPushButton::clicked, [this](){
            QString inputEmail = emailEdit->text().trimmed();
            targetIP = ipEdit->text().trimmed();
            myPort = myPortEdit->text().toInt();
            targetPort = targetPortEdit->text().toInt();

            if (targetIP.isEmpty() || myPort <= 0 || targetPort <= 0) {
                QMessageBox::warning(this, "Error", "Configuración de red inválida.");
                return;
            }
            
            // Check Coordinator
            if (inputEmail.endsWith("@coordinador.es")) { 
                role = "COORDINADOR"; 
                userEmail = inputEmail;
                userDNI = "COORDINADOR";
                accept(); 
                return;
            }

            QSqlQuery q;
            // Check Tutor
            q.prepare("SELECT * FROM tutores_prueba WHERE correo = :email");
            q.bindValue(":email", inputEmail);
            if (q.exec() && q.next()) {
                role = "TUTOR";
                userEmail = inputEmail;
                userDNI = q.value("DNI").toString();
                accept();
                return;
            }

            // Check Student
            q.prepare("SELECT * FROM alumnos_prueba WHERE correo = :email");
            q.bindValue(":email", inputEmail);
            if (q.exec() && q.next()) {
                role = "ALUMNO";
                userEmail = inputEmail;
                userDNI = q.value("DNI").toString();
                accept();
                return;
            }

            QMessageBox::warning(this, "Error", "Correo no encontrado en la base de datos.");
        });
    }
};

// --- Forward Declaration ---
class MainMenu;
class ContactListWindow;

// --- ContactList Window ---
class ContactListWindow : public QWidget {
public:
    MainMenu* menuRef;
    QVBoxLayout* listLayout;
    QScrollArea* scrollArea;
    QString userRole;
    QString userDNI;

    ContactListWindow(MainMenu* menu, QString role, QString dni);
    void loadContacts();
};

// --- Alert Window ---
class AlertWindow : public QWidget {
public:
    MainMenu* menuRef;
    QString userRole;
    QVBoxLayout* listLayout;
    QScrollArea* scrollArea;

    AlertWindow(QString role, MainMenu* menu);
    void addAlertWidget(const QString& content, int priority, const QString& time, bool isSent);
    void loadAlerts(int targetPort);
};

// --- Incidents Window ---
class IncidentsWindow : public QWidget {
public:
    MainMenu* menuRef;
    QVBoxLayout* listLayout;
    QScrollArea* scrollArea;
    QLineEdit* nameInput;
    QLineEdit* descInput;

    IncidentsWindow(MainMenu* menu);
    void addIncidentWidget(const QString& name, const QString& desc, const QString& time);
    void loadIncidents();
};

// --- Minutes Window ---
class MinutesWindow : public QWidget {
public:
    MainMenu* menuRef;
    QVBoxLayout* listLayout;
    QScrollArea* scrollArea;
    QLineEdit* titleInput;
    QLineEdit* contentInput;

    MinutesWindow(MainMenu* menu);
    void addMinuteWidget(const QString& title, const QString& content, const QString& time);
    void loadMinutes();
};

// --- Chat Window ---
class ChatWindow : public QWidget {
public:
    QVBoxLayout *scrollLayout;
    QScrollArea *scrollArea;
    QLineEdit *messageInput;
    QMap<QString, QLabel*> pendingAcks;
    QString userRole;
    QString currentUserEmail;
    QString remoteUserEmail;
    MainMenu* menuRef;

    ChatWindow(QString myEmail, QString otherEmail, QString role, MainMenu* menu);

    void loadHistory();
    void scrollToBottom();
    void addMessage(const QString& text, const QString& time, bool isMine, int status, const QString& id = "");
    void updateAck(const QString& id);
};

// --- Main Menu ---
class MainMenu : public QWidget {
public:
    QString userRole;
    QString userEmail;
    QString userDNI;
    QString targetIP;
    int myPort, targetPort;
    QUdpSocket *socket;
    
    ContactListWindow *contactListWindow = nullptr;
    AlertWindow *alertWindow = nullptr;
    IncidentsWindow *incidentsWindow = nullptr;
    MinutesWindow *minutesWindow = nullptr;

    MainMenu(QString role, QString email, QString dni, QString ip, int myP, int targetP) 
        : userRole(role), userEmail(email), userDNI(dni), targetIP(ip), myPort(myP), targetPort(targetP) {
        setWindowTitle("Menú Principal");
        resize(400, 600);
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setSpacing(20); layout->setContentsMargins(30, 50, 30, 50);

        QLabel *icon = new QLabel(userRole == "TUTOR" ? "🎓" : (userRole == "COORDINADOR" ? "�" : "�👨‍🎓"));
        icon->setAlignment(Qt::AlignCenter); icon->setStyleSheet("font-size: 60px; margin-bottom: 10px;");
        layout->addWidget(icon);

        QLabel *welcome = new QLabel("Bienvenido, " + userRole);
        welcome->setAlignment(Qt::AlignCenter); welcome->setStyleSheet("font-size: 24px; font-weight: bold; color: #E9EDEF;");
        layout->addWidget(welcome);
        layout->addStretch();

        QPushButton *btnChats = new QPushButton("💬  CHATS");
        btnChats->setMinimumHeight(60);
        btnChats->setStyleSheet("background-color: #202C33; color: #00A884; font-size: 18px; border-radius: 10px; text-align: left; padding-left: 30px;");
        layout->addWidget(btnChats);

        QPushButton *btnAlerts = new QPushButton("⚠️  ALERTAS");
        btnAlerts->setMinimumHeight(60);
        btnAlerts->setStyleSheet("background-color: #202C33; color: #FFB000; font-size: 18px; border-radius: 10px; text-align: left; padding-left: 30px;");
        layout->addWidget(btnAlerts);

        QPushButton *btnIncidents = new QPushButton("📋  INCIDENCIAS");
        btnIncidents->setMinimumHeight(60);
        btnIncidents->setStyleSheet("background-color: #202C33; color: #FF5252; font-size: 18px; border-radius: 10px; text-align: left; padding-left: 30px;");
        
        QPushButton *btnMinutes = new QPushButton("📝  ACTAS");
        btnMinutes->setMinimumHeight(60);
        btnMinutes->setStyleSheet("background-color: #202C33; color: #00E676; font-size: 18px; border-radius: 10px; text-align: left; padding-left: 30px;");

        if (userRole == "COORDINADOR") {
            btnAlerts->hide();
            layout->addWidget(btnIncidents);
            btnMinutes->hide();
        } else if (userRole == "TUTOR") {
            layout->addWidget(btnMinutes);
            btnIncidents->hide();
        } else {
            btnIncidents->hide();
            btnMinutes->hide();
        }
        layout->addStretch();
        
        QLabel *footer = new QLabel("Puerto: " + QString::number(myPort));
        footer->setAlignment(Qt::AlignCenter); footer->setStyleSheet("color: #8696A0;");
        layout->addWidget(footer);

        // Socket Initialization
        socket = new QUdpSocket(this);
        if (!socket->bind(QHostAddress::Any, myPort)) QMessageBox::critical(this, "Error", "Puerto ocupado");
        connect(socket, &QUdpSocket::readyRead, this, &MainMenu::onReadyRead);

        connect(socket, &QUdpSocket::readyRead, this, &MainMenu::onReadyRead);

        connect(btnChats, &QPushButton::clicked, [this](){
            if (!contactListWindow) contactListWindow = new ContactListWindow(this, userRole, userDNI);
            contactListWindow->loadContacts();
            contactListWindow->show();
            this->hide();
        });

        connect(btnAlerts, &QPushButton::clicked, [this](){
            if (!alertWindow) alertWindow = new AlertWindow(userRole, this);
            alertWindow->loadAlerts(targetPort);
            alertWindow->show();
            this->hide();
        });

        connect(btnIncidents, &QPushButton::clicked, [this](){
            if (!incidentsWindow) incidentsWindow = new IncidentsWindow(this);
            incidentsWindow->loadIncidents();
            incidentsWindow->show();
            this->hide();
        });

        connect(btnMinutes, &QPushButton::clicked, [this](){
            if (!minutesWindow) minutesWindow = new MinutesWindow(this);
            minutesWindow->loadMinutes();
            minutesWindow->show();
            this->hide();
        });
    }

    void sendPacket(const QString& content) {
        socket->writeDatagram(content.toUtf8(), QHostAddress(targetIP), targetPort);
    }

    void onReadyRead() {
        while (socket->hasPendingDatagrams()) {
            QByteArray d; d.resize(socket->pendingDatagramSize());
            QHostAddress s; quint16 p;
            socket->readDatagram(d.data(), d.size(), &s, &p);
            QStringList parts = QString::fromUtf8(d).split("|");

            if (parts.size() >= 2) {
                QString type = parts[0], id = parts[1];
                
                if (type == "MSG") {
                    if (parts.size() >= 4) {
                        QString senderEmail = parts[2];
                        QString content = parts.mid(3).join("|");
                        QString time = QDateTime::currentDateTime().toString("HH:mm");
                        saveChatMessage(id, senderEmail, userEmail, content, time);
                        QApplication::alert(this);
                    }
                } else if (type == "ALERT" && parts.size() >= 4) {
                    int priority = parts[2].toInt();
                    QString content = parts.mid(3).join("|");
                    QString time = QDateTime::currentDateTime().toString("HH:mm");
                    saveAlert(id, content, priority, time, false, targetPort);

                    if (alertWindow && alertWindow->isVisible()) {
                        alertWindow->addAlertWidget(content, priority, time, false);
                    }
                    QMessageBox::warning(this, "⚠️ Nueva Alerta", content);
                    QApplication::alert(this);

                } else if (type == "ACK") {
                    markMessageAsRead(id);
                }
            }
        }
    }
};

// --- AlertWindow Implementation ---
AlertWindow::AlertWindow(QString role, MainMenu* menu) : userRole(role), menuRef(menu) {
    setWindowTitle("Alertas");
    resize(400, 600);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);

    // Header
    QWidget* header = new QWidget();
    header->setStyleSheet("background-color: #202C33; padding: 10px;");
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    QPushButton *backBtn = new QPushButton("⬅");
    backBtn->setFixedSize(30, 30);
    backBtn->setStyleSheet("background: transparent; color: #FFB000; font-size: 20px; border: none;");
    connect(backBtn, &QPushButton::clicked, [this](){ this->hide(); if (menuRef) menuRef->show(); });
    headerLayout->addWidget(backBtn);
    QLabel* title = new QLabel("Alertas");
    title->setStyleSheet("font-weight: bold; font-size: 18px; color: #FFB000;");
    headerLayout->addWidget(title); headerLayout->addStretch();
    layout->addWidget(header);

    // Content
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("background-color: #0B141A;");
    QWidget *scrollContent = new QWidget();
    listLayout = new QVBoxLayout(scrollContent);
    listLayout->setContentsMargins(20, 20, 20, 20); listLayout->setSpacing(15); listLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    layout->addWidget(scrollArea);

    if (userRole == "ALUMNO") {
        QWidget* controls = new QWidget();
        controls->setStyleSheet("background-color: #202C33; padding: 20px;");
        QVBoxLayout* ctrlLayout = new QVBoxLayout(controls);
        
        auto createAlertBtn = [&](const QString& text, int priority, const QString& color) {
            QPushButton* btn = new QPushButton(text);
            btn->setStyleSheet(QString("background-color: %1; color: #111B21; font-weight: bold; padding: 15px; border-radius: 10px;").arg(color));
            connect(btn, &QPushButton::clicked, [this, text, priority, menu](){
                QString id = QString::number(QDateTime::currentMSecsSinceEpoch());
                QString time = QDateTime::currentDateTime().toString("HH:mm");
                saveAlert(id, text, priority, time, true, menu->targetPort);
                menu->sendPacket("ALERT|" + id + "|" + QString::number(priority) + "|" + text);
                addAlertWidget(text, priority, time, true);
                QMessageBox::information(this, "Enviado", "Alerta enviada al tutor.");
            });
            ctrlLayout->addWidget(btn);
        };

        createAlertBtn("🔴 Necesito ayuda urgente", 1, "#FF5252");
        createAlertBtn("🟠 Duda con el ejercicio", 2, "#FFB74D");
        createAlertBtn("🟢 Tarea completada", 3, "#69F0AE");
        layout->addWidget(controls);
    }
}

void AlertWindow::addAlertWidget(const QString& content, int priority, const QString& time, bool isSent) {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    
    QString color = (priority == 1) ? "#FF5252" : (priority == 2) ? "#FFB74D" : "#69F0AE";
    QString icon = (priority == 1) ? "🔴" : (priority == 2) ? "🟠" : "🟢";
    
    QLabel* lbl = new QLabel(icon + " " + content + "\n🕒 " + time);
    lbl->setStyleSheet(QString("background-color: %1; color: #111B21; padding: 10px; border-radius: 10px; font-weight: bold;").arg(color));
    
    if (isSent) { layout->addStretch(); layout->addWidget(lbl); }
    else { layout->addWidget(lbl); layout->addStretch(); }
    
    // Insert before the stretch item at the end
    listLayout->insertWidget(listLayout->count() - 1, widget);
    QTimer::singleShot(100, [this](){ scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum()); });
}

void AlertWindow::loadAlerts(int targetPort) {
    // Clear existing (except stretch)
    QLayoutItem* item;
    while ((item = listLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        else if (item->spacerItem()) { listLayout->addItem(item); break; } // Keep stretch
        delete item;
    }

    QSqlQuery q;
    q.prepare("SELECT content, priority, timestamp, is_sent FROM alerts WHERE remote_port = :p ORDER BY id ASC");
    q.bindValue(":p", targetPort);
    if (q.exec()) {
        while (q.next()) {
            addAlertWidget(q.value("content").toString(), q.value("priority").toInt(), q.value("timestamp").toString(), q.value("is_sent").toBool());
        }
    }
}

// --- IncidentsWindow Implementation ---
IncidentsWindow::IncidentsWindow(MainMenu* menu) : menuRef(menu) {
    setWindowTitle("Incidencias");
    resize(400, 600);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);

    // Header
    QWidget* header = new QWidget();
    header->setStyleSheet("background-color: #202C33; padding: 10px;");
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    QPushButton *backBtn = new QPushButton("⬅");
    backBtn->setFixedSize(30, 30);
    backBtn->setStyleSheet("background: transparent; color: #FF5252; font-size: 20px; border: none;");
    connect(backBtn, &QPushButton::clicked, [this](){ this->hide(); if (menuRef) menuRef->show(); });
    headerLayout->addWidget(backBtn);
    QLabel* title = new QLabel("Incidencias");
    title->setStyleSheet("font-weight: bold; font-size: 18px; color: #FF5252;");
    headerLayout->addWidget(title); headerLayout->addStretch();
    layout->addWidget(header);

    // Content
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("background-color: #0B141A;");
    QWidget *scrollContent = new QWidget();
    listLayout = new QVBoxLayout(scrollContent);
    listLayout->setContentsMargins(20, 20, 20, 20); listLayout->setSpacing(15); listLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    layout->addWidget(scrollArea);

    // Creation Form
    QWidget* form = new QWidget();
    form->setStyleSheet("background-color: #202C33; padding: 20px;");
    QVBoxLayout* formLayout = new QVBoxLayout(form);
    
    nameInput = new QLineEdit();
    nameInput->setPlaceholderText("Nombre de la incidencia");
    formLayout->addWidget(nameInput);
    
    descInput = new QLineEdit();
    descInput->setPlaceholderText("Descripción");
    formLayout->addWidget(descInput);
    
    QPushButton* createBtn = new QPushButton("Crear Incidencia");
    createBtn->setStyleSheet("background-color: #FF5252; color: #111B21; font-weight: bold; padding: 10px; border-radius: 10px; margin-top: 10px;");
    connect(createBtn, &QPushButton::clicked, [this](){
        QString name = nameInput->text().trimmed();
        QString desc = descInput->text().trimmed();
        if (name.isEmpty() || desc.isEmpty()) {
            QMessageBox::warning(this, "Error", "Rellena todos los campos.");
            return;
        }
        QString id = QString::number(QDateTime::currentMSecsSinceEpoch());
        QString time = QDateTime::currentDateTime().toString("HH:mm");
        saveIncident(id, name, desc, time);
        addIncidentWidget(name, desc, time);
        nameInput->clear(); descInput->clear();
        QMessageBox::information(this, "Éxito", "Incidencia creada.");
    });
    formLayout->addWidget(createBtn);
    layout->addWidget(form);
}

void IncidentsWindow::addIncidentWidget(const QString& name, const QString& desc, const QString& time) {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QLabel* lbl = new QLabel("📋 " + name + "\n" + desc + "\n🕒 " + time);
    lbl->setStyleSheet("background-color: #FF5252; color: #111B21; padding: 10px; border-radius: 10px; font-weight: bold;");
    lbl->setWordWrap(true);
    
    layout->addWidget(lbl);
    
    // Insert before the stretch item at the end
    listLayout->insertWidget(listLayout->count() - 1, widget);
    QTimer::singleShot(100, [this](){ scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum()); });
}

void IncidentsWindow::loadIncidents() {
    // Clear existing (except stretch)
    QLayoutItem* item;
    while ((item = listLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        else if (item->spacerItem()) { listLayout->addItem(item); break; } // Keep stretch
        delete item;
    }

    QSqlQuery q;
    q.prepare("SELECT name, description, timestamp FROM incidents ORDER BY id ASC");
    if (q.exec()) {
        while (q.next()) {
            addIncidentWidget(q.value("name").toString(), q.value("description").toString(), q.value("timestamp").toString());
        }
    }
}

// --- MinutesWindow Implementation ---
MinutesWindow::MinutesWindow(MainMenu* menu) : menuRef(menu) {
    setWindowTitle("Actas");
    resize(400, 600);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);

    // Header
    QWidget* header = new QWidget();
    header->setStyleSheet("background-color: #202C33; padding: 10px;");
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    QPushButton *backBtn = new QPushButton("⬅");
    backBtn->setFixedSize(30, 30);
    backBtn->setStyleSheet("background: transparent; color: #00E676; font-size: 20px; border: none;");
    connect(backBtn, &QPushButton::clicked, [this](){ this->hide(); if (menuRef) menuRef->show(); });
    headerLayout->addWidget(backBtn);
    QLabel* title = new QLabel("Actas");
    title->setStyleSheet("font-weight: bold; font-size: 18px; color: #00E676;");
    headerLayout->addWidget(title); headerLayout->addStretch();
    layout->addWidget(header);

    // Content
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("background-color: #0B141A;");
    QWidget *scrollContent = new QWidget();
    listLayout = new QVBoxLayout(scrollContent);
    listLayout->setContentsMargins(20, 20, 20, 20); listLayout->setSpacing(15); listLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    layout->addWidget(scrollArea);

    // Creation Form
    QWidget* form = new QWidget();
    form->setStyleSheet("background-color: #202C33; padding: 20px;");
    QVBoxLayout* formLayout = new QVBoxLayout(form);
    
    titleInput = new QLineEdit();
    titleInput->setPlaceholderText("Título del acta");
    formLayout->addWidget(titleInput);
    
    contentInput = new QLineEdit();
    contentInput->setPlaceholderText("Contenido");
    formLayout->addWidget(contentInput);
    
    QPushButton* createBtn = new QPushButton("Crear Acta");
    createBtn->setStyleSheet("background-color: #00E676; color: #111B21; font-weight: bold; padding: 10px; border-radius: 10px; margin-top: 10px;");
    connect(createBtn, &QPushButton::clicked, [this](){
        QString title = titleInput->text().trimmed();
        QString content = contentInput->text().trimmed();
        if (title.isEmpty() || content.isEmpty()) {
            QMessageBox::warning(this, "Error", "Rellena todos los campos.");
            return;
        }
        QString id = QString::number(QDateTime::currentMSecsSinceEpoch());
        QString time = QDateTime::currentDateTime().toString("HH:mm");
        saveMinute(id, title, content, time);
        addMinuteWidget(title, content, time);
        titleInput->clear(); contentInput->clear();
        QMessageBox::information(this, "Éxito", "Acta creada.");
    });
    formLayout->addWidget(createBtn);
    layout->addWidget(form);
}

void MinutesWindow::addMinuteWidget(const QString& title, const QString& content, const QString& time) {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QLabel* lbl = new QLabel("📝 " + title + "\n" + content + "\n🕒 " + time);
    lbl->setStyleSheet("background-color: #00E676; color: #111B21; padding: 10px; border-radius: 10px; font-weight: bold;");
    lbl->setWordWrap(true);
    
    layout->addWidget(lbl);
    
    // Insert before the stretch item at the end
    listLayout->insertWidget(listLayout->count() - 1, widget);
    QTimer::singleShot(100, [this](){ scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum()); });
}

void MinutesWindow::loadMinutes() {
    // Clear existing (except stretch)
    QLayoutItem* item;
    while ((item = listLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        else if (item->spacerItem()) { listLayout->addItem(item); break; } // Keep stretch
        delete item;
    }

    QSqlQuery q;
    q.prepare("SELECT title, content, timestamp FROM minutes ORDER BY id ASC");
    if (q.exec()) {
        while (q.next()) {
            addMinuteWidget(q.value("title").toString(), q.value("content").toString(), q.value("timestamp").toString());
        }
    }
}



// --- ContactListWindow Implementation ---
ContactListWindow::ContactListWindow(MainMenu* menu, QString role, QString dni) 
    : menuRef(menu), userRole(role), userDNI(dni) {
    setWindowTitle("Contactos");
    resize(400, 600);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);

    // Header
    QWidget* header = new QWidget();
    header->setStyleSheet("background-color: #202C33; padding: 10px;");
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    QPushButton *backBtn = new QPushButton("⬅");
    backBtn->setFixedSize(30, 30);
    backBtn->setStyleSheet("background: transparent; color: #00A884; font-size: 20px; border: none;");
    connect(backBtn, &QPushButton::clicked, [this](){ this->hide(); if (menuRef) menuRef->show(); });
    headerLayout->addWidget(backBtn);
    QLabel* title = new QLabel("Contactos");
    title->setStyleSheet("font-weight: bold; font-size: 18px; color: #00A884;");
    headerLayout->addWidget(title); headerLayout->addStretch();
    layout->addWidget(header);

    // Content
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("background-color: #0B141A;");
    QWidget *scrollContent = new QWidget();
    listLayout = new QVBoxLayout(scrollContent);
    listLayout->setContentsMargins(20, 20, 20, 20); listLayout->setSpacing(15); listLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    layout->addWidget(scrollArea);
}

void ContactListWindow::loadContacts() {
    // Clear existing (except stretch)
    QLayoutItem* item;
    while ((item = listLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        else if (item->spacerItem()) { listLayout->addItem(item); break; } 
        delete item;
    }

    QSqlQuery q;
    if (userRole == "TUTOR") {
        q.prepare("SELECT * FROM alumnos_prueba WHERE tutor_asignado = :dni");
        q.bindValue(":dni", userDNI);
    } else if (userRole == "ALUMNO") {
        q.prepare("SELECT * FROM tutores_prueba WHERE DNI = (SELECT tutor_asignado FROM alumnos_prueba WHERE DNI = :dni)");
        q.bindValue(":dni", userDNI);
    } else if (userRole == "COORDINADOR") {
        q.prepare("SELECT * FROM alumnos_prueba");
    }

    if (q.exec()) {
        while (q.next()) {
            QString name = q.value("nombre").toString() + " " + q.value("apellidos").toString();
            QString email = q.value("correo").toString();
            QString role = (userRole == "ALUMNO") ? "Tutor" : "Alumno";
            
            QPushButton* btn = new QPushButton(name + "\n" + email);
            btn->setStyleSheet("background-color: #202C33; color: #E9EDEF; font-size: 16px; padding: 15px; border-radius: 10px; text-align: left;");
            connect(btn, &QPushButton::clicked, [this, email](){
                ChatWindow* chat = new ChatWindow(menuRef->userEmail, email, userRole, menuRef);
                chat->show();
                this->hide();
            });
            
            // Insert before stretch
            listLayout->insertWidget(listLayout->count() - 1, btn);
        }
    }
}

// --- ChatWindow Implementation ---
ChatWindow::ChatWindow(QString myEmail, QString otherEmail, QString role, MainMenu* menu) 
    : currentUserEmail(myEmail), remoteUserEmail(otherEmail), userRole(role), menuRef(menu) {
    
    setWindowTitle("Chat con " + otherEmail);
    resize(450, 700);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);

    // Header
    QWidget* header = new QWidget();
    header->setStyleSheet("background-color: #202C33; padding: 10px;");
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    
    QPushButton *backBtn = new QPushButton("⬅");
    backBtn->setFixedSize(30, 30);
    backBtn->setStyleSheet("background: transparent; color: #00A884; font-size: 20px; border: none;");
    connect(backBtn, &QPushButton::clicked, [this](){
        this->hide();
        if (menuRef && menuRef->contactListWindow) menuRef->contactListWindow->show();
    });
    headerLayout->addWidget(backBtn);

    QLabel* avatar = new QLabel(userRole == "TUTOR" ? "🎓" : (userRole == "COORDINADOR" ? "📋" : "👨‍🎓"));
    avatar->setStyleSheet("font-size: 24px; background: transparent;");
    headerLayout->addWidget(avatar);

    QVBoxLayout* titles = new QVBoxLayout();
    QLabel* title = new QLabel(otherEmail);
    title->setStyleSheet("font-weight: bold; font-size: 16px; background: transparent;");
    titles->addWidget(title); 
    headerLayout->addLayout(titles); headerLayout->addStretch();
    layout->addWidget(header);

    // Scroll Area
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("background-color: #0B141A;");
    QWidget *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background-color: #0B141A;");
    scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(10, 10, 10, 10); scrollLayout->setSpacing(10); scrollLayout->addStretch(); 
    scrollArea->setWidget(scrollContent);
    layout->addWidget(scrollArea);

    // Input Area
    QWidget* inputArea = new QWidget();
    inputArea->setStyleSheet("background-color: #202C33; padding: 10px;");
    QHBoxLayout *inputLayout = new QHBoxLayout(inputArea);
    messageInput = new QLineEdit();
    messageInput->setPlaceholderText("Escribe un mensaje... ⌨️");
    QPushButton *sendButton = new QPushButton("➤");
    sendButton->setFixedSize(40, 40);
    inputLayout->addWidget(messageInput); inputLayout->addWidget(sendButton);
    if (userRole == "COORDINADOR") inputArea->hide();
    layout->addWidget(inputArea);

    connect(sendButton, &QPushButton::clicked, [this](){
        QString text = messageInput->text();
        if (text.isEmpty()) return;
        QString id = QString::number(QDateTime::currentMSecsSinceEpoch());
        QString time = QDateTime::currentDateTime().toString("HH:mm");
        
        saveChatMessage(id, currentUserEmail, remoteUserEmail, text, time);
        // Send packet with sender email for identification
        menuRef->sendPacket("MSG|" + id + "|" + currentUserEmail + "|" + text);
        
        addMessage(text, time, true, 0, id);
        messageInput->clear();
    });
    connect(messageInput, &QLineEdit::returnPressed, sendButton, &QPushButton::click);

    loadHistory();
}

void ChatWindow::loadHistory() {
    QSqlQuery historyQuery;
    historyQuery.prepare("SELECT id, content, timestamp, sender FROM chat_messages WHERE (sender = :me AND receiver = :other) OR (sender = :other AND receiver = :me) ORDER BY id ASC");
    historyQuery.bindValue(":me", currentUserEmail);
    historyQuery.bindValue(":other", remoteUserEmail);
    
    if (historyQuery.exec()) {
        while (historyQuery.next()) {
            bool isMine = (historyQuery.value("sender").toString() == currentUserEmail);
            addMessage(historyQuery.value("content").toString(), 
                       historyQuery.value("timestamp").toString(), 
                       isMine, 
                       1, // Status assumed read for history
                       historyQuery.value("id").toString());
        }
    }
    scrollToBottom();
}

void ChatWindow::addMessage(const QString& text, const QString& time, bool isMine, int status, const QString& id) {
    QLabel* st = nullptr;
    scrollLayout->addWidget(createMessageWidget(text, time, isMine, status, &st));
    if (isMine && status == 0 && st && !id.isEmpty()) pendingAcks.insert(id, st);
    scrollToBottom();
}

void ChatWindow::updateAck(const QString& id) {
    if (pendingAcks.contains(id)) {
        pendingAcks[id]->setText("✓✓");
        pendingAcks[id]->setStyleSheet("color: #53BDEB; font-size: 11px; font-weight: bold; background: transparent; margin-left: 5px;");
        pendingAcks.remove(id);
    }
}

void ChatWindow::scrollToBottom() {
    QTimer::singleShot(100, [this](){ scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum()); });
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setStyleSheet("QWidget { background-color: #111B21; font-family: 'Segoe UI', sans-serif; color: #E9EDEF; } "
                      "QLineEdit { background-color: #2A3942; border-radius: 20px; padding: 10px; } "
                      "QPushButton { background-color: #00A884; color: #111B21; border-radius: 20px; font-weight: bold; }");

    int myPort = 5000, targetPort = 5001;
    if (argc == 3) { myPort = atoi(argv[1]); targetPort = atoi(argv[2]); }

    initDb();

    LoginDialog login;
    if (login.exec() != QDialog::Accepted) return 0;

    MainMenu menu(login.role, login.userEmail, login.userDNI, login.targetIP, login.myPort, login.targetPort);
    menu.show();
    return app.exec();
}

