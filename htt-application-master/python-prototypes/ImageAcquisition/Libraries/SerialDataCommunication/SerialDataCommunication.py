import serial
from PyQt5 import QtCore
from PyQt5.QtCore import QObject, QThread, QMutex, pyqtSignal
import time


class DataCommunication(QObject):


    # Signal to be used to get data received
    data_ready = pyqtSignal(bytes)


    def __init__(self, serial_port):
        QObject.__init__(self)

        self.serial_port         = serial_port
        self.communication_mutex = QMutex()
        self.receiver_thread     = QThread()
        # self.receiver_thread.setPriority(QThread.LowPriority)
        self.moveToThread(self.receiver_thread)
        self.receiver_thread.started.connect(self.receiveData)
        # self.receiver_thread.finished.connect(self.deleteLater)
        self.receiving_is_on  = False
        self.receiver_thread.start()


    def startReceiving(self):
        """ Start data receiving phase """

        self.communication_mutex.lock()

        if self.receiving_is_on:
            self.communication_mutex.unlock()
            return

        # Start the receiving phase
        self.receiving_is_on  = True

        self.communication_mutex.unlock()


    def stopReceiving(self):
        """ Stop data receiving phase """


        self.communication_mutex.lock()

        if not self.receiving_is_on:
            self.communication_mutex.unlock()
            return

        # Stop the receiving phase
        self.receiving_is_on = False

        self.communication_mutex.unlock()


    def receivingIsOn(self):
        """ It says if the receiving phase is on or off """

        return self.receiving_is_on


    def sendData(self, data):
        """ Send data (commands) to the device.
        'data' is a string.
        """

        self.communication_mutex.lock()

        ret = False
        try:
            self.serial_port.write(data.encode())
            ret = True
        except:
            ret = False

        self.communication_mutex.unlock()

        return ret


    @QtCore.pyqtSlot()
    def receiveData(self):
        """" Receiver thread """

        while True:

            while self.receiving_is_on:

                self.communication_mutex.lock()

                try:
                    if self.serial_port:
                        if self.serial_port.in_waiting:
                            # Get data
                            data = self.serial_port.read_all()
                            print("> DataCommunication.receiveData - data read: " + str(data))
                            # Emit them
                            self.data_ready.emit(data)
                except:
                    pass

                self.communication_mutex.unlock()

                # time.sleep(5)

            while not self.receiving_is_on:
                pass
                # time.sleep(5)
