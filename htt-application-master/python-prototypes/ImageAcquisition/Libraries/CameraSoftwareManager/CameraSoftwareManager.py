# import sys
import os
import subprocess
# import signal
import psutil
# import win32con
import win32gui
import win32process
import win32api
from PIL import ImageGrab
from enum import Enum
from time import sleep
import logging


########################
###    Parameters    ###
########################
# Camera software
CAM_EXE_FOLDER_PATH   = "C:\Program Files (x86)\C3842C"
CAM_EXE_NAME          = "C3842.exe"
C3842_PROCESS_NAME    = CAM_EXE_NAME
OVCAMREG_PROCESS_NAME = "OVCamReg.exe"
C3842_MAIN_WND_WIDTH  = 670
C3842_MAIN_WND_HEIGHT = 521
C3842_ERR_WND_WIDTH   = 368
C3842_ERR_WND_HEIGHT  = 145
C3842_FRAME_WIDTH     = 400
C3842_FRAME_HEIGHT    = 400
C3842_FRAME_REL_X     = 248
C3842_FRAME_REL_Y     =  61
########################


### Enum that represents the code result of an operation
class HTTCameraResultCode(Enum):
    HTT_CAM_SUCCESSFUL        =  0
    HTT_CAM_PROCESS_ERROR     = -1
    HTT_CAM_NOT_CONNECTED     = -2
    HTT_CAM_CANNOT_TOUCH_WND  = -3
    HTT_CAM_APP_WND_NOT_FOCUS = -4
    HTT_CAM_GENERIC_ERROR     = -99


### Class used to manage the camera software
class HTTCameraManager:


    def __init__(self):
        self._reset_state()

    def moveToDir(self, folder_path):
        # Move to the application software directory
        os.chdir(folder_path)
        logging.debug("os.chdir\t[ok]")

    def launchCameraExe(self, folder_path=None, debug=False):
        """ Launch the camera executable """

        logging.info("Starts")

        # Save folder path
        application_folder_path = os.getcwd()
        if folder_path:
            self.cam_exe_folder_path = folder_path
        else:
            self.cam_exe_folder_path = CAM_EXE_FOLDER_PATH

        # Find all running processes
        processes_before = []
        for proc in psutil.process_iter():
            processes_before.append(proc)

        #############
        ### Debug ###
        if debug:
            print("====================================")
            print("=== Running processes before launch:")
            print("====================================")
            for proc in processes_before:
                print(proc)
        #############

        # If there are already some camera software instances running,
        # close them
        for proc in psutil.process_iter():
            if proc.name() == C3842_PROCESS_NAME or proc.name() == OVCAMREG_PROCESS_NAME:
                proc.kill()

        self._reset_state()

        # Try to launch the software
        try:
            self.USE_PROCESS_SHELL = False
            # Move to the camera software directory
            os.chdir(self.cam_exe_folder_path)
            logging.debug("os.chdir\t[ok]")

            # Launch the camera process by a shell
            if self.USE_PROCESS_SHELL:
                self.process = subprocess.Popen(CAM_EXE_NAME, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                logging.debug("subprocess.Popen (with shell)\t[ok]")

            else:
                self.process = subprocess.Popen(CAM_EXE_NAME, shell=False, stdout=None, stderr=None)
                logging.debug("subprocess.Popen (without shell)\t[ok]")

            # Wait for the processes to start
            sleep(4)
            # Get parent and child processes
            if self.USE_PROCESS_SHELL:
                self.parent_process   = psutil.Process(self.process.pid)
                logging.debug("Parent process\t[ok]")

                self.cmd_process      = self.parent_process
                self.child_processes  = self.parent_process.children(recursive=True)
                logging.debug("Child processes\t[ok]")

                self.C3842_process    = next(x for x in self.child_processes if x.name() == C3842_PROCESS_NAME) # target process
                logging.debug("C3842 process [ok]")

                self.OVCamReg_process = next(x for x in self.child_processes if x.name() == OVCAMREG_PROCESS_NAME)
                logging.debug("OVCamReg process\t[ok]")

                logging.debug("All processes\t[ok]")
            else:
                self.parent_process   = psutil.Process(self.process.pid)
                logging.debug("Parent (C3842) process\t[ok]")

                self.cmd_process      = None
                self.C3842_process    = self.parent_process
                self.child_processes  = self.parent_process.children(recursive=True)
                logging.debug("Child processes\t[ok]")

                self.OVCamReg_process = next(x for x in self.child_processes if x.name() == OVCAMREG_PROCESS_NAME)
                logging.debug("OVCamReg process\t[ok]")

                logging.debug("All processes\t[ok]")

            # Get window IDs
            self.C3842_hwnds     = self._get_hwnds_by_pid(self.C3842_process.pid)  # target process window ID
            self.C3842_main_hwnd = self.C3842_hwnds[0]
            logging.debug("UI window handlers\t[ok]")

        except:
            # Return with error
            logging.error("Camera process error")
            # Move to the application software directory
            self.moveToDir(application_folder_path)
            return HTTCameraResultCode.HTT_CAM_PROCESS_ERROR

        #############
        ### Debug ###
        if debug:
            print("Camera parent process ID: " + str(self.parent_process.pid))
        logging.debug("Camera parent process ID: " + str(self.parent_process.pid))
        #############

        # Find all running processes
        processes_after = []
        for proc in psutil.process_iter():
            processes_after.append(proc)

        #############
        ### Debug ###
        if debug:
            print("===================================")
            print("=== Running processes after launch:")
            print("===================================")
            for proc in processes_after:
                print(proc)
        #############

        # Get target window info
        #  Position and size
        rect = win32gui.GetWindowRect(self.C3842_main_hwnd)
        self.x = rect[0]
        self.y = rect[1]
        self.w = rect[2] - self.x
        self.h = rect[3] - self.y
        logging.debug("C3842 window size: " + str(self.w) + "x" + str(self.h))

        # Check window size
        # if self.w != C3842_MAIN_WND_WIDTH or self.h != C3842_MAIN_WND_HEIGHT:
        # TODO: Relaxes the constraint in a intervall of +/- 5 pixels
        if self.w < C3842_MAIN_WND_WIDTH-5 or self.w > C3842_MAIN_WND_WIDTH+5 or self.h < C3842_MAIN_WND_HEIGHT-5 or self.h > C3842_MAIN_WND_HEIGHT+5:
            # Main window is not the one expected
            # when the camera is correctly connected
            logging.info("Camera not connected")
            # Move to the application software directory
            self.moveToDir(application_folder_path)
            return HTTCameraResultCode.HTT_CAM_NOT_CONNECTED

        # Move main window to top left
        try:
            win32gui.MoveWindow(self.C3842_main_hwnd, 0, 0, self.w, self.h, 0)
            logging.debug("win32gui.MoveWindow\t[ok]")
        except:
            # Return with error
            logging.warning("Cannot move C3842 window (admin needed)")
            # Move to the application software directory
            self.moveToDir(application_folder_path)
            return HTTCameraResultCode.HTT_CAM_CANNOT_TOUCH_WND

        # Return successful
        logging.info("Camera application correctly launched")
        # Move to the application software directory
        self.moveToDir(application_folder_path)
        return HTTCameraResultCode.HTT_CAM_SUCCESSFUL


    def closeCameraExe(self):
        # Kill all processes linked to the camera
        try:
            if self.parent_process is not None:
                self.parent_process.kill()
                logging.debug("Parent process closed")
            if self.child_processes is not None:
                for cp in self.child_processes:
                    cp.kill()
                logging.debug("Child processes closed")
            return True
        except:
            logging.error("Camera process closing error")
            return False


    def grabCameraFrame(self, debug=False):
        # Grab current camera frame

        # Check process state
        if self.C3842_process is None:
            return HTTCameraResultCode.HTT_CAM_PROCESS_ERROR, None

        # Get target window ID again,
        # also to check that camera window wasn't close in the meantime
        try:
            self.C3842_hwnds     = self._get_hwnds_by_pid(self.C3842_process.pid)  # target process window ID
            self.C3842_main_hwnd = self.C3842_hwnds[0]
            logging.debug("C3842 window handler recovering\t[ok]")
        except:
            logging.error("Error during C3842 window handler recovering")
            return HTTCameraResultCode.HTT_CAM_PROCESS_ERROR, None

        try:
            win32gui.SetForegroundWindow(self.C3842_main_hwnd)
            logging.debug("win32gui.SetForegroundWindow\t[ok]")
            # Get window rectangle
            wbox = win32gui.GetWindowRect(self.C3842_main_hwnd)
            logging.debug("win32gui.GetWindowRect\t[ok]")
            # Get frame rectangle
            fbox = tuple([wbox[0] + C3842_FRAME_REL_X,
                          wbox[1] + C3842_FRAME_REL_Y,
                          wbox[0] + C3842_FRAME_REL_X + C3842_FRAME_WIDTH,
                          wbox[1] + C3842_FRAME_REL_Y + C3842_FRAME_HEIGHT])
            logging.debug("fbox\t[ok]")
            # Get the frame!
            image = ImageGrab.grab(fbox)
            logging.info("Frame portion grabbed!")
        except:
            logging.error("Cannot focus on the C3842 window")
            return HTTCameraResultCode.HTT_CAM_APP_WND_NOT_FOCUS, None

        #############
        ### Debug ###
        if debug:
            image.show()
        #############

        return HTTCameraResultCode.HTT_CAM_SUCCESSFUL, image



    def _reset_state(self):
        self.process          = None
        self.parent_process   = None
        self.cmd_process      = None
        self.child_processes  = None
        self.C3842_process    = None
        self.OVCamReg_process = None
        self.C3842_hwnds      = None
        self.C3842_main_hwnd  = None


    @staticmethod
    def _get_hwnds_by_pid(pid):
        def callback(hwnd, hwnds):
            if win32gui.IsWindowVisible(hwnd) and win32gui.IsWindowEnabled(hwnd):
                _, found_pid = win32process.GetWindowThreadProcessId(hwnd)
                if found_pid == pid:
                    hwnds.append(hwnd)
            return True

        hwnds = []
        win32gui.EnumWindows(callback, hwnds)
        return hwnds




if __name__ == "__main__":

    dbg = True

    imaging_manager = HTTCameraManager()
    retcode = imaging_manager.launchCameraExe(debug=dbg)
    if HTTCameraResultCode.HTT_CAM_SUCCESSFUL == retcode:
        print("Camera software correctly started.")
    elif HTTCameraResultCode.HTT_CAM_NOT_CONNECTED == retcode:
        print("ERROR: Camera not connected.")
    elif HTTCameraResultCode.HTT_CAM_PROCESS_ERROR == retcode:
        print("ERROR: Unable to launch camera software.")
    elif HTTCameraResultCode.HTT_CAM_CANNOT_TOUCH_WND == retcode:
        print("ERROR: Unable to move/touch camera software window.")
    else:
        print("ERROR: Generic fault situation.")

    sleep(5)

    imaging_manager.closeCameraExe()








