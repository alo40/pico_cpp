package SolarMonitoringArchitecture {

    part def SmartSolarMPPT;
    part def RaspberryPiPico;
    part def RaspberryPi;
    part def Mac;
    part def JupyterAnalysis;

    part def SolarMonitoringSystem {
        part mppt : SmartSolarMPPT;
        part pico : RaspberryPiPico;
        part loggerHost : RaspberryPi;
        part analysisHost : Mac;
        part analysis : JupyterAnalysis;
    }

    connection def VEDirectConnection {
        end mppt : SmartSolarMPPT;
        end pico : RaspberryPiPico;
    }

    connection def USBSerialConnection {
        end pico : RaspberryPiPico;
        end loggerHost : RaspberryPi;
    }

    connection def NetworkSynchronization {
        end loggerHost : RaspberryPi;
        end analysisHost : Mac;
    }

    connection def LocalDataAccess {
        end analysisHost : Mac;
        end analysis : JupyterAnalysis;
    }

    part solarMonitoringSystem : SolarMonitoringSystem {
        connection veDirect : VEDirectConnection connect mppt to pico;
        connection usbSerial : USBSerialConnection connect pico to loggerHost;
        connection synchronization : NetworkSynchronization connect loggerHost to analysisHost;
        connection localData : LocalDataAccess connect analysisHost to analysis;
    }
}
