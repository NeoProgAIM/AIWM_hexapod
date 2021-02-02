#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <QThread>
#include "swlp.h"
#include "streamservice.h"
#include "streamframeprovider.h"

class Core : public QObject {
    Q_OBJECT
public:
    explicit Core(StreamService* streamService);
    virtual ~Core() {}

    Q_INVOKABLE bool startCommunication();
    Q_INVOKABLE void stopCommunication();

    Q_INVOKABLE void sendGetUpCommand();
    Q_INVOKABLE void sendGetDownCommand();
    Q_INVOKABLE void sendUpDownCommand();
    Q_INVOKABLE void sendPushPullCommand();
    Q_INVOKABLE void sendAttackLeftCommand();
    Q_INVOKABLE void sendAttackRightCommand();
    Q_INVOKABLE void sendDanceCommand();
    Q_INVOKABLE void sendRotateXCommand();
    Q_INVOKABLE void sendRotateZCommand();
    Q_INVOKABLE void sendStopMoveCommand();
    Q_INVOKABLE void sendStartMotionCommand(QVariant stepLength, QVariant curvature);

    Q_INVOKABLE void setMotionSpeed(QVariant motionSpeed);


public: // Call from SWLP thread
    void swlpStatusPayloadProcess(const swlp_status_payload_t* payload);
    void swlpCommandPayloadPrepare(swlp_command_payload_t* payload);

signals:
    // To SWLP module
    void swlpRunCommunication();
    
    // To StreamService module
    void streamServiceRun(QString cameraIp);

    // To QML from SWLP
    void swlpFrameReceived();
    void swlpConnectionClosed();
    void swlpSystemStatusUpdated(QVariant newSystemStatus, QVariant newModuleStatus);
    void swlpBatteryStatusUpdated(QVariant newBatteryCharge, QVariant newBatteryVoltage);

protected:
    Swlp m_swlp;
    StreamService* m_streamService;

    uint8_t m_commandForSend        {SWLP_CMD_NONE};
    uint8_t m_stepLenght            {0};
    int16_t m_curvature             {0};
    uint8_t m_motionSpeed           {90}; // %
};

#endif // CORE_H
